#ifndef UTILS_H
#define UTILS_H

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define NUM_FRYZJEROW 3
#define NUM_KLIENTOW 40
#define MAX_QUEUE_SIZE 10
#define NUM_FOTELE 2
#define SHM_KEY 3000
#define SEM_KEY 5678
#define POCZEKALNIA_KEY 91011
#define FOTELE_KEY 121314
#define FRYZJER_SIGNAL_KEY 151617
#define MSG_QUEUE_KEY 181920

extern int poczekalnia_id, fotele_id, fryzjer_signal_id;
extern int msg_queue_id;
extern struct Queue queue;
extern struct SharedMemory* sharedMemory;
extern int continueFlag;
extern int salonClosed;

struct SharedMemory {
    int salon_open;
    int tens;
    int twenties;
    int fifties;
    char client_done[NUM_KLIENTOW];
    int client_on_chair[NUM_FOTELE];
   // int continueFlag; // Dodane pole do synchronizacji
};

struct Queue {
    int clients[MAX_QUEUE_SIZE];
    int head;
    int tail;
    int size;
};

struct Message {
    long mtype; // Typ komunikatu (identyfikator fryzjera)
    int client_id; // ID klienta
    int client_pid;
};

const char* get_timestamp();
void lock_semaphore();
void unlock_semaphore();
int init_resources();
void cleanup_resources();
int enqueue(int client_id);
int dequeue();
void process_payment(int tens, int twenties, int fifties);
void give_change(int amount);
void* zombie_collector(void* arg);

#endif