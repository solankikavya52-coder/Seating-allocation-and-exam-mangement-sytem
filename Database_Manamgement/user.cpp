#include <iostream>
#include <string>
#include <memory>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/exception.h>
#include <bcrypt/BCrypt.hpp>

using namespace std;
using namespace sql;

struct CreateUserResult {
    bool success;
    string message;
    int user_id = -1;
};

CreateUserResult createUser(
    const string& username,
    const string& password,
    const string& role
) {
    try {
        string passwordHash = BCrypt::generateHash(password);

        Driver* driver = get_driver_instance();

        unique_ptr<Connection> db(
            driver->connect("tcp://127.0.0.1:3306", "root", "")
        );

        db->setSchema("exam_system");

        unique_ptr<PreparedStatement> stmt(
            db->prepareStatement(
                "INSERT INTO `USER` "
                "(username, password_hash, role) "
                "VALUES (?, ?, ?)"
            )
        );

        stmt->setString(1, username);
        stmt->setString(2, passwordHash);
        stmt->setString(3, role);

        stmt->executeUpdate();

        unique_ptr<PreparedStatement> idStmt(
            db->prepareStatement("SELECT LAST_INSERT_ID()")
        );

        unique_ptr<sql::ResultSet> result(idStmt->executeQuery());

        int userId = -1;

        if (result->next()) {
            userId = result->getInt(1);
        }

        return {
            true,
            "User created successfully",
            userId
        };
    } catch (const SQLException& e) {
        cerr << "Database error: " << e.what() << '\n';

        return {
            false,
            "Failed to create user"
        };
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << '\n';

        return {
            false,
            "Failed to create user"
        };
    }
}

int main() {
    string username;
    string password;
    string role;

    cout << "Enter username: ";
    getline(cin, username);

    cout << "Enter password: ";
    getline(cin, password);

    cout << "Enter role (Admin/Invigilator/Student): ";
    getline(cin, role);

    CreateUserResult result = createUser(
        username,
        password,
        role
    );

    cout << result.message << '\n';

    if (result.success) {
        cout << "User ID: " << result.user_id << '\n';
    }

    return 0;
}