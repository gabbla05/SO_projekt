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

//SEMAFORY
int poczekalnia_id, fotele_id, fryzjer_signal_id; 

//KOLEJKA KOMUNIKATOW
int msg_queue_id; //kolejka komunikatow

// KOLEJKA I PAMIEC WSPOLDZIELONA
struct Queue queue = { .head = 0, .tail = 0, .size = 0 };
struct SharedMemory* sharedMemory;

// FLAGI
int shm_id;
int continueFlag = 0;
int salonClosed = 0;

// Funkcja generujaca znacznik czasu
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

//SEMAFOR PAMIECI WSPOLDZIELONEJ - FUNKCJE
void lock_semaphore() {
    struct sembuf sops = {0, -1, 0};
    semop(SEM_KEY, &sops, 1);
}

void unlock_semaphore() {
    struct sembuf sops = {0, 1, 0};
    semop(SEM_KEY, &sops, 1);
}

// FUNKCJE OBSLUGUJACE STRUKTURE KOLEJKI - DO POCZEKALNI
int enqueue(int client_id) {
    lock_semaphore();
    if (queue.size >= MAX_QUEUE_SIZE) {
        unlock_semaphore();
        return -1; // Kolejka pełna
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
        return -1; // Kolejka pusta
    }

    int client_id = queue.clients[queue.head];
    queue.head = (queue.head + 1) % MAX_QUEUE_SIZE;
    queue.size--;
    unlock_semaphore();
    return client_id;
}

// FUNKCJE OBSLUGUJACE PLATNOSC
void process_payment(int tens, int twenties, int fifties) {
    lock_semaphore();
    sharedMemory->tens += tens;
    sharedMemory->twenties += twenties;
    sharedMemory->fifties += fifties;
    unlock_semaphore();
}

void give_change(int amount) {
    lock_semaphore();
    while (amount > 0) {
        if (amount >= 50 && sharedMemory->fifties > 0) {
            sharedMemory->fifties--;
            amount -= 50;
        } else if (amount >= 20 && sharedMemory->twenties > 0) {
            sharedMemory->twenties--;
            amount -= 20;
        } else if (amount >= 10 && sharedMemory->tens > 0) {
            sharedMemory->tens--;
            amount -= 10;
        } else {
            break;
        }
    }
    unlock_semaphore();
}

// INICJALIZACJA ZASOBOW DLA KAZDEGO DNIA ORAZ ICH CZYSZCZENIE
// utils.c - zaktualizowane funkcje zarządzania zasobami

// Funkcja inicjalizująca pojedynczy semafor
int initialize_semaphore(int key, int initial_value) {
    int sem_id = semget(key, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("Błąd tworzenia semafora");
        return -1;
    }
    
    if (semctl(sem_id, 0, SETVAL, initial_value) == -1) {
        perror("Błąd inicjalizacji semafora");
        return -1;
    }
    
    return sem_id;
}

// Funkcja inicjalizująca pamięć współdzieloną
struct SharedMemory* initialize_shared_memory() {
    // Utworzenie segmentu pamięci współdzielonej
    shm_id = shmget(SHM_KEY, sizeof(struct SharedMemory), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Błąd tworzenia pamięci współdzielonej");
        return NULL;
    }

    // Przyłączenie segmentu
    struct SharedMemory* memory = (struct SharedMemory*)shmat(shm_id, NULL, 0);
    if (memory == (void*)-1) {
        perror("Błąd dołączania pamięci współdzielonej");
        return NULL;
    }

    // Inicjalizacja wartości początkowych
    memory->salon_open = 1;
    memory->tens = 10;      // Początkowa ilość 10 zł
    memory->twenties = 5;   // Początkowa ilość 20 zł
    memory->fifties = 2;    // Początkowa ilość 50 zł

    // Czyszczenie tablic statusu
    memset(memory->client_done, 0, sizeof(memory->client_done));
    memset(memory->client_on_chair, 0, sizeof(memory->client_on_chair));

    return memory;
}

// Funkcja inicjalizująca kolejkę komunikatów
int initialize_message_queue() {
    int queue_id = msgget(MSG_QUEUE_KEY, IPC_CREAT | 0666);
    if (queue_id == -1) {
        perror("Błąd tworzenia kolejki komunikatów");
        return -1;
    }
    return queue_id;
}

