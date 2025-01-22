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
        //sleep(rand() % 10 + 5); // Losowy czas oczekiwania od 5 do 14 sekund
        //int action = rand() % 2; // Losowy wybór między 0 a 1
        int action = 0;
        sleep(3);
        if (action == 0) {
            // Wyślij sygnał SIGUSR1 do losowego fryzjera
            int fryzjer_id = rand() % NUM_FRYZJEROW + 1; // Losowy fryzjer od 1 do NUM_FRYZJEROW
            printf("%s [KIEROWNIK] Wyłączam fryzjera %d.\n", get_timestamp(), fryzjer_id);
            kill(fryzjer_pids[fryzjer_id - 1], SIGUSR1); // Wyślij sygnał tylko do wybranego fryzjera
        } else {
        /*
        printf("%s [KIEROWNIK] Removing all clients from the waiting room.\n", get_timestamp());

        struct Message message;
        while (msgrcv(msg_queue_id, &message, sizeof(message) - sizeof(long), 1, 0) != -1) {
            int client_id = message.client_id;
            int client_pid = message.client_pid;

            printf("%s [KIEROWNIK] Wyrzucam klienta %d (PID: %d) z kolejki komunikatów.\n", get_timestamp(), client_id, client_pid);

            kill(client_pid, SIGUSR2);
        }
                */
        break; // Exit the loop after processing
                /*
                lock_semaphore();
                    for (int i = 0; i < NUM_FOTELE; i++) {
                        if (kasa->client_on_chair[i] != 0) { // Jeśli fotel jest zajęty
                            int client_id = kasa->client_on_chair[i];
                            printf("%s [KIEROWNIK] Wyrzucam klienta %d z fotela.\n", get_timestamp(), client_id);
                            kill(client_pids[client_id - 1], SIGUSR2); // Wyślij sygnał do klienta
                            kasa->client_on_chair[i] = 0; // Zwolnij fotel
                        }
                    }
                unlock_semaphore();
                */
        }

        break; // Zakończ pętlę po wykonaniu jednej akcji
    }
}