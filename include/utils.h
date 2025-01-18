#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

// Stałe dla kolejki komunikatów
#define QUEUE_KEY 1234
#define MAX_CLIENTS 100
#define MAX_QUEUE_SIZE 10
#define NUM_FRYZJEROW 3

extern int poczekalnia[MAX_QUEUE_SIZE];
extern int msg_queue_id; // Identyfikator kolejki komunikatów
extern volatile sig_atomic_t fryzjer_active[NUM_FRYZJEROW];

// Struktura wiadomości w kolejce komunikatów
struct Message {
    long msg_type;  // Typ wiadomości
    int client_id;  // ID klienta
};




// Struktura kasy
struct Kasa {
    int tens;    // Liczba 10-złotówek
    int twenties; // Liczba 20-złotówek
    int fifties;  // Liczba 50-złotówek
    pthread_mutex_t lock; // Mutex do synchronizacji dostępu
};

extern struct Kasa kasa; // Deklaracja globalnej kasy

extern volatile sig_atomic_t fryzjer_active[NUM_FRYZJEROW];

// Semafory
extern sem_t poczekalnia_sem;
extern sem_t fotele_sem;

// Mutexy
extern pthread_mutex_t queue_mutex;

// Deklaracje funkcji
void init_resources(int K, int N);
void cleanup_resources();
void enqueue(int client_id);
int dequeue();
int process_payment(int tens, int twenties, int fifties);
int give_change(int amount);
void handle_signal(int sig);


#endif
