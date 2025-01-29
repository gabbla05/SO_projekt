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
#define MONEY 3
#define MESSAGE_P 1
#define SERVICE_TIME 2
#define KEY_PATH "/tmp"  // Ścieżka do pliku, który będzie używany do generowania kluczy
#define KEY_CHAR_MESSAGEQUEUE 'M'  // Znak do identyfikacji kolejki komunikatów
#define KEY_CHAR_KASA 'K'          // Znak do identyfikacji semafora kasy
#define KEY_CHAR_FOTELE 'F'        // Znak do identyfikacji semafora foteli
#define KEY_CHAR_POCZEKALNIA 'P'   // Znak do identyfikacji semafora poczekalni
#define KEY_CHAR_SHAREDMEMORY 'S'  // Znak do identyfikacji pamięci współdzielonej

extern int poczekalnia, fotele, kasa;
extern int messageQueue;
extern int sharedMemoryId;
extern int *memory;

extern long client_id;
extern long fryzjer_id;

pthread_t timeSimulation;

struct message {
    unsigned long mtype; 
    unsigned long pid;
    unsigned int message[MONEY];
};
//kolejka komunikatow
int createMessageQueue(key_t key);
void deleteMessageQueue(int msg_queue_id);
void recieveMessage(int msg_queue_id, struct message *msg, long receiver);
void sendMessage(int msg_queue_id, struct message* msg);
//pamiec wspoldzielona dla kasy
int createSharedMemory(key_t key);
int *attachSharedMemory(int shm_id);
void detachSharedMemory(int *kasa);
void deleteSharedMemory(int shm_id);
//semafory
int createSemaphore(key_t key);
void deleteSemaphore(int sem_id);
void setSemaphore(int sem_id, int max_value);
void increaseSemaphore(int sem_id, int a);
void decreaseSemaphore(int sem_id, int a);
int decreaseSemaphoreNowait(int sem_id, int a);
int semaphoreValue(int sem_id);
//czas
void *timeSimulator(void *arg);
void endTimeSimulator();
//zasoby
void initResources();
void initCash(int *memory);
void cleanResources();

void waitProcesses(int a);
void* zombie_collector(void* arg);


#endif