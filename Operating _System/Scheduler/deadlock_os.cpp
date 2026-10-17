
#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

mutex roomMutex;
mutex allocationMutex;

void allocateSeat(int studentId)
{
    // Always lock resources in the same order
    lock_guard<mutex> lock1(roomMutex);
    lock_guard<mutex> lock2(allocationMutex);

    cout << "Student " << studentId
         << " allocated a seat successfully." << endl;
}

void reallocateSeat(int studentId)
{
    // Same locking order prevents circular wait
    lock_guard<mutex> lock1(roomMutex);
    lock_guard<mutex> lock2(allocationMutex);

    cout << "Emergency reallocation completed for Student "
         << studentId << "." << endl;
}

int main()
{
    thread t1(allocateSeat, 101);
    thread t2(reallocateSeat, 102);
    thread t3(allocateSeat, 103);

    t1.join();
    t2.join();
    t3.join();

    cout << "\nAll operations completed safely." << endl;

    return 0;
}
