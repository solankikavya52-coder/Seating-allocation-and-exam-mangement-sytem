/**
 * ============================================================================
 * Examination Operations & Intelligent Seating Allocation Platform
 * Module: Seating Allocation Engine (DBMS Transactional Core)
 * Author: Ayush Chauhan (Student 4, Roll No: 2418385)
 * ============================================================================
 * 
 * Technical Highlights:
 *  - ACID Transaction Guarantee: Atomic commit/rollback via MySQL Connector/C++
 *  - Strict Uniqueness: Enforces composite UNIQUE (room_id, seat_no)
 *  - Capacity Pre-Validation: Verifies total room seats >= total students before writes
 *  - Critical Section Safety: Mutex protection over multi-row allocations
 */

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>

using namespace std;
using namespace sql;

// Mutex protecting critical allocation routines
mutex allocationMutex;

struct Student {
    int studentId;
    string rollNo;
    string name;
};

struct Room {
    int roomId;
    string roomNo;
    int capacity;
};

int main() {
    cout << "========================================================================\n";
    cout << "   EXAMINATION ALLOCATION PLATFORM - TRANSACTIONAL ALLOCATION ENGINE    \n";
    cout << "   Student 4: Ayush Chauhan (Roll No: 2418385)                          \n";
    cout << "========================================================================\n\n";

    unique_ptr<Connection> con;

    try {
        Driver* driver = get_driver_instance();
        con.reset(driver->connect("tcp://127.0.0.1:3306", "root", ""));
        con->setSchema("exam_system");

        vector<Student> students;
        vector<Room> rooms;

        unique_ptr<Statement> stmt(con->createStatement());

        // 1. Fetch eligible students sorted by roll number
        unique_ptr<ResultSet> studentResult(stmt->executeQuery(
            "SELECT student_id, roll_no, name FROM STUDENT ORDER BY roll_no ASC"
        ));

        while (studentResult->next()) {
            students.push_back({
                studentResult->getInt("student_id"),
                studentResult->getString("roll_no"),
                studentResult->getString("name")
            });
        }
        studentResult.reset();

        // 2. Fetch available examination rooms
        unique_ptr<ResultSet> roomResult(stmt->executeQuery(
            "SELECT room_id, room_no, capacity FROM ROOM ORDER BY room_no ASC"
        ));

        long long totalCapacity = 0;
        while (roomResult->next()) {
            Room room{
                roomResult->getInt("room_id"),
                roomResult->getString("room_no"),
                roomResult->getInt("capacity")
            };

            if (room.capacity <= 0) {
                cerr << "Error: Room " << room.roomNo << " has invalid capacity <= 0.\n";
                return 1;
            }

            totalCapacity += room.capacity;
            rooms.push_back(room);
        }
        roomResult.reset();

        cout << "Loaded " << students.size() << " student(s) and " 
             << rooms.size() << " room(s) with total capacity of " << totalCapacity << " seats.\n";

        // 3. Validation: Check if students exist
        if (students.empty()) {
            cout << "No eligible students found for allocation.\n";
            return 0;
        }

        // 4. Capacity Pre-Validation (DBMS Rule: Fail fast before modifying state)
        if (rooms.empty() || totalCapacity < static_cast<long long>(students.size())) {
            cerr << "\n[!] CRITICAL ERROR: Insufficient room capacity!\n"
                 << "    - Total Students : " << students.size() << "\n"
                 << "    - Available Seats: " << totalCapacity << "\n"
                 << "    Aborting allocation to prevent partial/corrupted data writes.\n";
            return 1;
        }

        // 5. ACID Transaction: Disable autocommit
        con->setAutoCommit(false);

        try {
            // Clear previous allocation batch atomically
            stmt->executeUpdate("DELETE FROM ALLOCATION");

            unique_ptr<PreparedStatement> insertAllocation(
                con->prepareStatement(
                    "INSERT INTO ALLOCATION (student_id, room_id, seat_no) VALUES (?, ?, ?)"
                )
            );

            size_t studentIndex = 0;

            for (const Room& room : rooms) {
                for (int seat = 1; seat <= room.capacity && studentIndex < students.size(); ++seat, ++studentIndex) {
                    {
                        lock_guard<mutex> lock(allocationMutex);
                        insertAllocation->setInt(1, students[studentIndex].studentId);
                        insertAllocation->setInt(2, room.roomId);
                        insertAllocation->setInt(3, seat);
                        insertAllocation->executeUpdate();
                    }

                    cout << "Seat #" << seat << " | Room " << room.roomNo 
                         << " <- " << students[studentIndex].rollNo 
                         << " (" << students[studentIndex].name << ")\n";
                }

                if (studentIndex == students.size()) {
                    break;
                }
            }

            // Commit atomic transaction
            con->commit();
            con->setAutoCommit(true);
            cout << "\n[+] SUCCESS: All " << students.size() << " students allocated atomically.\n";

        } catch (...) {
            // Roll back all changes if any error/constraint violation occurs
            con->rollback();
            con->setAutoCommit(true);
            cerr << "\n[-] TRANSACTION ROLLED BACK: Exception encountered during allocation.\n";
            throw;
        }

    } catch (const SQLException& e) {
        if (con) {
            try {
                con->rollback();
                con->setAutoCommit(true);
            } catch (...) {}
        }
        cerr << "Database SQLException (Code " << e.getErrorCode() << "): " << e.what() << '\n';
        return 1;
    } catch (const exception& e) {
        if (con) {
            try {
                con->rollback();
                con->setAutoCommit(true);
            } catch (...) {}
        }
        cerr << "Standard Exception: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
