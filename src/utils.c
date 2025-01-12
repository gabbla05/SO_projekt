#include "utils.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

sem_t poczekalnia_sem;
sem_t fotele_sem;
pthread_mutex_t queue_mutex;
pthread_mutex_t kasa_mutex;
int poczekalnia[100]; // Kolejka klientów (wskazania do wątków)

void init_resources(int K, int N) {
    // Inicjalizacja mutexów
    if (pthread_mutex_init(&kasa_mutex, NULL) != 0) {
        perror("Błąd inicjalizacji mutexu kasy");
        exit(EXIT_FAILURE); 
    }
    if (pthread_mutex_init(&queue_mutex, NULL) != 0) {
        perror("Błąd inicjalizacji mutexu kolejki");
        exit(EXIT_FAILURE);
    }

    // Inicjalizacja semaforów
    if (sem_init(&poczekalnia_sem, 0, K) == -1) {
        perror("Błąd inicjalizacji semafora poczekalnia_sem");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&fotele_sem, 0, N) == -1) {
        perror("Błąd inicjalizacji semafora fotele_sem");
        exit(EXIT_FAILURE);
    }

    // Inicjalizacja poczekalni
    for (int i = 0; i < 100; i++) {
        poczekalnia[i] = -1;
    }
}
//godziny otwarcia - wprowadzane z klawiatury ktora jest aktualnie godzina - salon czynny od 8 do 18
// poczekalnia jako kolejka komiunikatow; klient fryzjer mutex komiuinikuja sie mutexem 
//kasa - randomowa wielokrotnosc dziesiatki to cena uslugi, losuje sie dla kazdego klienta, platnosc, reszta itd...; kasa jako lista indeks 0 liczba 10zlotowek, indeks 1 liczba 20zlotowek, indeks 2 liczba 50zlotowek 