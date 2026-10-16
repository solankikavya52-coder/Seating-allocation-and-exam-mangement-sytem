#include <crow.h>
#include <iostream>
#include <string>
#include <mutex>
#include <windows.h>

using namespace std;

crow::SimpleApp app;

PROCESS_INFORMATION allocationProcess{};
bool allocationRunning = false;
string processStatus = "NOT_RUNNING";
mutex processMutex;

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

crow::response startAllocation() {
    lock_guard<mutex> lock(processMutex);

    if (allocationRunning) {
        return jsonResponse(
            409,
            makeResult(false, "Allocation process is already running")
        );
    }

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    char command[] = "allocation.exe";

    BOOL created = CreateProcessA(
        nullptr,
        command,
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!created) {
        processStatus = "FAILED";
        return jsonResponse(
            500,
            makeResult(false, "Failed to start allocation process")
        );
    }

    allocationProcess = pi;
    allocationRunning = true;
    processStatus = "RUNNING";

    return jsonResponse(
        200,
        makeResult(true, "Allocation process started")
    );
}

crow::response stopAllocation() {
    lock_guard<mutex> lock(processMutex);

    if (!allocationRunning) {
        return jsonResponse(
            404,
            makeResult(false, "No allocation process is running")
        );
    }

    if (!TerminateProcess(allocationProcess.hProcess, 1)) {
        return jsonResponse(
            500,
            makeResult(false, "Failed to terminate allocation process")
        );
    }

    WaitForSingleObject(allocationProcess.hProcess, INFINITE);

    CloseHandle(allocationProcess.hProcess);
    CloseHandle(allocationProcess.hThread);

    allocationProcess = {};
    allocationRunning = false;
    processStatus = "TERMINATED";

    return jsonResponse(
        200,
        makeResult(true, "Allocation process terminated")
    );
}

CROW_ROUTE(app, "/register").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto body = crow::json::load(req.body);

    if (!body || !body.has("username") ||
        !body.has("password") || !body.has("role")) {
        return jsonResponse(
            400,
            makeResult(false, "All fields are required")
        );
    }

    return jsonResponse(
        501,
        makeResult(false, "Connect the C++ user module to enable registration")
    );
});

CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto body = crow::json::load(req.body);

    if (!body || !body.has("username") || !body.has("password")) {
        return jsonResponse(
            400,
            makeResult(false, "Username and password are required")
        );
    }

    return jsonResponse(
        501,
        makeResult(false, "Connect the C++ login module to enable login")
    );
});

CROW_ROUTE(app, "/import/students").methods(crow::HTTPMethod::POST)
([](const crow::request&) {
    return jsonResponse(
        501,
        makeResult(false, "Connect the student import module")
    );
});

CROW_ROUTE(app, "/import/rooms").methods(crow::HTTPMethod::POST)
([](const crow::request&) {
    return jsonResponse(
        501,
        makeResult(false, "Connect the room import module")
    );
});

CROW_ROUTE(app, "/students")
([] {
    crow::json::wvalue result;
    result["success"] = true;
    result["students"] = crow::json::wvalue::list();
    return jsonResponse(200, move(result));
});

CROW_ROUTE(app, "/students/<string>")
([](const string& rollNo) {
    return jsonResponse(
        501,
        makeResult(false, "Connect the student database module")
    );
});

CROW_ROUTE(app, "/students/branch/<int>")
([](int branchId) {
    return jsonResponse(
        501,
        makeResult(false, "Connect the branch query module")
    );
});

CROW_ROUTE(app, "/students/<string>")
.methods(crow::HTTPMethod::DELETE)
([](const string& rollNo) {
    return jsonResponse(
        501,
        makeResult(false, "Connect the student deletion module")
    );
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

    crow::json::wvalue result;
    result["success"] = true;
    result["status"] = processStatus;

    return jsonResponse(200, move(result));
});

int main() {
    app.port(3000).multithreaded().run();
    return 0;
}