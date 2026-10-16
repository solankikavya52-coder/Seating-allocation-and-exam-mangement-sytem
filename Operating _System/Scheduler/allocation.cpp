#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
using namespace std;

struct Student {
    string rollNo;
    string name;
};

struct Room {
    string roomNo;
    int capacity;
};

int main() {
    try {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();

        unique_ptr<sql::Connection> con(
            driver->connect("tcp://127.0.0.1:3306", "root", "")
        );

        con->setSchema("exam_system");

        unique_ptr<sql::Statement> stmt(con->createStatement());

        vector<Student> students;
        vector<Room> rooms;

        unique_ptr<sql::ResultSet> studentResult(
            stmt->executeQuery(
                "SELECT roll_no, name FROM STUDENT ORDER BY roll_no"
            )
        );

        while (studentResult->next()) {
            Student s;
            s.rollNo = studentResult->getString("roll_no");
            s.name = studentResult->getString("name");
            students.push_back(s);
        }

        unique_ptr<sql::ResultSet> roomResult(
            stmt->executeQuery(
                "SELECT room_no, capacity FROM ROOM ORDER BY room_no"
            )
        );

        while (roomResult->next()) {
            Room r;
            r.roomNo = roomResult->getString("room_no");
            r.capacity = roomResult->getInt("capacity");
            rooms.push_back(r);
        }

        int totalCapacity = 0;
        for (const Room& room : rooms) {
            if (room.capacity > 0) {
                totalCapacity += room.capacity;
            }
        }

        if (totalCapacity < static_cast<int>(students.size())) {
            cerr << "Not enough room capacity" << endl;
            return 1;
        }

        int studentIndex = 0;

        for (const Room& room : rooms) {
            for (int count = 0;
                 count < room.capacity && studentIndex < static_cast<int>(students.size());
                 count++) {
                cout << students[studentIndex].rollNo
                     << " (" << students[studentIndex].name << ")"
                     << " -> Room " << room.roomNo << endl;

                studentIndex++;
            }

            if (studentIndex == static_cast<int>(students.size())) {
                break;
            }
        }

        cout << "Room allocation completed successfully." << endl;
    }
    catch (sql::SQLException& e) {
        cerr << "Database error: " << e.what() << endl;
        return 1;
    }

    return 0;
}