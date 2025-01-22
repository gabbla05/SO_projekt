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
atomic_int zombie_collector_running = 1;
pid_t main_process_pid;

void handle_signal(int sig) {
    if (sig == SIGUSR2) {
        static int signal_handled = 0; // Flaga zapobiegająca wielokrotnemu wywołaniu
        if (signal_handled) return; // Jeśli sygnał już był obsłużony, zakończ
        signal_handled = 1;

        lock_semaphore();
        kasa->salon_open = 0;
        unlock_semaphore();
        printf("%s [KIEROWNIK] Zamknięcie salonu zostało zainicjowane.\n", get_timestamp());

        // Wyślij sygnał SIGUSR2 do wszystkich klientów, aby opuścili salon
        kill(0, SIGUSR2); // Wysyła sygnał do wszystkich procesów w grupie
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
    return NULL;
}

void simulate_time(void* arg) {
    int starting_time = *((int*)arg); // Dereference the pointer to get the starting time

    for (int i = 0; i < 18; i++) {
        sleep(1); // Symulacja jednej godziny
        printf("%s [SYMULACJA] Minęła godzina %d.\n",get_timestamp(), (starting_time + i)); // Dopasuj godzinę startową
    }
    kill(0, SIGUSR2); // Wyślij sygnał zamknięcia salonu
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

    printf("Podaj aktualną godzinę (8-18): ");
    int hour;
    scanf("%d", &hour);
    if (hour < 8 || hour > 18) {
        printf("Nieprawidłowa godzina. Salon działa od 8 do 18.\n");
        exit(EXIT_FAILURE);
    }

    printf("%s Salon otwarty. Symulacja od godziny %d do 18.\n",get_timestamp(), hour);

    init_resources();

    // Symulacja czasu pracy salonu
    int starting_time = hour;

    /* pthread_t time_thread;
    if (pthread_create(&time_thread, NULL, (void*)simulate_time, (void*)&starting_time) != 0) {
        perror("Błąd tworzenia wątku symulacji czasu");
        exit(EXIT_FAILURE);
    }*/

    pthread_t zombie_thread;
    if (pthread_create(&zombie_thread, NULL, zombie_collector, NULL) != 0) {
        perror("Błąd tworzenia wątku zbierającego zombie");
        exit(EXIT_FAILURE);
        
    }

    pid_t fryzjer_pids[NUM_FRYZJEROW];


    // Tworzenie procesów fryzjerów
    for (int i = 0; i < NUM_FRYZJEROW; i++) {
        fryzjer_pids[i] = fork();
        if (fryzjer_pids[i] == 0) {
            fryzjer_handler(i + 1);
            exit(0);
        }
    }

    // Tworzenie procesu kierownika
    pid_t kierownik_pid = fork();
    if (kierownik_pid == 0) {
        kierownik_handler(fryzjer_pids);
        exit(0);
    }

    // Tworzenie procesów klientów
    for (int i = 0; i < NUM_KLIENTOW; i++) {
        pid_t klient_pid = fork();
        if (klient_pid == 0) {
            klient_handler(i + 1);
            exit(0);
        }
        usleep(100000); // Opóźnienie między klientami
    }

    // Czekaj na zakończenie wątku symulacji czasu
    //pthread_join(time_thread, NULL);

    atomic_store(&zombie_collector_running, 0); // Zatrzymaj wątek
    pthread_join(zombie_thread, NULL);


    while(1){
        int switcher;
        scanf("%d", &switcher);
        switch(switcher){
            case 1: {
                // Wyślij sygnał SIGUSR1 do losowego fryzjera
                int fryzjer_id = rand() % NUM_FRYZJEROW; // Losowy fryzjer od 0 do NUM_FRYZJEROW - 1
                printf("%s [KIEROWNIK] Wysyłam sygnał SIGUSR1 do fryzjera %d.\n", get_timestamp(), fryzjer_id + 1);
                kill(fryzjer_pids[fryzjer_id], SIGUSR1);
                break;
            }
            case 2: {
                // Wyślij sygnał SIGUSR2 (zamknij salon)
                printf("%s [KIEROWNIK] Wysyłam sygnał SIGUSR2 (zamknięcie salonu).\n", get_timestamp());
                kill(0, SIGUSR2); // Wyślij sygnał do wszystkich procesów
                break;
            }
            case 3: {
                // Wyjdź z programu
                cleanup_resources(); // Sprzątanie zasobów po zakończeniu pracy
                printf("Salon został zamknięty.\n");
                return 0;
            }
            default:
                printf("Nieprawidłowa opcja. Wybierz ponownie.\n");
                break;
        }
    }

        // Czekaj na zakończenie procesów fryzjerów i klientów
    for (int i = 0; i < NUM_FRYZJEROW; i++) {
        waitpid(fryzjer_pids[i], NULL, 0);
    }

    for (int i = 0; i < NUM_KLIENTOW; i++) {
        wait(NULL); // Czekanie na klientów
    }

    return 0;
}
