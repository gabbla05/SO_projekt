#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/prctl.h>
#include "utils.h"
#include "kierownik.h"

void kierownik_handler(pid_t fryzjer_pids[], int action_type) {
    prctl(PR_SET_NAME, "kierownik", 0, 0, 0);
    srand(time(NULL));

    if (action_type == 1) {
        // Wyślij sygnał SIGUSR1 do losowego fryzjera
        int fryzjer_id = rand() % NUM_FRYZJEROW + 1;
        printf("%s [KIEROWNIK] Wyłączam fryzjera %d.\n", get_timestamp(), fryzjer_id);
        kill(fryzjer_pids[fryzjer_id - 1], SIGUSR1);
    } 
    else if (action_type == 2) {
        // Wyczyść semafor poczekalni
        printf("%s [KIEROWNIK] Czyszczenie semafora poczekalni.\n", get_timestamp());
        semctl(poczekalnia_id, 0, SETVAL, MAX_QUEUE_SIZE);

        // Czyszczenie kolejki komunikatów i wysyłanie sygnałów do klientów w kolejce
        printf("%s [KIEROWNIK] Czyszczenie kolejki komunikatów i powiadamianie klientów.\n", get_timestamp());
        struct Message message;
        while (msgrcv(msg_queue_id, &message, sizeof(message) - sizeof(long), 1, IPC_NOWAIT) != -1) {
            int client_pid = message.client_pid;
            printf("%s [KIEROWNIK] Wysyłanie sygnału SIGUSR2 do klienta w kolejce (PID: %d).\n", 
                   get_timestamp(), client_pid);
            kill(client_pid, SIGUSR2);
        }

        
        // Wymuś opuszczenie salonu przez klientów na fotelach
        printf("%s [KIEROWNIK] Wymuszanie opuszczenia salonu przez klientów na fotelach.\n", get_timestamp());

        lock_semaphore();
        for (int i = 0; i < NUM_FOTELE; i++) {
            if (sharedMemory->client_on_chair[i] != 0) {
                int client_pid = sharedMemory->client_on_chair[i];
                printf("%s [KIEROWNIK] Wysyłanie sygnału SIGUSR2 do klienta na fotelu (PID: %d).\n", 
                       get_timestamp(), client_pid);
                kill(client_pid, SIGUSR2);
                sharedMemory->client_on_chair[i] = 0;
            }
        }
        unlock_semaphore();

        // Zwolnij semafor foteli
        printf("%s [KIEROWNIK] Zwolnienie wszystkich foteli.\n", get_timestamp());
        semctl(fotele_id, 0, SETVAL, NUM_FOTELE);

        printf("%s [KIEROWNIK] Zakończono czyszczenie salonu.\n", get_timestamp());
    }
}