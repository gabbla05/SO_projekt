#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdatomic.h>

int poczekalnia_id, fotele_id, fryzjer_signal_id;
int msg_queue_id; // Define msg_queue_id
struct Queue queue = { .head = 0, .tail = 0, .size = 0 };
struct Kasa* kasa;
int shm_id; //id pam dz


// Funkcja do generowania znacznika czasu
const char* get_timestamp() {
    static char buffer[80]; // Statyczny bufor, aby zachować wartość po wyjściu z funkcji
    time_t now;
    struct tm *timeinfo;

    time(&now);
    timeinfo = localtime(&now);

    strftime(buffer, sizeof(buffer), "[%Y-%m-%d %H:%M:%S]", timeinfo);
    return buffer;
}

void lock_semaphore() {
    struct sembuf sops = {0, -1, 0};
    semop(SEM_KEY, &sops, 1);
}

void unlock_semaphore() {
    struct sembuf sops = {0, 1, 0};
    semop(SEM_KEY, &sops, 1);
}

void init_resources() {
    shm_id = shmget(SHM_KEY, sizeof(struct Kasa), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Błąd tworzenia pamięci współdzielonej dla kasy.");
        exit(EXIT_FAILURE);
    }

    kasa = shmat(shm_id, NULL, 0);
    if (kasa == (void*)-1) {
        perror("Błąd dołączania pamięci współdzielonej dla kasy");
        exit(EXIT_FAILURE);
    }

    kasa->salon_open = 1;
    kasa->tens = 10;
    kasa->twenties = 5;
    kasa->fifties = 2;
    for (int i = 0; i < NUM_KLIENTOW; i++) {
        kasa->client_done[i] = 0;
    }

    poczekalnia_id = semget(POCZEKALNIA_KEY, 1, IPC_CREAT | 0666);
    fotele_id = semget(FOTELE_KEY, 1, IPC_CREAT | 0666);
    fryzjer_signal_id = semget(FRYZJER_SIGNAL_KEY, 1, IPC_CREAT | 0666);
    msg_queue_id = msgget(MSG_QUEUE_KEY, IPC_CREAT | 0666);

    if (poczekalnia_id == -1 || fotele_id == -1 || fryzjer_signal_id == -1 || msg_queue_id == -1) {
        perror("Błąd tworzenia semaforów lub kolejki komunikatów");
        exit(EXIT_FAILURE);
    }

    semctl(poczekalnia_id, 0, SETVAL, MAX_QUEUE_SIZE);
    semctl(fotele_id, 0, SETVAL, 2);
    semctl(fryzjer_signal_id, 0, SETVAL, 0);
}

void cleanup_resources() {
    printf("[MAIN] Sprzątanie zasobów...\n");
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("Błąd usuwania pamięci współdzielonej");
    }

    if (semctl(poczekalnia_id, 0, IPC_RMID) == -1) {
        perror("Błąd usuwania semafora poczekalni");
    }

    if (semctl(fotele_id, 0, IPC_RMID) == -1) {
        perror("Błąd usuwania semafora foteli");
    }

    if (semctl(fryzjer_signal_id, 0, IPC_RMID) == -1) {
        perror("Błąd usuwania semafora fryzjera");
    }

    if (msgctl(msg_queue_id, IPC_RMID, NULL) == -1) {
        perror("Błąd usuwania kolejki komunikatów");
    }

    printf("[MAIN] Zasoby zostały pomyślnie usunięte.\n");
}

int enqueue(int client_id) {
    lock_semaphore();
    if (queue.size >= MAX_QUEUE_SIZE) {
        unlock_semaphore();
        return -1;
    }

    queue.clients[queue.tail] = client_id;
    queue.tail = (queue.tail + 1) % MAX_QUEUE_SIZE;
    queue.size++;
    unlock_semaphore();
    return 0;
}

int dequeue() {
    lock_semaphore();
    if (queue.size == 0) {
        unlock_semaphore();
        return -1;
    }

    int client_id = queue.clients[queue.head];
    queue.head = (queue.head + 1) % MAX_QUEUE_SIZE;
    queue.size--;
    unlock_semaphore();
    return client_id;
}

void process_payment(int tens, int twenties, int fifties) {
    lock_semaphore();
    kasa->tens += tens;
    kasa->twenties += twenties;
    kasa->fifties += fifties;
    unlock_semaphore();
}

void give_change(int amount) {
    lock_semaphore();
    while (amount > 0) {
        if (amount >= 50 && kasa->fifties > 0) {
            kasa->fifties--;
            amount -= 50;
        } else if (amount >= 20 && kasa->twenties > 0) {
            kasa->twenties--;
            amount -= 20;
        } else if (amount >= 10 && kasa->tens > 0) {
            kasa->tens--;
            amount -= 10;
        } else {
            break;
        }
    }
    unlock_semaphore();
}

