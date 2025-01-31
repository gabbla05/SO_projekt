#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <time.h>

#define NUM_FRYZJEROW 3
#define NUM_KLIENTOW 10
#define MAX_QUEUE_SIZE 3
#define NUM_FOTELE 2  
#define MONEY 3                                         
#define MESSAGE_P 1                 
#define SERVICE_TIME 2 
#define KEY_PATH "/tmp/" 
#define KEY_CHAR_MESSAGEQUEUE 'm' 
#define KEY_CHAR_KASA 'k' 
#define KEY_CHAR_FOTELE 'f' 
#define KEY_CHAR_POCZEKALNIA 'p' 
#define KEY_CHAR_SHAREDMEMORY 'm' 

struct message
{
    unsigned long mtype;
    unsigned long pid;
    unsigned int message[MONEY];
};

int createMessageQueue(key_t key);
void deleteMessageQueue(int msg_queue_id);
void recieveMessage(int msg_queue_id, struct message* msg, long receiver);
void sendMessage(int msg_queue_id, struct message* msg);

int createSharedMemory(key_t key);
int *attachSharedMemory(int shm_id);
void detachSharedMemory(int* kasa); 
void deleteSharedMemory(int shm_id);

int createSemaphore(key_t key);
void deleteSemaphore(int sem_id);
void setSemaphore(int sem_id, int max_value);
void decreaseSemaphore(int sem_id, int a);
void increaseSemaphore(int sem_id, int a);
int decreaseSemaphoreNowait(int sem_id, int a);
int semaphoreValue(int sem_id);

const char* get_timestamp();

#endif