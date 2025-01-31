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
#include <string.h>
#include <sys/time.h>

//12ss

// Add to utils.c
int systemErrorOccurred = 0;
pid_t kierownik_pid = 0;

void triggerSystemShutdown() {
    systemErrorOccurred = 1;
    // Send signal to kierownik to initiate system-wide shutdown
    kill(kierownik_pid, SIGTERM);
}

// FUNKCJE DO KOLEJKI KOMUNIKATOW ===========================================================================
int createMessageQueue(key_t key)
{
    int msg_queue_id;
    if ((msg_queue_id = msgget(key, IPC_CREAT | 0600)) == -1)
    {
        perror("");
        fprintf(stderr,"Nie udalo sie stworzyc kolejki komunikatów. PID: %d\n", getpid());
        exit(EXIT_FAILURE);
    }
    return msg_queue_id;
}

void recieveMessage(int msg_queue_id, struct message *msg, long receiver)
{
    int result = msgrcv(msg_queue_id, (struct msgbuf *)msg, sizeof(struct message) - sizeof(long), receiver, 0);
    if (result == -1)
    {
        if (errno == EINTR) {
            recieveMessage(msg_queue_id, msg, receiver);
        } else {
            perror("");
            fprintf(stderr,"Nie odało się odczytać komunikatu z kolejki komunikatów. PID: %d\n", getpid());
            exit(EXIT_FAILURE);
        }
    }
}

void sendMessage(int msg_queue_id, struct message* msg)
{
    int result = msgsnd(msg_queue_id, (struct msgbuf *)msg, sizeof(struct message) - sizeof(long), 0);
    if (result == -1)
    {
        if (errno == EINTR) {
            sendMessage(msg_queue_id, msg);
        } else {
            perror("");
            fprintf(stderr,"Nie udało się dodać komunikatu do kolejki komunikatów. PID: %d\n", getpid());
            exit(EXIT_FAILURE);
        }
    }
}

void deleteMessageQueue(int msg_queue_id) {
    if (msgctl(msg_queue_id, IPC_RMID, NULL) == -1)
    {
        perror("");
        fprintf(stderr,"Nie odało się usunąć kolejki komunikatów. PID: %d\n", getpid());
        exit(EXIT_FAILURE);
    }
    
}

// FUNKCJE DO PAMIECI WSPOLDZIELONEJ ======================================================================
int createSharedMemory(key_t key)
{
    int shm_id = shmget(key, sizeof(int[MONEY]), 0600 | IPC_CREAT);
    if (shm_id == -1)
    {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    return shm_id;
}

int *attachSharedMemory(int shm_id)
{
    int *kasa = shmat(shm_id, 0, 0);
    if (*kasa == -1)
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    return kasa;
}

void detachSharedMemory(int *kasa)
{
    int result = shmdt(kasa);
    if (result == -1)
    {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
}

void deleteSharedMemory(int shm_id)
{
    int result = shmctl(shm_id, IPC_RMID, 0);
    if (result == -1)
    {
        perror("shmctl IPC_RMID");
        exit(EXIT_FAILURE);
    }
}

// FUNKCJE DO SEMAFOROW ==================================================================
int createSemaphore(key_t key)
{
    int sem_id = semget(key, 1, 0600 | IPC_CREAT);
    if (sem_id == -1)
    {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    return sem_id;
}

void deleteSemaphore(int sem_id)
{
    int result = semctl(sem_id, 0, IPC_RMID);
    if (result == -1)
    {
        perror("semctl IPC_RMID");
        exit(EXIT_FAILURE);
    }
}

void setSemaphore(int sem_id, int max_value)
{
    int result = semctl(sem_id, 0, SETVAL, max_value);
    if (result == -1)
    {
        perror("semctl SETVAL");
        exit(EXIT_FAILURE);
    }
}

void decreaseSemaphore(int sem_id, int a)
{
    struct sembuf sem_buff;
    sem_buff.sem_num = 0;
    sem_buff.sem_op = -a;
    sem_buff.sem_flg = 0;
    int res = semop(sem_id, &sem_buff, 1);
    if (res == -1){
    // {
         if (errno == EINTR)
         {
             decreaseSemaphore(sem_id, a);
         }
         else
        {
    //         perror("semop p");
    //         exit(EXIT_FAILURE);
    //     }
     
    perror("semop p");
    triggerSystemShutdown();
    }
    }
}

void increaseSemaphore(int sem_id, int a)
{
    struct sembuf semOperation;
    semOperation.sem_num = 0;
    semOperation.sem_op = a;
    semOperation.sem_flg = 0;
    int result = semop(sem_id, &semOperation, 1);
    if (result == -1){
    // {
    if (errno == EINTR)
        {
             increaseSemaphore(sem_id, a);
         }
         else
         {
    //         perror("semop v");
    //         exit(EXIT_FAILURE);
    //     }
    // }
     perror("semop v");
     triggerSystemShutdown();
         }
    }
}

int decreaseSemaphoreNowait(int sem_id, int a)
{
    struct sembuf semOperation;
    semOperation.sem_num = 0;
    semOperation.sem_op = -a;
    semOperation.sem_flg = IPC_NOWAIT;
    int result = semop(sem_id, &semOperation, 1);
    if (result == -1)
    {
         if (errno == EAGAIN)
         {
             return 1;
         }
         else if (errno == EINTR)
        {
             decreaseSemaphoreNowait(sem_id, a);
         }
         else
         {
        //     perror("semop p nowait");
        //     exit(EXIT_FAILURE);
        // }
        perror("semop p nowait");
        triggerSystemShutdown();
        }
    }
    return 0;
}

int semaphoreValue(int sem_id) {
    int result = semctl(sem_id, 0, GETVAL);
    if (result == -1)
    {
        perror("semctl GETVAL");
         triggerSystemShutdown();
    }
    return result;
}

// Miernik czasu rzeczywistego.
const char* get_timestamp() {
    static char buffer[80];
    struct timeval now;
    struct tm *timeinfo;

    gettimeofday(&now, NULL);
    timeinfo = localtime(&now.tv_sec);

    strftime(buffer, sizeof(buffer), "[%Y-%m-%d %H:%M:%S", timeinfo);
    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), ".%03ld]", now.tv_usec / 1000);

    return buffer;
}