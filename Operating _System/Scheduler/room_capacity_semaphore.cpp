
#include <iostream>
#include <pthread.h>
#include <semaphore.h>

using namespace std;

sem_t seats;

void* requestSeat(void* arg) {
    int id = *(int*)arg;

    if (sem_trywait(&seats) == 0) {
        cout << "Student " << id << " got a seat\n";
    } else {
        cout << "Student " << id << " denied\n";
    }

    return nullptr;
}

int main() {
    int capacity = 5;
    int students = 10;

    sem_init(&seats, 0, capacity);

    pthread_t threads[10];
    int ids[10];

    for (int i = 0; i < students; i++) {
        ids[i] = i + 1;
        pthread_create(&threads[i], nullptr,
                       requestSeat, &ids[i]);
    }

    for (int i = 0; i < students; i++) {
        pthread_join(threads[i], nullptr);
    }

    sem_destroy(&seats);

    return 0;
}
