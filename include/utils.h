#ifndef UTILS_H
#define UTILS_H

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#define _XOPEN_SOURCE 500

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>

// Stałe
#define QUEUE_KEY 1234
#define SHM_KEY 5678
#define SEM_KEY 91011
#define MAX_QUEUE_SIZE 10
#define NUM_FRYZJEROW 3
#define NUM_KLIENTOW 10
#define POCZEKALNIA_KEY 12345
#define FOTELE_KEY 12346
#define MAX_FOTELE 2
#define FRYZJER_SIGNAL_KEY 12347
#define SLEEPING_COUNT_KEY 12348

extern pid_t main_process_pid; // Deklaracja zmiennej
extern int shm_id;
extern int msg_queue_id;
extern int poczekalnia_id; // Semafor dla poczekalni
extern int fotele_id;      // Semafor dla foteli
extern int fryzjer_signal_id;
extern int sleeping_count_id;
extern int* sleeping_fryzjer;


typedef struct Queue {
    int clients[MAX_QUEUE_SIZE];
    int head;
    int tail;
    int size;
} Queue;

extern struct Queue queue; 


// Struktura kasy
struct Kasa {
    int tens;
    int twenties;
    int fifties;
    int fryzjer_status[NUM_FRYZJEROW]; 
    int client_done[NUM_KLIENTOW];
    int salon_open;
};
extern struct Kasa* kasa; // Deklaracja wskaźnika

// Struktura wiadomości w kolejce komunikatów
struct Message {
    long msg_type;  // Typ wiadomości
    int client_id;  // ID klienta
};

// Funkcje do obsługi zasobów
void init_resources();
void cleanup_resources();
int enqueue(int client_id);
int dequeue();
void process_payment(int tens, int twenties, int fifties);
void give_change(int amount);
void* reap_zombies(void* arg);
void wait_for_processes();
void lock_semaphore();
void unlock_semaphore();
const char* get_process_role();
void register_signal_handlers();
void sigusr2_handler(int signo);

#endif
