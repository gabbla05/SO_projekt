#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/prctl.h>
#include "utils.h"
#include "kierownik.h"

void kierownik_handler(pid_t fryzjer_pids[]) {
    prctl(PR_SET_NAME, "kierownik", 0, 0, 0);
    srand(time(NULL));

    while (1) {
        sleep(rand() % 10 + 5); // Losowy czas oczekiwania od 5 do 14 sekund
        //int action = rand() % 2;
        sleep(2);
        int action = 0;
        if (action == 0) {
            int fryzjer_id = rand() % NUM_FRYZJEROW + 1; // Losowy fryzjer od 1 do NUM_FRYZJEROW
            printf("%s [KIEROWNIK] Wyłączam fryzjera %d.\n", get_timestamp(), fryzjer_id);
            kill(fryzjer_pids[fryzjer_id - 1], SIGUSR1); // Wyślij sygnał tylko do wybranego fryzjera
            break;
        } else {
            printf("%s [KIEROWNIK] Wszyscy klienci muszą natychmiast opuścić salon.\n", get_timestamp());
            kill(0, SIGUSR2); // Wyślij sygnał do wszystkich procesów
            break;
        }
    }
}