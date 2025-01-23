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
        int action = 1;
        sleep(3);
        if (action == 0) {
            // Wyślij sygnał SIGUSR1 do losowego fryzjera
            int fryzjer_id = rand() % NUM_FRYZJEROW + 1; // Losowy fryzjer od 1 do NUM_FRYZJEROW
            printf("%s [KIEROWNIK] Wyłączam fryzjera %d.\n", get_timestamp(), fryzjer_id);
            kill(fryzjer_pids[fryzjer_id - 1], SIGUSR1); // Wyślij sygnał tylko do wybranego fryzjera
        } else {
            // Wyczyść semafor poczekalni
            printf("%s [KIEROWNIK] Czyszczenie semafora poczekalni.\n", get_timestamp());
            semctl(poczekalnia_id, 0, SETVAL, MAX_QUEUE_SIZE); // Ustaw semafor na maksymalną pojemność

            // Zabij procesy klientów znajdujące się w kolejce
            printf("%s [KIEROWNIK] Zabijanie procesów klientów w kolejce.\n", get_timestamp());
            struct Message message;
            while (msgrcv(msg_queue_id, &message, sizeof(message) - sizeof(long), 1, IPC_NOWAIT) != -1) {
                int client_pid = message.client_pid;
                printf("%s [KIEROWNIK] Wysyłanie sygnału SIGUSR2 do klienta (PID: %d).\n", get_timestamp(), client_pid);
                kill(client_pid, SIGUSR2); // Wyślij sygnał SIGUSR2 do klienta
            }

            // Wymuś opuszczenie salonu przez klientów na fotelach
            printf("%s [KIEROWNIK] Wymuszanie opuszczenia salonu przez klientów na fotelach.\n", get_timestamp());

            lock_semaphore(); // Zablokuj dostęp do pamięci współdzielonej
            kasa->continueFlag = 1; // Ustaw flagę continueFlag
            for (int i = 0; i < NUM_FOTELE; i++) {
                if (kasa->client_on_chair[i] != 0) { // Jeśli fotel jest zajęty
                    int client_pid = kasa->client_on_chair[i];
                    printf("%s [KIEROWNIK] Wysyłanie sygnału SIGUSR2 do klienta na fotelu (PID: %d).\n", get_timestamp(), client_pid);
                    kill(client_pid, SIGUSR2); // Wyślij sygnał SIGUSR2 do klienta
                    kasa->client_on_chair[i] = 0; // Zwolnij fotel
                }
            }
            unlock_semaphore(); // Odblokuj dostęp do pamięci współdzielonej

            // Zwolnij semafor foteli
            printf("%s [KIEROWNIK] Zwolnienie wszystkich foteli.\n", get_timestamp());
            semctl(fotele_id, 0, SETVAL, NUM_FOTELE); // Ustaw semafor na maksymalną liczbę foteli

            // Wyczyść kolejkę komunikatów (bez usuwania jej)
            printf("%s [KIEROWNIK] Kolejka komunikatów została wyczyszczona.\n", get_timestamp());

            break; // Zakończ pętlę po wykonaniu akcji
        }

        break; // Zakończ pętlę po wykonaniu jednej akcji
    }
}