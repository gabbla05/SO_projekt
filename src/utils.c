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


int poczekalnia, fotele, kasa;
int messageQueue;
int sharedMemoryId;
int *memory;

int hour = 8;


long client_id;

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
}

int *attachSharedMemory(int shm_id) {
    kasa = shmat(shm_id, NULL, 0);
    if (kasa == (void*)-1) {
        perror("Blad dolaczania pamieci wspoldzielonej dla kasy");
        exit(EXIT_FAILURE);
    }
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
            increaseSemaphoreNowait(sem_id, a);
        } else if (errno == EAGAIN) {
            return 1;
        } else {
            perror("Blad zmniejszania wartosci na semaforze(nowait).");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}

int semaphoreValue(int sem_id) {
    int result = semctl(sem_id, 0, GETVAL);
    if (result == -1)
    {
        perror("Blad pobrania wartosci na semaforze.");
        exit(EXIT_FAILURE);
    }
    return result;
}

// SYMULACJA CZASU ==================================================================================

void *timeSimulator(void *arg) {
    while (1) {
        sleep(1);

        hour++;

        if (hour == 8) {
            printf("%s Godzina 8:00. Salon zostal otwarty.\n", get_timestamp());
            increaseSemaphore(poczekalnia, MAX_QUEUE_SIZE);
        }

        if (hour == 18) {
            printf("%s Godzina 18:00. Salon sie zamyka.\n", get_timestamp());
            decreaseSemaphore(poczekalnia, MAX_QUEUE_SIZE);
        }

        if (hour == 24) {
            hour = 0;
        }

        if (hour % 2 == 0) {
            printf("%s Aktualna godzina: %d.\n", hour);
        }

        if(hour % 6 == 0) {
            displayKasa();
        }

    }
    return NULL;
}

void endTimeSimulator() {
    pthread_cancel(timeSimulation);
    pthread_join(timeSimulation, NULL);
}

// TWORZENIE/CZYSZCZENIE ZASOBOW ====================================================================================
void initResources() {
    key_t key;

    key = ftok(KEY_PATH, KEY_CHAR_MESSAGEQUEUE);
    messageQueue = createMessageQueue(key);

    key = ftok(KEY_PATH, KEY_CHAR_KASA);
    kasa = createSemaphore(key);
    setSemaphore(kasa, 1);

    key = ftok(KEY_PATH, KEY_CHAR_FOTELE);
    fotele = createSemaphore(key);
    setSemaphore(fotele, NUM_FOTELE);

    key = ftok(KEY_PATH, KEY_CHAR_POCZEKALNIA);
    poczekalnia = createSemaphore(key);
    setSemaphore(poczekalnia, MAX_QUEUE_SIZE);

    key = ftok(KEY_PATH, KEY_CHAR_SHAREDMEMORY);
    sharedMemoryId = createSharedMemory(key);
    memory = attachSharedMemory(sharedMemoryId);
}

void initCash(int *memory) {
    if (memory == NULL) {
        perror("Pamiec wspoldzielona nie zostala poprawnie zainicjowana.");
        return;
    }
    memory[0] = 10;
    memory[1] = 5;
    memory[2] = 2;
    printf("%s Stan poczatkowy kasy: 10 x 10zl, 5 x 20zl, 2 x 50zl.\n");
}

void cleanResources() {
    deleteMessageQueue(messageQueue);
    deleteSemaphore(kasa);
    deleteSemaphore(fotele);
    deleteSemaphore(poczekalnia);
    detachSharedMemory(memory);
    deleteSharedMemory(memory);
    printf("%s Zasoby zostaly zwolnione.\n", get_timestamp());
}



void waitProcesses(int a) {
    int pid;
    int status;
    for (int i = 0; i < a; i++) {
        pid = wait(&status);
        if(pid == -1) {
            perror("Blad w funkcji wait.");
            exit(EXIT_FAILURE);
        } 
    }
}





