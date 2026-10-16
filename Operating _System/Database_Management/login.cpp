#include <iostream>
#include <string>
#include <memory>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>
#include <bcrypt/BCrypt.hpp>

using namespace std;
using namespace sql;

struct LoginResult {
    bool success;
    string message;
    int user_id = -1;
    string username;
    string role;
};

LoginResult login(const string& username, const string& password) {
    try {
        Driver* driver = get_driver_instance();

        unique_ptr<Connection> db(
            driver->connect("tcp://127.0.0.1:3306", "root", "")
        );

        db->setSchema("exam_system");

        unique_ptr<PreparedStatement> stmt(
            db->prepareStatement(
                "SELECT user_id, username, password_hash, role "
                "FROM `USER` WHERE username = ?"
            )
        );

        stmt->setString(1, username);

        unique_ptr<ResultSet> result(stmt->executeQuery());

        if (!result->next()) {
            return {
                false,
                "Invalid username or password"
            };
        }

        int userId = result->getInt("user_id");
        string storedUsername = result->getString("username");
        string passwordHash = result->getString("password_hash");
        string role = result->getString("role");

        bool validPassword = BCrypt::validatePassword(
            password,
            passwordHash
        );

        if (!validPassword) {
            return {
                false,
                "Invalid username or password"
            };
        }

        return {
            true,
            "Login successful",
            userId,
            storedUsername,
            role
        };
    } catch (const exception&) {
        return {
            false,
            "An internal error occurred"
        };
    }
}

int main() {
    string username;
    string password;

    cout << "Username: ";
    getline(cin, username);

    cout << "Password: ";
    getline(cin, password);

    LoginResult result = login(username, password);

    cout << result.message << '\n';

    if (result.success) {
        cout << "User ID: " << result.user_id << '\n';
        cout << "Username: " << result.username << '\n';
        cout << "Role: " << result.role << '\n';
    }

    return 0;
}