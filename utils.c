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


// FUNKCJE DO KOLEJKI KOMUNIKATOW ===========================================================================
int createMessageQueue(key_t key) {
    int msg_queue_id;

    if((msg_queue_id = msgget(key, IPC_CREAT | 0600)) == -1) {
        perror("Blad tworzenia kolejki komunikatow.");
        exit(EXIT_FAILURE);
    }
    return msg_queue_id;
}

void deleteMessageQueue(int msg_queue_id) {
    if (msgctl(msg_queue_id, IPC_RMID, NULL) == -1) {
        perror("Blad usuwania kolejki komunikatow.");
    }
}

void recieveMessage(int msg_queue_id, struct message *msg, long receiver) {
    int result = msgrcv(msg_queue_id, (struct msgbuf *)msg, sizeof(struct message) - sizeof(long), receiver, 0);
    if (result == -1) { 
        if(errno == EINTR) {
            recieveMessage(msg_queue_id, msg, receiver);
        } else {
            perror("Blad odczytywania komunikatu z kolejki komunikatow.");
            exit(EXIT_FAILURE);
        }
    }  
}

void sendMessage(int msg_queue_id, struct message* msg) {
    int result = msgsnd(msg_queue_id, (struct msgbuf *)msg, sizeof(struct message) - sizeof(long), 0);
    if (result == -1) { 
        if(errno == EINTR) {
            sendMessage(msg_queue_id, msg);
        } else {
            perror("Blad wysylania komunikatu do kolejki komunikatow.");
            exit(EXIT_FAILURE);
        }
    }  
}

// FUNKCJE DO PAMIECI WSPOLDZIELONEJ ======================================================================
int createSharedMemory(key_t key) {
    int shm_id = shmget(key, sizeof(int[3]), IPC_CREAT | 0600);
    if (shm_id == -1) {
        perror("Blad tworzenia pamieci wspoldzielonej dla kasy.");
        exit(EXIT_FAILURE);
    }
    return shm_id;
}

int *attachSharedMemory(int shm_id) {
    int *kasa = shmat(shm_id, NULL, 0);
    if (kasa == (void*)-1) {
        perror("Blad dolaczania pamieci wspoldzielonej dla kasy");
        exit(EXIT_FAILURE);
    }
    return kasa;
}

void detachSharedMemory(int *kasa) {
    if (shmdt(kasa) == -1) {
        perror("Blad odlaczania pamieci wspoldzielonej dla kasy");
        exit(EXIT_FAILURE);
    }
}

void deleteSharedMemory(int shm_id) {
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("Błąd usuwania pamięci współdzielonej dla kasy");
        exit(EXIT_FAILURE);
    }
}

// FUNKCJE DO SEMAFOROW ==================================================================
int createSemaphore(key_t key) {
    int sem_id = semget(key, 1, 0600 | IPC_CREAT);
    if (sem_id == -1) {
        perror("Blad tworzenia semafora.");
        exit(EXIT_FAILURE);
    }
    return sem_id;
}

void deleteSemaphore(int sem_id) {
    int result = semctl(sem_id, 0, IPC_RMID);
    if (result == -1) {
        perror("Blad usuwania semafora.");
        exit(EXIT_FAILURE);
    }
}

void setSemaphore(int sem_id, int max_value) {
    int result = semctl(sem_id, 0, SETVAL, max_value);
    if(result == -1) {
        perror("Blad ustawiania semafora.");
        exit(EXIT_FAILURE);
    }
}

void increaseSemaphore(int sem_id, int a) {
    struct sembuf semOperation;
    semOperation.sem_num = 0;
    semOperation.sem_op = a;
    semOperation.sem_flg = 0;
    int result = semop(sem_id, &semOperation, 1);
    if (result == -1) {
        if(errno == EINTR) {
            increaseSemaphore(sem_id, a);
        } else {
            perror("Blad zwiekszania wartosci na semaforze.");
            exit(EXIT_FAILURE);
        }
    }
}

void decreaseSemaphore(int sem_id, int a) {
    struct sembuf semOperation;
    semOperation.sem_num = 0;
    semOperation.sem_op = -a;
    semOperation.sem_flg = 0;
    int result = semop(sem_id, &semOperation, 1);
    if (result == -1) {
        if(errno == EINTR) {
            increaseSemaphore(sem_id, a);
        } else {
            perror("Blad zmniejszania wartosci na semaforze.");
            exit(EXIT_FAILURE);
        }
    }
}

int decreaseSemaphoreNowait(int sem_id, int a) {
    struct sembuf semOperation;
    semOperation.sem_num = 0;
    semOperation.sem_op = -a;
    semOperation.sem_flg = IPC_NOWAIT;
    int result = semop(sem_id, &semOperation, 1);
    if (result == -1) {
        if(errno == EINTR) {
            decreaseSemaphoreNowait(sem_id, a);
        } else if (errno == EAGAIN) {
            return 1;
        } else {
            perror("Blad zmniejszania wartosci na semaforze(nowait).");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
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










