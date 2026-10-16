
#include <iostream>
#include <pthread.h>

using namespace std;

pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;

string attendanceStatus = "NOT_MARKED";

void* updateAttendance(void* arg) {
    const char* status = (const char*)arg;

    pthread_mutex_lock(&lock1);

    cout << "Current Attendance: "
         << attendanceStatus << endl;

    attendanceStatus = status;

    cout << "Updated Attendance: "
         << attendanceStatus << endl;

    pthread_mutex_unlock(&lock1);

    return nullptr;
}

int main() {
    pthread_t t1, t2;

    cout << "Attendance Management System" << endl;
    cout << "Starting attendance updates...\n" << endl;

    pthread_create(&t1, nullptr, updateAttendance,
                   (void*)"PRESENT");

    pthread_create(&t2, nullptr, updateAttendance,
                   (void*)"ABSENT");

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    cout << "\nFinal Attendance: "
         << attendanceStatus << endl;

    pthread_mutex_destroy(&lock1);

    return 0;
}
