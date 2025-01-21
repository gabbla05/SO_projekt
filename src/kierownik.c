#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 500
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/prctl.h>
#include "utils.h"

extern pid_t main_process_pid;

void kierownik_handler() {
    prctl(PR_SET_NAME, "kierownik", 0, 0, 0); // Ustaw nazwę procesu
    srand(time(NULL));
    pid_t pid = getpid(); // Pobierz PID kierownika

    while (1) {
        sleep(rand() % 10 + 5); // Losowe opóźnienie
        int sig_type = rand() % 2; // Wybór sygnału

        if (sig_type == 0) {
            // Losuj fryzjera do wyłączenia (od 1 do NUM_FRYZJEROW)
            int fryzjer_id = rand() % NUM_FRYZJEROW + 1;
            printf("%s [PID %d] Wysyłam SIGUSR1: Wyłączam fryzjera %d.\n", 
                   get_process_role(), pid, fryzjer_id);
            kill(0, SIGUSR1); // Wyłączenie fryzjera
        } else {
            printf("%s [PID %d] Wysyłam SIGUSR2: Zamknięcie salonu.\n", 
                   get_process_role(), pid);
            kill(0, SIGUSR2); // Zamknięcie salonu
            exit(0); // Zakończenie działania kierownika
        }
    }
}

