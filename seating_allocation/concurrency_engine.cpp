/**
 * ============================================================================
 * Examination Operations & Intelligent Seating Allocation Platform
 * Module: Seating Allocation Engine (OS Concurrency Suite)
 * Author: Ayush Chauhan (Student 4, Roll No: 2418385)
 * ============================================================================
 * 
 * Operating Systems Concepts Demonstrated:
 *  - Multithreading: Parallel worker threads (std::thread) splitting allocation workloads
 *  - Critical Section: Concurrent read/increment/claim on room seat counters
 *  - Race Condition: Unsynchronized threads causing duplicate seat assignments
 *  - Mutual Exclusion (Mutex): Fine-grained per-room locks (std::mutex, unique_lock)
 *  - Synchronization Barrier: atomic<bool> barrier for simultaneous thread start
 */

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <map>
#include <atomic>

using namespace std;

struct StudentRecord {
    int studentId;
    string rollNo;
    string name;
};

struct RoomRecord {
    int roomId;
    string roomNo;
    int capacity;
    int allocatedCount = 0;
};

struct AllocationRecord {
    int studentId;
    string rollNo;
    string name;
    int roomId;
    string roomNo;
    int seatNo;
    int workerThreadId;
};

// Room-level mutexes for fine-grained mutual exclusion
vector<mutex> roomMutexes(10);
atomic<bool> startSimulationBarrier{false};

vector<StudentRecord> generateMockStudents(int count) {
    vector<StudentRecord> students;
    students.reserve(count);
    for (int i = 1; i <= count; ++i) {
        students.push_back({
            i,
            "2418" + to_string(300 + i),
            "Student_" + to_string(i)
        });
    }
    return students;
}

vector<RoomRecord> generateMockRooms() {
    return {
        {101, "LT-101", 30, 0},
        {102, "LT-102", 30, 0},
        {103, "LT-103", 20, 0}
    };
}

