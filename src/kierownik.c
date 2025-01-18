#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

void* kierownik_handler(void* arg) {
    srand(time(NULL));
    while (1) {
        sleep(rand() % 10 + 5); // Kierownik wysyła sygnał co 5-15 sekund

        int signal_type = rand() % 2; // Losowanie sygnału (0 -> SIGUSR1, 1 -> SIGUSR2)
        if (signal_type == 0) {
            printf("Kierownik: wysyłam SIGUSR1 (wyłączenie fryzjera).\n");
            raise(SIGUSR1); // Wysyłanie sygnału SIGUSR1
        } else {
            printf("Kierownik: wysyłam SIGUSR2 (zamknięcie salonu).\n");
            raise(SIGUSR2); // Wysyłanie sygnału SIGUSR2
            break; // Po SIGUSR2 salon jest zamknięty
        }
    }

    return NULL;
}
