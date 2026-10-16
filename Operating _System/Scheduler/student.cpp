#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>

using namespace std;
using namespace sql;

struct Student {
    int student_id;
    string roll_no;
    string name;
    string email;
    string branch_name;
    int semester_no;
};

struct DeleteResult {
    int affectedRows;
};

unique_ptr<Connection> connectDB() {
    Driver* driver = get_driver_instance();

    unique_ptr<Connection> db(
        driver->connect("tcp://127.0.0.1:3306", "root", "")
    );

    db->setSchema("exam_system");

    return db;
}

vector<Student> executeStudentQuery(
    const string& query,
    const string& parameter,
    bool useParameter
) {
    auto db = connectDB();

    unique_ptr<PreparedStatement> stmt(
        db->prepareStatement(query)
    );

    if (useParameter) {
        stmt->setString(1, parameter);
    }

    unique_ptr<ResultSet> result(stmt->executeQuery());

    vector<Student> students;

    while (result->next()) {
        Student s;

        s.student_id = result->getInt("student_id");
        s.roll_no = result->getString("roll_no");
        s.name = result->getString("name");

        if (result->isNull("email")) {
            s.email = "";
        } else {
            s.email = result->getString("email");
        }

        s.branch_name = result->getString("branch_name");
        s.semester_no = result->getInt("semester_no");

        students.push_back(s);
    }

    return students;
}

vector<Student> getStudentByRollNo(const string& rollNo) {
    string query = R"(
        SELECT
            s.student_id,
            s.roll_no,
            s.name,
            s.email,
            b.branch_name,
            se.semester_no
        FROM STUDENT s
        JOIN BRANCH b
            ON s.branch_id = b.branch_id
        JOIN SEMESTER se
            ON s.semester_id = se.semester_id
        WHERE s.roll_no = ?
    )";

    return executeStudentQuery(query, rollNo, true);
}

vector<Student> getStudentsByBranch(int branchId) {
    string query = R"(
        SELECT
            s.student_id,
            s.roll_no,
            s.name,
            s.email,
            b.branch_name,
            se.semester_no
        FROM STUDENT s
        JOIN BRANCH b
            ON s.branch_id = b.branch_id
        JOIN SEMESTER se
            ON s.semester_id = se.semester_id
        WHERE s.branch_id = ?
        ORDER BY s.roll_no
    )";

    return executeStudentQuery(query, to_string(branchId), true);
}

vector<Student> getAllStudents() {
    string query = R"(
        SELECT
            s.student_id,
            s.roll_no,
            s.name,
            s.email,
            b.branch_name,
            se.semester_no
        FROM STUDENT s
        JOIN BRANCH b
            ON s.branch_id = b.branch_id
        JOIN SEMESTER se
            ON s.semester_id = se.semester_id
        ORDER BY s.roll_no
    )";

    return executeStudentQuery(query, "", false);
}

DeleteResult deleteStudent(const string& rollNo) {
    auto db = connectDB();

    unique_ptr<PreparedStatement> stmt(
        db->prepareStatement(
            "DELETE FROM STUDENT WHERE roll_no = ?"
        )
    );

    stmt->setString(1, rollNo);

    int affected = stmt->executeUpdate();

    return {affected};
}

void displayStudents(const vector<Student>& students) {
    for (const auto& s : students) {
        cout << "Student ID: " << s.student_id << '\n';
        cout << "Roll No: " << s.roll_no << '\n';
        cout << "Name: " << s.name << '\n';
        cout << "Email: " << s.email << '\n';
        cout << "Branch: " << s.branch_name << '\n';
        cout << "Semester: " << s.semester_no << "\n\n";
    }

    if (students.empty()) {
        cout << "No students found.\n";
    }
}

int main() {
    try {
        int choice;

        cout << "1. Search student by roll number\n";
        cout << "2. Get students by branch ID\n";
        cout << "3. Get all students\n";
        cout << "4. Delete student\n";
        cout << "Enter choice: ";
        cin >> choice;

        if (choice == 1) {
            string rollNo;

            cout << "Enter roll number: ";
            cin >> rollNo;

            displayStudents(getStudentByRollNo(rollNo));
        } else if (choice == 2) {
            int branchId;

            cout << "Enter branch ID: ";
            cin >> branchId;

            displayStudents(getStudentsByBranch(branchId));
        } else if (choice == 3) {
            displayStudents(getAllStudents());
        } else if (choice == 4) {
            string rollNo;

            cout << "Enter roll number to delete: ";
            cin >> rollNo;

            DeleteResult result = deleteStudent(rollNo);

            if (result.affectedRows > 0) {
                cout << "Student deleted successfully.\n";
            } else {
                cout << "Student not found.\n";
            }
        } else {
            cout << "Invalid choice.\n";
        }
    } catch (const SQLException& e) {
        cerr << "Database error: " << e.what() << '\n';
        return 1;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}