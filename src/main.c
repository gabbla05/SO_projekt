#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 500
#include "utils.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <stdatomic.h>

atomic_int zombie_collector_running = 1; // Flaga kontrolująca działanie wątku zombie_collector
pid_t main_process_pid;

void handle_signal(int sig) {
    if (sig == SIGUSR2) {
        static int signal_handled = 0; // Flaga zapobiegająca wielokrotnemu wywołaniu
        signal_handled = 1;

        printf("%s [KIEROWNIK] Wszyscy klienci muszą natychmiast opuścić salon.\n", get_timestamp());

        // Nie wysyłaj ponownie sygnału SIGUSR2!
        // kill(0, SIGUSR2); // Usuń tę linię, aby uniknąć rekurencji
    }
}

// Funkcja wątku zbierającego zombie
void* zombie_collector(void* arg) {
    (void)arg; // Nieużywany argument
    while (atomic_load(&zombie_collector_running)) {
        // Wywołaj waitpid, aby zebrać procesy zombie
        while (waitpid(-1, NULL, WNOHANG) > 0) {
            // Proces zombie został zebrany
        }
        sleep(1); // Odczekaj sekundę przed kolejnym sprawdzeniem
    }
    printf("%s [SYSTEM] Wątek zbierający zombie zakończył działanie.\n", get_timestamp());
    return NULL;
}

int main() {
    main_process_pid = getpid();
    srand(time(NULL));

    

    // Utwórz wątek zbierający zombie
    pthread_t zombie_thread;
    if (pthread_create(&zombie_thread, NULL, zombie_collector, NULL) != 0) {
        perror("Błąd tworzenia wątku zbierającego zombie");
        exit(EXIT_FAILURE);
    }

    // Tworzenie procesu kierownika
    pid_t kierownik_pid = fork();
    if (kierownik_pid == 0) {
        // Proces potomny (kierownik)
        kierownik_handler(NULL); // Przekazujemy NULL, ponieważ fryzjer_pids nie jest używany w kierownik_handler
        exit(0);
    } else if (kierownik_pid < 0) {
        perror("Błąd forkowania procesu kierownika");
        exit(EXIT_FAILURE);
    }

    // Główna pętla programu
    int option;
    while (1) {
        printf("\nWybierz opcję:\n");
        printf("1. Zakończ program\n");
        printf("2. Inna opcja (do zaimplementowania)\n");
        printf("Wybierz: ");
        scanf("%d", &option);

        switch (option) {
            case 1:
                printf("%s [KIEROWNIK] Kończenie programu...\n", get_timestamp());
                endProgram(0); // Wywołanie funkcji endProgram z kodem 0
                break;
            case 2:
                printf("Inna opcja (do zaimplementowania)\n");
                break;
            default:
                printf("Nieprawidłowa opcja. Spróbuj ponownie.\n");
                break;
        }
    }

    // Zakończenie wątku zbierającego zombie
    atomic_store(&zombie_collector_running, 0);
    pthread_join(zombie_thread, NULL);

    return 0;
}