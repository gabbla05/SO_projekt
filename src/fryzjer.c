#include "fryzjer.h"
#include "utils.h"
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

extern sem_t fotele_sem;
extern pthread_mutex_t queue_mutex;
extern int poczekalnia[100]; // Kolejka o stałej wielkości (można to zmienić na dynamiczną)

void* fryzjer_handler(void *arg) {
    int clientID;
    while (1) {      
        if (poczekalnia[0] != -1) { // Sprawdzenie, czy jest klient
            pthread_mutex_lock(&queue_mutex);
            clientID = poczekalnia[0];
            poczekalnia[0] = -1; // Usuwanie klienta z kolejki
            pthread_mutex_unlock(&queue_mutex);

            sem_wait(&fotele_sem); // Czekanie na fotel
            // TODO: Obsługa płatności
        } else {
            printf("Fryzjer śpi\n");
            usleep(rand() % 1000000 + 1000000);
        }
    }
}
