#define _POSIX_C_SOURCE 200809L
#include "utils.h"
#include "fryzjer.h"
#include "klient.h"
#include "kierownik.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Liczba fryzjerów i klientów
#define NUM_FRYZJEROW 3
#define NUM_KLIENTOW 10
#define NUM_FOTELI 2

// Flaga otwartości salonu
volatile sig_atomic_t salon_open = 1;

// Funkcja obsługująca sygnały
void handle_signal(int sig) {
    if (sig == SIGUSR1) {
        for (int i = 0; i < NUM_FRYZJEROW; i++) {
            if (fryzjer_active[i]) {
                fryzjer_active[i] = 0; // Wyłącz fryzjera
                printf("Otrzymano SIGUSR1: Wyłączono fryzjera %d.\n", i + 1);
                return;
            }
        }
        printf("Otrzymano SIGUSR1, ale wszyscy fryzjerzy są już wyłączeni.\n");
    } else if (sig == SIGUSR2) {
        printf("Otrzymano SIGUSR2: Natychmiastowe zamknięcie salonu.\n");
        salon_open = 0;
    }
}


// Funkcja główna programu
int main() {
    // Rejestracja obsługi sygnałów
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Błąd rejestracji SIGUSR1");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("Błąd rejestracji SIGUSR2");
        exit(EXIT_FAILURE);
    }

    printf("Obsługa sygnałów została zarejestrowana.\n");

    // Inicjalizacja zasobów
    init_resources(10, NUM_FOTELI); // 10 miejsc w poczekalni, 2 fotele

    // Tworzenie wątku kierownika
    pthread_t kierownik_thread;
    if (pthread_create(&kierownik_thread, NULL, kierownik_handler, NULL) != 0) {
        perror("Błąd tworzenia wątku kierownika");
        exit(EXIT_FAILURE);
    }

    // Tworzenie wątków fryzjerów
    pthread_t fryzjer_threads[NUM_FRYZJEROW];
    for (int i = 0; i < NUM_FRYZJEROW; i++) {
        int* fryzjer_id = malloc(sizeof(int));
        if (!fryzjer_id) {
            perror("Błąd alokacji pamięci dla fryzjera");
            exit(EXIT_FAILURE);
        }
        *fryzjer_id = i + 1;
        if (pthread_create(&fryzjer_threads[i], NULL, fryzjer_handler, fryzjer_id) != 0) {
            perror("Błąd tworzenia wątku fryzjera");
            exit(EXIT_FAILURE);
        }
    }

    // Tworzenie wątków klientów
    pthread_t klient_threads[NUM_KLIENTOW];
    for (int i = 0; i < NUM_KLIENTOW; i++) {
        int* klient_id = malloc(sizeof(int));
        if (!klient_id) {
            perror("Błąd alokacji pamięci dla klienta");
            exit(EXIT_FAILURE);
        }
        *klient_id = i + 1;
        if (pthread_create(&klient_threads[i], NULL, klient_handler, klient_id) != 0) {
            perror("Błąd tworzenia wątku klienta");
            exit(EXIT_FAILURE);
        }
        usleep(rand() % 500000); // Klienci przychodzą w losowych odstępach czasu
    }

    // Program działa, dopóki salon jest otwarty
    while (salon_open) {
        usleep(100000); // Główna pętla kontrolna
    }

    // Zamykanie salonu
    printf("Salon jest zamykany. Fryzjerzy kończą pracę.\n");

    // Czekanie na zakończenie wątków fryzjerów
    for (int i = 0; i < NUM_FRYZJEROW; i++) {
        pthread_cancel(fryzjer_threads[i]); // Anulowanie pracy fryzjerów
        pthread_join(fryzjer_threads[i], NULL);
    }

    // Czekanie na zakończenie wątków klientów
    for (int i = 0; i < NUM_KLIENTOW; i++) {
        pthread_join(klient_threads[i], NULL);
    }

    // Czekanie na zakończenie wątku kierownika
    pthread_join(kierownik_thread, NULL);

    // Sprzątanie zasobów
    cleanup_resources();

    printf("Salon został zamknięty.\n");

    return 0;
}
