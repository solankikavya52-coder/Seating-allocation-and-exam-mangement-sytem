
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
using namespace std;
mutex roomMutex;
struct Allocation
{
    int studentId;
    string studentName;
    int roomId;
    int seatNo;
};
vector<Allocation> allocations = {
    {101, "Student A", 1, 1},
    {102, "Student B", 1, 2},
    {103, "Student C", 2, 1},
    {104, "Student D", 2, 2}
};
// Display all seating allocations
void displayReport()
{
    lock_guard<mutex> lock(roomMutex);
    cout << "\n===== SEATING ALLOCATION REPORT =====\n";
    cout << "Student ID\tName\t\tRoom\tSeat\n";
    for (const auto& a : allocations) {
        cout << a.studentId << "\t\t"
             << a.studentName << "\t"
             << a.roomId << "\t"
             << a.seatNo << "\n";
    }
}
// Search for a student
void searchStudent(int id) 
{
    lock_guard<mutex> lock(roomMutex);
    for (const auto& a : allocations)
{
        if (a.studentId == id) 
{
            cout << "Student: " << a.studentName
                 << ", Room: " << a.roomId
                 << ", Seat: " << a.seatNo << "\n";
            return;
        }
    }
    cout << "Student allocation not found.\n";
}
// Initial emergency reallocation workflow
void reallocateStudent(int studentId, int newRoom, int newSeat) 
{
    lock_guard<mutex> lock(roomMutex);
    // Check if the destination seat is already occupied
    for (const auto& a : allocations) 
{
        if (a.roomId == newRoom && a.seatNo == newSeat) 
{
            cout << "Target seat is already occupied!\n";
            return;
        }
    }
    // Find and update the student's allocation
    for (auto& a : allocations) 
{
        if (a.studentId == studentId)
{
            int oldRoom = a.roomId;
            int oldSeat = a.seatNo;
            a.roomId = newRoom;
            a.seatNo = newSeat;
            cout << "Reallocation successful.\n";
            cout << "Previous: Room " << oldRoom
                 << ", Seat " << oldSeat << "\n";
            cout << "New: Room " << newRoom
                 << ", Seat " << newSeat << "\n";
            return;
        }
    }
    cout << "Student allocation not found.\n";
}
int main() {
    int choice;
    do
{
        cout << "\n===== REPORTS & REALLOCATION =====\n";
        cout << "1. Display Seating Report\n";
        cout << "2. Search Student\n";
        cout << "3. Emergency Reallocation\n";
        cout << "0. Exit\n";
        cout << "Enter choice: ";
        cin >> choice;
        if (choice == 1) 
{
            thread reportThread(displayReport);
            reportThread.join();
        }
        else if (choice == 2) 
{
            int id;
            cout << "Enter Student ID: ";
            cin >> id;
            thread searchThread(searchStudent, id);
            searchThread.join();
        }
        else if (choice == 3) 
{
            int id, room, seat;
            cout << "Student ID: ";
            cin >> id;
            cout << "New Room ID: ";
            cin >> room;
            cout << "New Seat Number: ";
            cin >> seat;
            thread allocationThread(
                reallocateStudent, id, room, seat
            );
            allocationThread.join();
        }
    } while (choice != 0);
    return 0;
}