// Główna funkcja inicjalizująca wszystkie zasoby
int init_resources() {
    printf("%s [SYSTEM] Inicjalizacja zasobów systemowych...\n", get_timestamp());

    // Inicjalizacja semaforów
    poczekalnia_id = initialize_semaphore(POCZEKALNIA_KEY, MAX_QUEUE_SIZE);
    fotele_id = initialize_semaphore(FOTELE_KEY, NUM_FOTELE);
    fryzjer_signal_id = initialize_semaphore(FRYZJER_SIGNAL_KEY, 0);

    if (poczekalnia_id == -1 || fotele_id == -1 || fryzjer_signal_id == -1) {
        printf("%s [SYSTEM] Błąd podczas inicjalizacji semaforów\n", get_timestamp());
        cleanup_resources();
        return 0;
    }

    // Inicjalizacja pamięci współdzielonej
    sharedMemory = initialize_shared_memory();
    if (sharedMemory == NULL) {
        printf("%s [SYSTEM] Błąd podczas inicjalizacji pamięci współdzielonej\n", get_timestamp());
        cleanup_resources();
        return 0;
    }

    // Inicjalizacja kolejki komunikatów
    msg_queue_id = initialize_message_queue();
    if (msg_queue_id == -1) {
        printf("%s [SYSTEM] Błąd podczas inicjalizacji kolejki komunikatów\n", get_timestamp());
        cleanup_resources();
        return 0;
    }

    printf("%s [SYSTEM] Zasoby zostały zainicjalizowane pomyślnie\n", get_timestamp());
    return 1;
}

// Funkcja czyszcząca kolejkę komunikatów
void clear_message_queue() {
    struct Message message;
    while (msgrcv(msg_queue_id, &message, sizeof(message) - sizeof(long), 0, IPC_NOWAIT) != -1) {
        // Kontynuuj usuwanie wiadomości
    }
}

// Funkcja do odłączania pamięci współdzielonej
void detach_shared_memory() {
    if (sharedMemory != NULL && sharedMemory != (void*)-1) {
        if (shmdt(sharedMemory) == -1) {
            perror("Błąd odłączania pamięci współdzielonej");
        }
        sharedMemory = NULL;
    }
}

// Funkcja czyszcząca zasoby na koniec dnia (bez usuwania IPC)
void cleanup_daily_resources() {
    printf("%s [SYSTEM] Czyszczenie zasobów dziennych...\n", get_timestamp());

    // Czyszczenie kolejki komunikatów
    clear_message_queue();

    // Reset pamięci współdzielonej
    if (sharedMemory != NULL) {
        sharedMemory->salon_open = 0;
        memset(sharedMemory->client_done, 0, sizeof(sharedMemory->client_done));
        memset(sharedMemory->client_on_chair, 0, sizeof(sharedMemory->client_on_chair));
    }

    // Reset semaforów do wartości początkowych
    semctl(poczekalnia_id, 0, SETVAL, MAX_QUEUE_SIZE);
    semctl(fotele_id, 0, SETVAL, NUM_FOTELE);
    semctl(fryzjer_signal_id, 0, SETVAL, 0);

    printf("%s [SYSTEM] Zasoby dzienne zostały wyczyszczone\n", get_timestamp());
}

// Funkcja usuwająca wszystkie zasoby IPC
void cleanup_resources() {
    printf("%s [SYSTEM] Usuwanie wszystkich zasobów IPC...\n", get_timestamp());

    // Czyszczenie kolejki komunikatów
    if (msg_queue_id != -1) {
        clear_message_queue();
        if (msgctl(msg_queue_id, IPC_RMID, NULL) == -1) {
            perror("Błąd usuwania kolejki komunikatów");
        }
    }

    // Odłączenie i usunięcie pamięci współdzielonej
    detach_shared_memory();
    if (shm_id != -1) {
        if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
            perror("Błąd usuwania pamięci współdzielonej");
        }
    }

    // Usunięcie semaforów
    if (poczekalnia_id != -1) {
        if (semctl(poczekalnia_id, 0, IPC_RMID) == -1) {
            perror("Błąd usuwania semafora poczekalni");
        }
    }
    
    if (fotele_id != -1) {
        if (semctl(fotele_id, 0, IPC_RMID) == -1) {
            perror("Błąd usuwania semafora foteli");
        }
    }
    
    if (fryzjer_signal_id != -1) {
        if (semctl(fryzjer_signal_id, 0, IPC_RMID) == -1) {
            perror("Błąd usuwania semafora sygnałów fryzjera");
        }
    }

    printf("%s [SYSTEM] Wszystkie zasoby IPC zostały usunięte\n", get_timestamp());
}