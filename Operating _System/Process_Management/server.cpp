#include <crow.h>
#include <iostream>
#include <string>
#include <mutex>
#include <memory>
#include <windows.h>
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

crow::SimpleApp app;
PROCESS_INFORMATION allocationProcess{};
bool allocationRunning = false;
string processStatus = "NOT_RUNNING";
mutex processMutex;

unique_ptr<Connection> connectDB() {
    Driver* driver = get_mysql_driver_instance();
    unique_ptr<Connection> db(
        driver->connect("tcp://127.0.0.1:3306", "root", "")
    );
    db->setSchema("exam_system");
    return db;
}

crow::response jsonResponse(int status, crow::json::wvalue body) {
    crow::response res(status);
    res.set_header("Content-Type", "application/json");
    res.body = body.dump();
    return res;
}

crow::json::wvalue makeResult(bool success, const string& message) {
    crow::json::wvalue result;
    result["success"] = success;
    result["message"] = message;
    return result;
}

void refreshAllocationStatusLocked() {
    if (!allocationRunning || allocationProcess.hProcess == nullptr) {
        return;
    }

    DWORD waitResult = WaitForSingleObject(allocationProcess.hProcess, 0);
    if (waitResult == WAIT_OBJECT_0) {
        DWORD exitCode = 1;
        GetExitCodeProcess(allocationProcess.hProcess, &exitCode);
        CloseHandle(allocationProcess.hProcess);
        CloseHandle(allocationProcess.hThread);
        allocationProcess = {};
        allocationRunning = false;
        processStatus = exitCode == 0 ? "COMPLETED" : "FAILED";
    } else if (waitResult == WAIT_FAILED) {
        processStatus = "FAILED";
    }
}

crow::response startAllocation() {
    lock_guard<mutex> lock(processMutex);
    refreshAllocationStatusLocked();

    if (allocationRunning) {
        return jsonResponse(409, makeResult(false, "Allocation process is already running"));
    }

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    char command[] = "allocation.exe";

    BOOL created = CreateProcessA(
        nullptr, command, nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
        nullptr, nullptr, &si, &pi
    );

    if (!created) {
        processStatus = "FAILED";
        return jsonResponse(
            500,
            makeResult(false, "Failed to start allocation.exe. Check the server working directory and file permissions.")
        );
    }

    allocationProcess = pi;
    allocationRunning = true;
    processStatus = "RUNNING";
    return jsonResponse(200, makeResult(true, "Allocation process started"));
}

crow::response stopAllocation() {
    lock_guard<mutex> lock(processMutex);
    refreshAllocationStatusLocked();

    if (!allocationRunning) {
        return jsonResponse(404, makeResult(false, "No allocation process is running"));
    }

    if (!TerminateProcess(allocationProcess.hProcess, 1)) {
        refreshAllocationStatusLocked();
        return jsonResponse(500, makeResult(false, "Failed to terminate allocation process"));
    }

    WaitForSingleObject(allocationProcess.hProcess, INFINITE);
    CloseHandle(allocationProcess.hProcess);
    CloseHandle(allocationProcess.hThread);
    allocationProcess = {};
    allocationRunning = false;
    processStatus = "TERMINATED";
    return jsonResponse(200, makeResult(true, "Allocation process terminated"));
}

crow::json::wvalue studentToJson(ResultSet* row) {
    crow::json::wvalue item;
    item["student_id"] = row->getInt("student_id");
    item["roll_no"] = row->getString("roll_no");
    item["name"] = row->getString("name");
    item["email"] = row->isNull("email") ? "" : row->getString("email");
    item["branch_name"] = row->getString("branch_name");
    item["semester_no"] = row->getInt("semester_no");
    return item;
}

crow::response listStudents(int mode, const string& rollNo = "", int branchId = 0) {
    try {
        auto db = connectDB();
        string query =
            "SELECT s.student_id, s.roll_no, s.name, s.email, "
            "b.branch_name, se.semester_no "
            "FROM STUDENT s "
            "JOIN BRANCH b ON s.branch_id = b.branch_id "
            "JOIN SEMESTER se ON s.semester_id = se.semester_id ";

        if (mode == 1) {
            query += "WHERE s.roll_no = ? ";
        } else if (mode == 2) {
            query += "WHERE s.branch_id = ? ";
        }
        query += "ORDER BY s.roll_no";

        unique_ptr<PreparedStatement> stmt(db->prepareStatement(query));
        if (mode == 1) {
            stmt->setString(1, rollNo);
        } else if (mode == 2) {
            stmt->setInt(1, branchId);
        }

        unique_ptr<ResultSet> rows(stmt->executeQuery());
        crow::json::wvalue::list students;
        size_t count = 0;
        while (rows->next()) {
            students.push_back(studentToJson(rows.get()));
            ++count;
        }

        if (mode == 1 && count == 0) {
            return jsonResponse(404, makeResult(false, "Student not found"));
        }

        crow::json::wvalue body;
        body["success"] = true;
        body["students"] = std::move(students);
        return jsonResponse(200, std::move(body));
    } catch (const SQLException& e) {
        cerr << "Student query failed: " << e.what() << '\n';
        return jsonResponse(500, makeResult(false, "Database error"));
    } catch (const exception& e) {
        cerr << "Student query failed: " << e.what() << '\n';
        return jsonResponse(500, makeResult(false, "Internal server error"));
    }
}

