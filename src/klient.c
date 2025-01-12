#include "klient.h"
#include "utils.h"
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

extern sem_t poczekalnia_sem;
extern pthread_mutex_t queue_mutex;
extern int poczekalnia[100];

void* client_handler(void* arg) {
    struct Client* client_data = (struct Client*)arg;
    pthread_t thread_id = pthread_self();

    while (1) {
        if (sem_trywait(&poczekalnia_sem) == 0) {
            pthread_mutex_lock(&queue_mutex);
            // Wstawianie klienta do kolejki
            for (int i = 0; i < 100; i++) {
                if (poczekalnia[i] == -1) {
                    poczekalnia[i] = (int)thread_id;
                    break;
                }
            }
            pthread_mutex_unlock(&queue_mutex);
            sem_post(&poczekalnia_sem);
        } else {
            printf("Klient ID %lu śpi i zarabia pieniądze\n", thread_id);
            usleep(rand() % 1000000 + 1000000);
        }
    }
}
