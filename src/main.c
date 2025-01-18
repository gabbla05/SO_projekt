#include "utils.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "fryzjer.h"
#include "klient.h"

#define NUM_KLIENTOW 10 // Liczba klientów
#define NUM_FRYZJEROW 3 // Liczba fryzjerów
#define NUM_FOTELI 2    // Liczba foteli

int main() {
    init_resources(10, NUM_FOTELI); // Inicjalizacja poczekalni i foteli

    pthread_t fryzjer_threads[NUM_FRYZJEROW];
    pthread_t klient_threads[NUM_KLIENTOW];
    int klient_ids[NUM_KLIENTOW];

    // Tworzenie wątków fryzjerów
    for (int i = 0; i < NUM_FRYZJEROW; i++) {
        int* fryzjer_id = malloc(sizeof(int)); // Dynamicznie alokujemy pamięć na ID fryzjera
        *fryzjer_id = i + 1;
        pthread_create(&fryzjer_threads[i], NULL, fryzjer_handler, fryzjer_id);
    }

    // Tworzenie wątków klientów
    for (int i = 0; i < NUM_KLIENTOW; i++) {
        klient_ids[i] = i + 1;
        pthread_create(&klient_threads[i], NULL, klient_handler, &klient_ids[i]);
        usleep(rand() % 1000000); // Klienci przychodzą losowo
    }

    // Czekanie na zakończenie wątków klientów
    for (int i = 0; i < NUM_KLIENTOW; i++) {
        pthread_join(klient_threads[i], NULL);
    }

    cleanup_resources(); // Sprzątanie zasobów
    return 0;
}