CROW_ROUTE(app, "/register").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto body = crow::json::load(req.body);
    if (!body || !body.has("username") || !body.has("password") || !body.has("role")) {
        return jsonResponse(400, makeResult(false, "Username, password, and role are required"));
    }

    try {
        string username = body["username"].s();
        string password = body["password"].s();
        string role = body["role"].s();

        if (username.empty() || password.empty()) {
            return jsonResponse(400, makeResult(false, "Username and password cannot be empty"));
        }
        if (role != "Admin" && role != "Invigilator" && role != "Student") {
            return jsonResponse(400, makeResult(false, "Invalid role"));
        }

        string passwordHash = BCrypt::generateHash(password);
        auto db = connectDB();
        unique_ptr<PreparedStatement> stmt(db->prepareStatement(
            "INSERT INTO `USER` (username, password_hash, role) VALUES (?, ?, ?)"
        ));
        stmt->setString(1, username);
        stmt->setString(2, passwordHash);
        stmt->setString(3, role);
        stmt->executeUpdate();

        unique_ptr<PreparedStatement> idStmt(db->prepareStatement("SELECT LAST_INSERT_ID()"));
        unique_ptr<ResultSet> idResult(idStmt->executeQuery());
        int userId = idResult->next() ? idResult->getInt(1) : -1;

        crow::json::wvalue result = makeResult(true, "User created successfully");
        result["user_id"] = userId;
        return jsonResponse(201, std::move(result));
    } catch (const SQLException& e) {
        cerr << "Registration failed: " << e.what() << '\n';
        if (e.getErrorCode() == 1062) {
            return jsonResponse(409, makeResult(false, "Username already exists"));
        }
        return jsonResponse(500, makeResult(false, "Database error"));
    } catch (const exception& e) {
        cerr << "Registration failed: " << e.what() << '\n';
        return jsonResponse(500, makeResult(false, "Registration failed"));
    }
});

CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto body = crow::json::load(req.body);
    if (!body || !body.has("username") || !body.has("password")) {
        return jsonResponse(400, makeResult(false, "Username and password are required"));
    }

    try {
        string username = body["username"].s();
        string password = body["password"].s();
        if (username.empty() || password.empty()) {
            return jsonResponse(400, makeResult(false, "Username and password cannot be empty"));
        }

        auto db = connectDB();
        unique_ptr<PreparedStatement> stmt(db->prepareStatement(
            "SELECT user_id, username, password_hash, role FROM `USER` WHERE username = ?"
        ));
        stmt->setString(1, username);
        unique_ptr<ResultSet> row(stmt->executeQuery());

        if (!row->next() || !BCrypt::validatePassword(password, row->getString("password_hash"))) {
            return jsonResponse(401, makeResult(false, "Invalid username or password"));
        }

        crow::json::wvalue result = makeResult(true, "Login successful");
        result["user_id"] = row->getInt("user_id");
        result["username"] = row->getString("username");
        result["role"] = row->getString("role");
        return jsonResponse(200, std::move(result));
    } catch (const exception& e) {
        cerr << "Login failed: " << e.what() << '\n';
        return jsonResponse(500, makeResult(false, "Login failed"));
    }
});

CROW_ROUTE(app, "/import/students").methods(crow::HTTPMethod::POST)
([](const crow::request&) {
    return jsonResponse(501, makeResult(false,
        "Excel upload is not wired into the server yet; run importStudents.exe separately"));
});

CROW_ROUTE(app, "/import/rooms").methods(crow::HTTPMethod::POST)
([](const crow::request&) {
    return jsonResponse(501, makeResult(false,
        "Excel upload is not wired into the server yet; run importRooms.exe separately"));
});

CROW_ROUTE(app, "/students")
([] {
    return listStudents(0);
});

CROW_ROUTE(app, "/students/branch/<int>")
([](int branchId) {
    if (branchId <= 0) {
        return jsonResponse(400, makeResult(false, "Branch ID must be positive"));
    }
    return listStudents(2, "", branchId);
});

CROW_ROUTE(app, "/students/<string>")
([](const string& rollNo) {
    return listStudents(1, rollNo);
});

CROW_ROUTE(app, "/students/<string>")
.methods(crow::HTTPMethod::DELETE)
([](const string& rollNo) {
    try {
        auto db = connectDB();
        unique_ptr<PreparedStatement> stmt(db->prepareStatement(
            "DELETE FROM STUDENT WHERE roll_no = ?"
        ));
        stmt->setString(1, rollNo);
        int affected = stmt->executeUpdate();
        if (affected == 0) {
            return jsonResponse(404, makeResult(false, "Student not found"));
        }
        return jsonResponse(200, makeResult(true, "Student deleted successfully"));
    } catch (const SQLException& e) {
        cerr << "Student deletion failed: " << e.what() << '\n';
        return jsonResponse(500, makeResult(false, "Database error"));
    }
});

CROW_ROUTE(app, "/allocation/start").methods(crow::HTTPMethod::POST)
([] {
    return startAllocation();
});

CROW_ROUTE(app, "/allocation/stop").methods(crow::HTTPMethod::POST)
([] {
    return stopAllocation();
});

CROW_ROUTE(app, "/allocation/status")
([] {
    lock_guard<mutex> lock(processMutex);
    refreshAllocationStatusLocked();
    crow::json::wvalue result;
    result["success"] = true;
    result["status"] = processStatus;
    return jsonResponse(200, std::move(result));
});

int main() {
    app.port(3000).multithreaded().run();
    return 0;
}
