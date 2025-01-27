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

int poczekalnia_id, fotele_id, fryzjer_signal_id;
int msg_queue_id; // Define msg_queue_id
struct Queue queue = { .head = 0, .tail = 0, .size = 0 };
struct Kasa* kasa;
int shm_id; //id pam dz
int continueFlag = 0;

/* funkcje do kolejki komunikatow, 
   funckje do pamieci dzielonej
    funkcje do semaforow */


// Funkcja do generowania znacznika czasu
const char* get_timestamp() {
    static char buffer[80];
    struct timeval now;
    struct tm *timeinfo;

    gettimeofday(&now, NULL);
    timeinfo = localtime(&now.tv_sec);

    // Format the time with milliseconds
    strftime(buffer, sizeof(buffer), "[%Y-%m-%d %H:%M:%S", timeinfo);
    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), ".%03ld]", now.tv_usec / 1000);

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
    int shm_id = shmget(key, sizeof(struct Kasa), IPC_CREAT | 0600);
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

void decreaseSemaphoreNowait(int sem_id, int a) {
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

void init_resources() {
    

    

    kasa->salon_open = 1;
    kasa->tens = 10;
    kasa->twenties = 5;
    kasa->fifties = 2;
    kasa->continueFlag = 0;

    for (int i = 0; i < NUM_KLIENTOW; i++) {
        kasa->client_done[i] = 0;
    }    

    for (int i = 0; i < NUM_FOTELE; i++) {
        kasa->client_on_chair[i] = 0;  // Inicjalizacja pól client_on_chair
    }

    poczekalnia_id = semget(POCZEKALNIA_KEY, 1, IPC_CREAT | 0666);
    fotele_id = semget(FOTELE_KEY, 1, IPC_CREAT | 0666);
    fryzjer_signal_id = semget(FRYZJER_SIGNAL_KEY, 1, IPC_CREAT | 0666);
    
    

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
    

    if (semctl(poczekalnia_id, 0, IPC_RMID) == -1) {
        perror("Błąd usuwania semafora poczekalni");
    }

    if (semctl(fotele_id, 0, IPC_RMID) == -1) {
        perror("Błąd usuwania semafora foteli");
    }

    if (semctl(fryzjer_signal_id, 0, IPC_RMID) == -1) {
        perror("Błąd usuwania semafora fryzjera");
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

void close_salon(pid_t fryzjer_pids[], pid_t klient_pids[]) {
    printf("%s [SYSTEM] Godzina 18:00. Zamykanie salonu.\n", get_timestamp());
    
    semctl(SEM_KEY, 0, SETVAL, 0);
    cleanup_resources();
    init_resources();
    // Ustaw flagę salon_open na 0
    lock_semaphore();
    kasa->continueFlag = 1;
    unlock_semaphore();

    // Wyślij sygnał SIGUSR1 do wszystkich fryzjerów, aby zakończyli pracę
    for (int i = 0; i < NUM_FRYZJEROW; i++) {
        kill(fryzjer_pids[i], SIGUSR1);
    }

    // Wyślij sygnał SIGUSR2 do wszystkich klientów, aby zakończyli pracę
    for (int i = 0; i < NUM_KLIENTOW; i++) {
        kill(klient_pids[i], SIGUSR2);
    }

    // Poczekaj na zakończenie wszystkich procesów
    for (int i = 0; i < NUM_FRYZJEROW; i++) {
        waitpid(fryzjer_pids[i], NULL, WNOHANG); 
    }

    for (int i = 0; i < NUM_KLIENTOW; i++) {
        waitpid(klient_pids[i], NULL, WNOHANG); 
    }

    printf("%s [SYSTEM] Salon zamknięty.\n", get_timestamp());
}

void open_salon(pid_t fryzjer_pids[], pid_t klient_pids[]) {
    printf("%s [SYSTEM] Godzina 8:00. Otwieranie salonu.\n", get_timestamp());

    // Ustaw flagę salon_open na 1
    lock_semaphore();
    kasa->salon_open = 1;
    kasa->continueFlag = 0;
    unlock_semaphore();

    //  //Tworzenie procesów fryzjerów
    // for (int i = 0; i < NUM_FRYZJEROW; i++) {
    //     fryzjer_pids[i] = fork();
    //     if (fryzjer_pids[i] == 0) {
    //         fryzjer_handler(i + 1);
    //         exit(0);
    //     }
    // }

    // Tworzenie procesu kierownika
    /*pid_t kierownik_pid = fork();
    if (kierownik_pid == 0) {
        kierownik_handler(fryzjer_pids);
        exit(0);
    }*/

    // // Tworzenie procesów klientów
    // for (int i = 0; i < NUM_KLIENTOW; i++) {
    //     klient_pids[i] = fork();
    //     if (klient_pids[i] == 0) {
    //         klient_handler(i + 1);
    //         exit(0);
    //     }
    //     usleep(100000); // Opóźnienie między klientami
    // }
   printf("%s [SYSTEM] Salon otwarty. Procesy wznawiają działanie.\n", get_timestamp());
}