// -----------------------------------------------------------------------------
// Mode 1 Worker: Unprotected Allocation (Demonstrates Race Condition)
// -----------------------------------------------------------------------------
void allocateUnprotectedWorker(
    int workerId,
    const vector<StudentRecord>& assignedStudents,
    vector<RoomRecord>& sharedRooms,
    vector<AllocationRecord>& localAllocations
) {
    // Wait at synchronization barrier so all threads begin simultaneously
    while (!startSimulationBarrier.load(memory_order_relaxed)) {
        this_thread::yield();
    }

    for (const auto& student : assignedStudents) {
        for (auto& room : sharedRooms) {
            // CRITICAL SECTION (UNPROTECTED): Check capacity then increment
            if (room.allocatedCount < room.capacity) {
                int currentOccupied = room.allocatedCount;
                // Realistic context-switch / preemption delay
                this_thread::sleep_for(chrono::microseconds(60));
                int claimedSeat = currentOccupied + 1;
                room.allocatedCount = currentOccupied + 1;

                localAllocations.push_back({
                    student.studentId,
                    student.rollNo,
                    student.name,
                    room.roomId,
                    room.roomNo,
                    claimedSeat,
                    workerId
                });
                break;
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Mode 2 Worker: Synchronized Allocation (Mutex-Protected Critical Section)
// -----------------------------------------------------------------------------
void allocateProtectedWorker(
    int workerId,
    const vector<StudentRecord>& assignedStudents,
    vector<RoomRecord>& sharedRooms,
    vector<AllocationRecord>& localAllocations
) {
    // Wait at synchronization barrier
    while (!startSimulationBarrier.load(memory_order_relaxed)) {
        this_thread::yield();
    }

    for (const auto& student : assignedStudents) {
        bool allocated = false;

        for (size_t rIdx = 0; rIdx < sharedRooms.size(); ++rIdx) {
            // CRITICAL SECTION (PROTECTED): Acquire fine-grained room lock
            unique_lock<mutex> roomLock(roomMutexes[rIdx]);

            if (sharedRooms[rIdx].allocatedCount < sharedRooms[rIdx].capacity) {
                int claimedSeat = ++sharedRooms[rIdx].allocatedCount;
                roomLock.unlock(); // Release lock immediately after claim

                localAllocations.push_back({
                    student.studentId,
                    student.rollNo,
                    student.name,
                    sharedRooms[rIdx].roomId,
                    sharedRooms[rIdx].roomNo,
                    claimedSeat,
                    workerId
                });
                allocated = true;
                break;
            }
        }
    }
}

struct CollisionReport {
    int totalProcessed;
    int totalCollisions;
    map<pair<int, int>, vector<string>> collidedSeats;
};

CollisionReport analyzeResults(const vector<AllocationRecord>& allocations) {
    CollisionReport report;
    report.totalProcessed = static_cast<int>(allocations.size());
    report.totalCollisions = 0;

    map<pair<int, int>, vector<string>> seatMap;
    for (const auto& a : allocations) {
        seatMap[{a.roomId, a.seatNo}].push_back(
            a.rollNo + " (" + a.name + ", Thread-" + to_string(a.workerThreadId) + ")"
        );
    }

    for (const auto& entry : seatMap) {
        if (entry.second.size() > 1) {
            report.totalCollisions += (static_cast<int>(entry.second.size()) - 1);
            report.collidedSeats[entry.first] = entry.second;
        }
    }

    return report;
}

void runUnprotectedDemo() {
    cout << "\n========================================================================\n";
    cout << ">>> MODE 1: UNPROTECTED CONCURRENT ALLOCATION (RACE CONDITION) <<<\n";
    cout << "========================================================================\n";
    cout << "Spawning 4 concurrent worker threads to allocate 80 students WITHOUT mutex locks...\n";

    vector<StudentRecord> students = generateMockStudents(80);
    vector<RoomRecord> rooms = generateMockRooms();
    
    int numThreads = 4;
    int chunkSize = 80 / numThreads;
    vector<thread> workers;
    vector<vector<AllocationRecord>> threadResults(numThreads);

    startSimulationBarrier.store(false);

    for (int i = 0; i < numThreads; ++i) {
        vector<StudentRecord> slice(
            students.begin() + (i * chunkSize),
            students.begin() + ((i + 1) * chunkSize)
        );
        workers.emplace_back(allocateUnprotectedWorker, i + 1, slice, ref(rooms), ref(threadResults[i]));
    }

    auto startTime = chrono::high_resolution_clock::now();
    startSimulationBarrier.store(true);

    for (auto& t : workers) {
        t.join();
    }

    auto endTime = chrono::high_resolution_clock::now();
    double durationMs = chrono::duration<double, milli>(endTime - startTime).count();

    vector<AllocationRecord> allAllocations;
    for (int i = 0; i < numThreads; ++i) {
        allAllocations.insert(allAllocations.end(), threadResults[i].begin(), threadResults[i].end());
    }

    CollisionReport report = analyzeResults(allAllocations);

    cout << "\nExecution Results:\n";
    cout << "  - Total Students Processed: " << report.totalProcessed << "\n";
    cout << "  - Execution Time          : " << fixed << setprecision(2) << durationMs << " ms\n";
    cout << "  - Collisions Detected     : " << report.totalCollisions << " DOUBLE-BOOKED SEATS!\n";

    if (!report.collidedSeats.empty()) {
        cout << "\n[!] Evidence of Race Condition (First 3 Collisions):\n";
        int count = 0;
        for (const auto& col : report.collidedSeats) {
            cout << "  * Room ID " << col.first.first << ", Seat #" << col.first.second 
                 << " was simultaneously claimed by " << col.second.size() << " students:\n";
            for (const auto& s : col.second) {
                cout << "      - " << s << "\n";
            }
            if (++count >= 3) break;
        }
    }
}

void runProtectedDemo() {
    cout << "\n========================================================================\n";
    cout << ">>> MODE 2: SYNCHRONIZED CONCURRENT ALLOCATION (MUTEX PROTECTED) <<<\n";
    cout << "========================================================================\n";
    cout << "Spawning 4 concurrent worker threads to allocate 80 students WITH std::mutex locks...\n";

    vector<StudentRecord> students = generateMockStudents(80);
    vector<RoomRecord> rooms = generateMockRooms();
    
    int numThreads = 4;
    int chunkSize = 80 / numThreads;
    vector<thread> workers;
    vector<vector<AllocationRecord>> threadResults(numThreads);

    startSimulationBarrier.store(false);

    for (int i = 0; i < numThreads; ++i) {
        vector<StudentRecord> slice(
            students.begin() + (i * chunkSize),
            students.begin() + ((i + 1) * chunkSize)
        );
        workers.emplace_back(allocateProtectedWorker, i + 1, slice, ref(rooms), ref(threadResults[i]));
    }

    auto startTime = chrono::high_resolution_clock::now();
    startSimulationBarrier.store(true);

    for (auto& t : workers) {
        t.join();
    }

    auto endTime = chrono::high_resolution_clock::now();
    double durationMs = chrono::duration<double, milli>(endTime - startTime).count();

    vector<AllocationRecord> allAllocations;
    for (int i = 0; i < numThreads; ++i) {
        allAllocations.insert(allAllocations.end(), threadResults[i].begin(), threadResults[i].end());
    }

    CollisionReport report = analyzeResults(allAllocations);

    cout << "\nExecution Results:\n";
    cout << "  - Total Students Processed: " << report.totalProcessed << "\n";
    cout << "  - Execution Time          : " << fixed << setprecision(2) << durationMs << " ms\n";
    cout << "  - Collisions Detected     : " << report.totalCollisions << " CONFLICTS (100% THREAD SAFE)\n";

    cout << "\n[+] Synchronization Result:\n";
    cout << "  Every single student received a strictly unique (room, seat) pair.\n";
    cout << "  Mutual exclusion satisfied: No two threads occupied the critical section together.\n";
}

int main(int argc, char* argv[]) {
    cout << "========================================================================\n";
    cout << "   EXAMINATION ALLOCATION PLATFORM - OS CONCURRENCY SUITE               \n";
    cout << "   Student 4: Ayush Chauhan (Roll No: 2418385)                          \n";
    cout << "========================================================================\n";

    if (argc > 1) {
        string flag = argv[1];
        if (flag == "--race") {
            runUnprotectedDemo();
            return 0;
        } else if (flag == "--safe") {
            runProtectedDemo();
            return 0;
        } else if (flag == "--compare") {
            runUnprotectedDemo();
            runProtectedDemo();
            return 0;
        }
    }

    // Default: Run comparison suite
    runUnprotectedDemo();
    runProtectedDemo();

    return 0;
}
