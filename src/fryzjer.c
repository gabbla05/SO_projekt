#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <string.h>

// Flaga oznaczająca aktywność fryzjera
volatile sig_atomic_t active = 1;



// Funkcja obsługująca fryzjera
void* fryzjer_handler(void* arg) {
    int fryzjer_id = *((int*)arg);
    free(arg);
    srand(time(NULL) + fryzjer_id);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa)); // Wyzerowanie struktury
    sa.sa_handler = handle_signal;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Błąd rejestracji SIGUSR1 dla fryzjera");
        pthread_exit(NULL);
    }



    while (1) {
        // Sprawdzenie, czy fryzjer jest aktywny
        if (!active) {
            printf("Fryzjer %d został wyłączony z pracy.\n", fryzjer_id);
            pthread_exit(NULL);
        }

        int client_id = dequeue();

        if (client_id != -1) {
            int service_cost = (rand() % 10 + 1) * 10;

            int payment = 0;
            int tens = 0, twenties = 0, fifties = 0;

            // Generowanie płatności z nominałów
            while (payment < service_cost) {
                int nominał = rand() % 3;
                if (nominał == 0) {
                    payment += 10;
                    tens++;
                } else if (nominał == 1) {
                    payment += 20;
                    twenties++;
                } else {
                    payment += 50;
                    fifties++;
                }
            }

            printf("Fryzjer %d obsługuje klienta %d. Usługa kosztuje %d zł, klient płaci %d zł.\n",
                   fryzjer_id, client_id, service_cost, payment);
            printf("Płatność klienta %d: %d x 10 zł, %d x 20 zł, %d x 50 zł\n",
                   client_id, tens, twenties, fifties);

            // Dodanie dokładnych nominałów do kasy
            process_payment(tens, twenties, fifties);

            int change = payment - service_cost;
            if (change > 0) {
                printf("Fryzjer %d wydaje resztę %d zł klientowi %d.\n", fryzjer_id, change, client_id);
                give_change(change);
            }

            sem_wait(&fotele_sem);
            printf("Fryzjer %d wykonuje usługę dla klienta %d.\n", fryzjer_id, client_id);
            usleep(rand() % 1000000 + 1000000); // Symulacja czasu usługi
            sem_post(&fotele_sem);

            printf("Fryzjer %d zakończył obsługę klienta %d.\n", fryzjer_id, client_id);
        } else {
            printf("Brak klientów, fryzjer %d śpi...\n", fryzjer_id);
            usleep(1000000);
        }
    }

    return NULL;
}
