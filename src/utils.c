#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

// Globalna kasa
struct Kasa kasa;

int poczekalnia[MAX_QUEUE_SIZE] = {0}; // Definicja tablicy

sem_t fotele_sem;
int msg_queue_id;

// Inicjalizacja zasobów
void init_resources(int K, int N) {
    // Tworzenie kolejki komunikatów
    msg_queue_id = msgget(QUEUE_KEY, IPC_CREAT | 0666);
    if (msg_queue_id == -1) {
        perror("Błąd tworzenia kolejki komunikatów");
        exit(EXIT_FAILURE);
    }
    printf("Kolejka komunikatów została utworzona (ID: %d)\n", msg_queue_id);

    // Inicjalizacja semafora dla foteli
    if (sem_init(&fotele_sem, 0, N) == -1) {
        perror("Błąd inicjalizacji semafora fotele_sem");
        exit(EXIT_FAILURE);
    }
    printf("Semafor fotele został zainicjalizowany z wartością: %d\n", N);

    // Inicjalizacja kasy
    kasa.tens = 10;
    kasa.twenties = 5;
    kasa.fifties = 2;
    if (pthread_mutex_init(&kasa.lock, NULL) != 0) {
        perror("Błąd inicjalizacji mutexa kasy");
        exit(EXIT_FAILURE);
    }
    printf("Kasa została zainicjalizowana. Początkowy stan:\n");
    printf(" - 10 zł: %d\n", kasa.tens);
    printf(" - 20 zł: %d\n", kasa.twenties);
    printf(" - 50 zł: %d\n", kasa.fifties);
}

// Sprzątanie zasobów
void cleanup_resources() {
    // Usuwanie kolejki komunikatów
    if (msgctl(msg_queue_id, IPC_RMID, NULL) == -1) {
        perror("Błąd usuwania kolejki komunikatów");
    } else {
        printf("Kolejka komunikatów została usunięta\n");
    }

    // Usuwanie semafora dla foteli
    if (sem_destroy(&fotele_sem) == -1) {
        perror("Błąd usuwania semafora fotele_sem");
    } else {
        printf("Semafor fotele został usunięty\n");
    }

    // Usuwanie mutexa kasy
    if (pthread_mutex_destroy(&kasa.lock) != 0) {
        perror("Błąd usuwania mutexa kasy");
    }
}

// Proces płatności
int process_payment(int tens, int twenties, int fifties) {
    pthread_mutex_lock(&kasa.lock);

    kasa.tens += tens;
    kasa.twenties += twenties;
    kasa.fifties += fifties;

    printf("Dodano do kasy: %d x 10 zł, %d x 20 zł, %d x 50 zł\n", tens, twenties, fifties);
    printf("Nowy stan kasy: %d x 10 zł, %d x 20 zł, %d x 50 zł\n",
           kasa.tens, kasa.twenties, kasa.fifties);

    pthread_mutex_unlock(&kasa.lock);
    return 0;
}

// Wydawanie reszty z oczekiwaniem na nominały
int give_change(int amount) {
    pthread_mutex_lock(&kasa.lock);

    int change = amount;
    int fifties_used = 0, twenties_used = 0, tens_used = 0;

    // Wydawanie reszty z jak najmniejszą liczbą banknotów
    while (change > 0) {
        if (change >= 50 && kasa.fifties > 0) {
            kasa.fifties--;
            change -= 50;
            fifties_used++;
        } else if (change >= 20 && kasa.twenties > 0) {
            kasa.twenties--;
            change -= 20;
            twenties_used++;
        } else if (change >= 10 && kasa.tens > 0) {
            kasa.tens--;
            change -= 10;
            tens_used++;
        } else {
            printf("Brak odpowiednich nominałów do wydania reszty %d zł. Czekanie na uzupełnienie kasy...\n", amount);
            pthread_mutex_unlock(&kasa.lock);
            usleep(500000); // Oczekiwanie przed ponowną próbą
            pthread_mutex_lock(&kasa.lock);
        }
    }

    pthread_mutex_unlock(&kasa.lock);

    printf("Reszta %d zł została wydana: %d x 50 zł, %d x 20 zł, %d x 10 zł.\n",
           amount, fifties_used, twenties_used, tens_used);
    return 0;
}

// Dodanie klienta do kolejki komunikatów
void enqueue(int client_id) {
    struct Message msg;
    msg.msg_type = 1; // Typ wiadomości: 1 dla klientów
    msg.client_id = client_id;

    if (msgsnd(msg_queue_id, &msg, sizeof(msg.client_id), 0) == -1) {
        perror("Błąd dodawania klienta do kolejki");
    } else {
        printf("Klient %d został dodany do poczekalni\n", client_id);
    }
}

// Pobranie klienta z kolejki komunikatów
int dequeue() {
    struct Message msg;

    if (msgrcv(msg_queue_id, &msg, sizeof(msg.client_id), 1, IPC_NOWAIT) == -1) {
        if (errno == ENOMSG) {
            // Brak klientów w kolejce
            return -1;
        } else {
            perror("Błąd pobierania klienta z kolejki");
            return -1;
        }
    } else {
        printf("Klient %d został pobrany z poczekalni\n", msg.client_id);
        return msg.client_id;
    }
}