#ifndef UTILS_H
#define UTILS_H

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define NUM_FRYZJEROW 3
#define NUM_KLIENTOW 10
#define MAX_QUEUE_SIZE 3
#define NUM_FOTELE 2
#define SHM_KEY 153271
#define SEM_KEY 5678
#define POCZEKALNIA_KEY 91011
#define FOTELE_KEY 121314
#define FRYZJER_SIGNAL_KEY 151617
#define MSG_QUEUE_KEY 181920


extern int poczekalnia_id, fotele_id, fryzjer_signal_id;
extern int msg_queue_id;
extern struct Queue queue;
extern struct Kasa* kasa;
extern int continueFlag;


struct Kasa {
    int salon_open;
    int tens;
    int twenties;
    int fifties;
    char client_done[NUM_KLIENTOW];
    int client_on_chair[NUM_FOTELE];
    int continueFlag;
};

struct Queue {
    int clients[MAX_QUEUE_SIZE];
    int head;
    int tail;
    int size;
};

struct message {
    unsigned long mtype; 
    unsigned long pid;
};

int createMessageQueue(key_t key);
void deleteMessageQueue(int msg_queue_id);
void recieveMessage(int msg_queue_id, struct message *msg, long receiver);
void sendMessage(int msg_queue_id, struct message* msg);

int createSharedMemory(key_t key);
int *attachSharedMemory(int shm_id);
void detachSharedMemory(int *kasa);
void deleteSharedMemory(int shm_id);

int createSemaphore(key_t key);
void deleteSemaphore(int sem_id);
void setSemaphore(int sem_id, int max_value);
void increaseSemaphore(int sem_id, int a);
void decreaseSemaphore(int sem_id, int a);
void decreaseSemaphoreNowait(int sem_id, int a);
int semaphoreValue(int sem_id);

const char* get_timestamp();
void lock_semaphore();
void unlock_semaphore();
void init_resources();
void cleanup_resources();
int enqueue(int client_id);
int dequeue();
void process_payment(int tens, int twenties, int fifties);
void give_change(int amount);
void* zombie_collector(void* arg);
void close_salon(pid_t fryzjer_pids[], pid_t klient_pids[]);
void open_salon(pid_t fryzjer_pids[], pid_t klient_pids[]);

#endif