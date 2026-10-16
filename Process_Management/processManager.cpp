#include <iostream>
#include <string>
#include <windows.h>
#include <thread>
#include <mutex>
#include <atomic>

using namespace std;

PROCESS_INFORMATION allocationProcess{};
HANDLE outputThreadHandle = nullptr;
mutex processMutex;

atomic<bool> processRunning(false);
string processStatus = "NOT_RUNNING";

struct ProcessResult {
    bool success;
    string message;
};

void readOutput(HANDLE pipe, bool isError) {
    char buffer[4096];
    DWORD bytesRead;

    while (ReadFile(pipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr)
           && bytesRead > 0) {
        buffer[bytesRead] = '\0';

        if (isError) {
            cerr << buffer;
        } else {
            cout << buffer;
        }
    }

    CloseHandle(pipe);
}

ProcessResult startAllocation() {
    lock_guard<mutex> lock(processMutex);

    if (processRunning) {
        return {false, "Allocation process is already running"};
    }

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE stdoutRead, stdoutWrite;
    HANDLE stderrRead, stderrWrite;

    if (!CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0)) {
        processStatus = "FAILED";
        return {false, "Failed to create stdout pipe"};
    }

    if (!CreatePipe(&stderrRead, &stderrWrite, &sa, 0)) {
        CloseHandle(stdoutRead);
        CloseHandle(stdoutWrite);
        processStatus = "FAILED";
        return {false, "Failed to create stderr pipe"};
    }

    SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(stderrRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = stdoutWrite;
    si.hStdError = stderrWrite;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi{};

    char command[] = "allocation.exe";

    processStatus = "CREATED";

    BOOL created = CreateProcessA(
        nullptr,
        command,
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    CloseHandle(stdoutWrite);
    CloseHandle(stderrWrite);

    if (!created) {
        CloseHandle(stdoutRead);
        CloseHandle(stderrRead);
        processStatus = "FAILED";
        return {false, "Failed to start allocation.exe"};
    }

    allocationProcess = pi;
    processRunning = true;
    processStatus = "RUNNING";

    thread(readOutput, stdoutRead, false).detach();
    thread(readOutput, stderrRead, true).detach();

    return {true, "Allocation process started"};
}

ProcessResult stopAllocation() {
    lock_guard<mutex> lock(processMutex);

    if (!processRunning) {
        return {false, "No allocation process is running"};
    }

    if (!TerminateProcess(allocationProcess.hProcess, 1)) {
        return {false, "Failed to terminate allocation process"};
    }

    WaitForSingleObject(allocationProcess.hProcess, INFINITE);

    CloseHandle(allocationProcess.hProcess);
    CloseHandle(allocationProcess.hThread);

    allocationProcess = {};
    processRunning = false;
    processStatus = "TERMINATED";

    return {true, "Allocation process terminated"};
}

string getProcessStatus() {
    lock_guard<mutex> lock(processMutex);
    return processStatus;
}

int main() {
    ProcessResult result = startAllocation();

    cout << result.message << '\n';

    if (result.success) {
        cout << "Status: " << getProcessStatus() << '\n';

        cout << "Press Enter to stop the process...";
        cin.get();

        result = stopAllocation();
        cout << result.message << '\n';
        cout << "Status: " << getProcessStatus() << '\n';
    }

    return 0;
}