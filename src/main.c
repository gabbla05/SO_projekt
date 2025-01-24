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
        printf("%s [DEBUG ZOMBIE_COLLECTOR] PRZED WAITPID\n", get_timestamp());
        while (waitpid(-1, NULL, WNOHANG) > 0) {
            printf("%s [DEBUG ZOMBIE_COLLECTOR] PROCES ZOMBIE ZEBRANY\n", get_timestamp());
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

    // Rejestracja obsługi sygnałów
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("Błąd rejestracji SIGUSR2");
        exit(EXIT_FAILURE);
    }
    int hour;
    /*printf("Podaj aktualną godzinę (8-18): ");
    int hour;
    scanf("%d", &hour);
    if (hour < 8 || hour > 18) {
        printf("Nieprawidłowa godzina. Salon działa od 8 do 18.\n");
        exit(EXIT_FAILURE);
    }

    printf("%s Salon otwarty. Symulacja od godziny %d do 18.\n", get_timestamp(), hour);*/

    init_resources();

    // Utwórz wątek zbierający zombie - jakos inaczej sie killuja
    pthread_t zombie_thread;
    if (pthread_create(&zombie_thread, NULL, zombie_collector, NULL) != 0) {
        perror("Błąd tworzenia wątku zbierającego zombie");
        exit(EXIT_FAILURE);
    }
    
    pid_t fryzjer_pids[NUM_FRYZJEROW];
    pid_t klient_pids[NUM_KLIENTOW];

    // Otwórz salon na początku
    open_salon(fryzjer_pids, klient_pids);
    
    for (int i = 0; i < NUM_FRYZJEROW; i++) {
        fryzjer_pids[i] = fork();
        if (fryzjer_pids[i] == 0) {
            fryzjer_handler(i + 1);
            exit(0);
        }
        printf("%s twoja stara\n", get_timestamp());
    }

    // Tworzenie procesu kierownika
    pid_t kierownik_pid = fork();
    if (kierownik_pid == 0) {
        kierownik_handler(fryzjer_pids);
        exit(0);
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

    

    // Symulacja czasu
    while (1) {
        sleep(1); // Symuluj upływ czasu (1 sekunda = 1 godzina)
        hour++;

        // Sprawdź, czy nadszedł czas zamknięcia salonu
        if (hour >= 18) {
            close_salon(fryzjer_pids, klient_pids);
            // Czekaj do godziny 8:00
            printf("%s [SYSTEM] Salon zamknięty. Czekam 14 sekund (14 godzin) przed ponownym otwarciem.\n", get_timestamp());
            sleep(14); // Czekaj 14 sekund (14 godzin w symulacji)

            // Resetuj godzinę na 8:00
            hour = 8;
            open_salon(fryzjer_pids, klient_pids);
        }
    }
    // Zatrzymaj wątek zbierający zombie
    atomic_store(&zombie_collector_running, 0); // Ustaw flagę na 0, aby zatrzymać wątek
    pthread_join(zombie_thread, NULL); // Poczekaj na zakończenie wątku

    printf("%s [SYSTEM] Wszystkie procesy zakończone. Zamykanie programu.\n", get_timestamp());

    cleanup_resources(); // Sprzątanie zasobów po zakończeniu pracy

    return 0;
}