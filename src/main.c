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

#define NUM_FRYZJEROW 3

atomic_int zombie_collector_running = 1; // Flaga kontrolująca działanie wątku zombie_collector
pid_t main_process_pid;
pid_t fryzjer_pids[NUM_FRYZJEROW];
pid_t klient_pids[NUM_KLIENTOW];
atomic_int program_running = 1;

void handle_signal(int sig) {
    if (sig == SIGUSR2) {
        static int signal_handled = 0; // Flaga zapobiegająca wielokrotnemu wywołaniu
        signal_handled = 1;

        printf("%s [KIEROWNIK] Wszyscy klienci muszą natychmiast opuścić salon.\n", get_timestamp());

        // Nie wysyłaj ponownie sygnału SIGUSR2!
        // kill(0, SIGUSR2); // Usuń tę linię, aby uniknąć rekurencji
    }
}

// Funkcja wątku obsługującego wejście z klawiatury
void* input_thread(void* arg) {
    (void)arg;

    while (atomic_load(&program_running)) {
        printf("\n%s [SYSTEM] Naciśnij:\n", get_timestamp());
        printf("1 - Wyślij pierwszy sygnał kierownika (SIGUSR1)\n");
        printf("2 - Wyślij drugi sygnał kierownika (SIGUSR2)\n");
        printf("q - Wyjdź z programu\n");

        char input = getchar();
        getchar(); // Pobierz znak nowej linii po wciśnięciu Enter

        switch (input) {
            case '1':
                printf("%s [SYSTEM] Uruchamiam kierownika z akcją 1 (SIGUSR1).\n", get_timestamp());
                pid_t kierownik_pid = fork();
                if (kierownik_pid == 0) {
                    kierownik_handler(fryzjer_pids, 1);
                    exit(0);
                }
                break;
            case '2':
                printf("%s [SYSTEM] Uruchamiam kierownika z akcją 2 (SIGUSR2).\n", get_timestamp());
                kierownik_pid = fork();
                if (kierownik_pid == 0) {
                    kierownik_handler(fryzjer_pids, 2);
                    exit(0);
                }
                break;
            case 'q':
                printf("%s [SYSTEM] Program zostanie zamkniety po zakonczeniu bieżącego dnia.\n", get_timestamp());
                atomic_store(&program_running, 0);
                break;
            default:
                printf("%s [SYSTEM] Nieznana komenda. Spróbuj ponownie.\n", get_timestamp());
                break;
        }
    }

    return NULL;
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

// Funkcja wątku symulującego czas
void* time_simulation_thread(void* arg) {
    (void)arg; // Nieużywany argument
        int hour = 8;
        printf("%s Salon otwarty. Aktualna godzina: %d\n", get_timestamp(), hour);
        init_resources();

        for(int i = 0; i < NUM_FRYZJEROW; i++){
            fryzjer_pids[i] == 0;
        }

        for(int i = 0; i < NUM_KLIENTOW; i++){
            klient_pids[i] == 0;
        }
        // Tworzenie procesów fryzjerów
        for (int i = 0; i < NUM_FRYZJEROW; i++) {
            fryzjer_pids[i] = fork();
            if (fryzjer_pids[i] == 0) {
                printf("%s [SYSTEM] Tworzenie fryzjera %d (PID: %d)\n", get_timestamp(), i + 1, getpid());
                fryzjer_handler(i + 1);
                exit(0);
            } else if (fryzjer_pids[i] < 0) {
                perror("Błąd podczas tworzenia fryzjera");
                exit(EXIT_FAILURE);
            }
        }

        // Tworzenie procesów klientów
        for (int i = 0; i < NUM_KLIENTOW; i++) {
            klient_pids[i] = fork();
            if (klient_pids[i] == 0) {
                klient_handler(i + 1);
                exit(0);
            }
            usleep(100000); // Opóźnienie między klientami
        }
        

    while (hour < 18) {
        hour++;
        printf("%s Salon otwarty. Aktualna godzina: %d\n", get_timestamp(), hour);
        sleep(1); // Symulacja upływu czasu (1 sekunda = 1 godzina)
    }
    printf("%s Zamykamy salon, po tym gdy fryzjerzy dokoncza aktualne zlecenia. Aktualna godzina: %d\n", get_timestamp(), hour);

    // Ustaw flagę informującą o zamknięciu salonu
    sharedMemory->salon_open = 0;

        // Wyczyść zasoby po zakończeniu dnia
        cleanup_resources();

        // Zakończ procesy fryzjerów i klientów
        for (int i = 0; i < NUM_FRYZJEROW; i++) {
            if (fryzjer_pids[i] > 0) {
                kill(fryzjer_pids[i], SIGUSR1); // Wyślij sygnał SIGTERM do fryzjera
            }
        }
        for (int i = 0; i < NUM_KLIENTOW; i++) {
            if (klient_pids[i] > 0) {
                kill(klient_pids[i], SIGUSR1); // Wyślij sygnał SIGTERM do klienta
            }
        }


    printf("%s [SYSTEM] Symulacja czasu zakończona. Salon zamknięty.\n", get_timestamp());
    return NULL;
}

int main() {
    main_process_pid = getpid();
    srand(time(NULL));

    // Rejestracja obsługi sygnałów
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("Błąd rejestracji SIGUSR2");
        exit(EXIT_FAILURE);
    }

    // Utwórz wątek obsługujący wejście z klawiatury
    pthread_t input_thread_handle;
    if (pthread_create(&input_thread_handle, NULL, input_thread, NULL) != 0) {
        perror("Błąd tworzenia wątku obsługującego wejście");
        exit(EXIT_FAILURE);
    }

    // Główna pętla programu
    while (atomic_load(&program_running)) {
        // Utwórz wątek zbierający zombie
        pthread_t zombie_thread;
        if (pthread_create(&zombie_thread, NULL, zombie_collector, NULL) != 0) {
            perror("Błąd tworzenia wątku zbierającego zombie");
            exit(EXIT_FAILURE);
        }

        // Utwórz wątek symulujący czas
        pthread_t time_thread;
        if (pthread_create(&time_thread, NULL, time_simulation_thread, NULL) != 0) {
            perror("Błąd tworzenia wątku symulującego czas");
            exit(EXIT_FAILURE);
        }

        

        // Oczekiwanie na zakończenie wątku symulującego czas
        pthread_join(time_thread, NULL);

        // Zatrzymaj wątek zbierający zombie
        atomic_store(&zombie_collector_running, 0);
        pthread_join(zombie_thread, NULL);


        printf("%s [SYSTEM] Koniec dnia. Rozpoczynam nowy dzień.\n", get_timestamp());
        sleep(10); // Czekaj 10 sekund przed rozpoczęciem nowego dnia
    }

    // Zatrzymaj wątek obsługujący wejście
    pthread_join(input_thread_handle, NULL);

    printf("%s [SYSTEM] Program zakończony.\n", get_timestamp());
    return 0;
}