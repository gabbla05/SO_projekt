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


pid_t main_process_pid;
volatile int czas_do_zamkniecia = 1;

void handle_signal(int sig) {
    if (sig == SIGUSR1) {
        lock_semaphore();
        for (int i = 0; i < NUM_FRYZJEROW; i++) {
            if (kasa->fryzjer_status[i] == 1) { // Znajdź aktywnego fryzjera
                kasa->fryzjer_status[i] = 0; // Oznacz fryzjera jako wyłączonego
                printf("[KIEROWNIK] Otrzymano SIGUSR1: Wyłączono fryzjera %d.\n", i + 1);
                unlock_semaphore();
                return;
            }
        }
        unlock_semaphore();
        printf("[KIEROWNIK] Otrzymano SIGUSR1, ale wszyscy fryzjerzy są już wyłączeni.\n");
    } else if (sig == SIGUSR2) {
        lock_semaphore();
        kasa->salon_open = 0; // Zamykanie salonu
        unlock_semaphore();
        printf("[KIEROWNIK] Otrzymano SIGUSR2: Zamknięcie salonu.\n");
        czas_do_zamkniecia = 0; // Wyłącz pętlę główną symulacji
    }
}

void sprawdz_godzine() {
    int godzina;
    printf("Podaj aktualną godzinę (8-18): ");
    while (scanf("%d", &godzina) != 1 || godzina < 8 || godzina > 18) {
        printf("Błąd: Podaj godzinę w przedziale od 8 do 18: ");
        while (getchar() != '\n'); // Wyczyść bufor
    }
    printf("Salon otwarty. Symulacja od godziny %d do 18.\n", godzina);

    int czas_pracy = 18 - godzina; // Liczba godzin do symulacji
    printf("Salon będzie działał przez %d godzin w symulacji.\n", czas_pracy);
    czas_do_zamkniecia = czas_pracy;
}

void symuluj_czas() {
    for (int i = 0; i < czas_do_zamkniecia; i++) {
        sleep(1); // Symulacja jednej godziny (1 sekunda = 1 godzina)
        printf("[SYMUACJA] Minęła godzina %d. Do zamknięcia salonu pozostało %d godzin.\n", 8 + i, czas_do_zamkniecia - i - 1);
    }
    printf("[SYMUACJA] Jest godzina 18. Salon zostaje zamknięty.\n");
    kill(0, SIGUSR2); // Wyślij sygnał zamknięcia salonu
}

int main() {
    main_process_pid = getpid(); // Ustaw PID procesu głównego
    srand(time(NULL));

    // Rejestracja obsługi sygnałów
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Błąd rejestracji SIGUSR1");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("Błąd rejestracji SIGUSR2");
        exit(EXIT_FAILURE);
    }

    printf("Obsługa sygnałów została zarejestrowana.\n");

    sprawdz_godzine(); // Pobierz godzinę od użytkownika

    // Inicjalizacja zasobów
    init_resources();

    // Tworzenie wątku do usuwania procesów zombie
    pthread_t zombie_thread;
    if (pthread_create(&zombie_thread, NULL, reap_zombies, NULL) != 0) {
        perror("Błąd tworzenia wątku zombie");
        exit(EXIT_FAILURE);
    }

    // Tworzenie procesu kierownika
    pid_t kierownik_pid = fork();
    if (kierownik_pid == 0) {
        prctl(PR_SET_NAME, "kierownik");
        kierownik_handler();
        exit(0);
    } else if (kierownik_pid < 0) {
        perror("Błąd tworzenia procesu kierownika");
        exit(EXIT_FAILURE);
    }

    // Tworzenie procesów fryzjerów
    pid_t fryzjer_pids[NUM_FRYZJEROW];
    for (int i = 0; i < NUM_FRYZJEROW; i++) {
        printf("[MAIN] Tworzenie procesu fryzjera %d...\n", i + 1);
        fryzjer_pids[i] = fork();
        if (fryzjer_pids[i] == 0) {
            prctl(PR_SET_NAME, "fryzjer");
            fryzjer_handler(i + 1);
            exit(0);
        } else if (fryzjer_pids[i] < 0) {
            perror("Błąd tworzenia procesu fryzjera");
            exit(EXIT_FAILURE);
        }
    }

    // Tworzenie procesów klientów
    for (int i = 0; i < NUM_KLIENTOW; i++) {
        pid_t klient_pid = fork();
        if (klient_pid == 0) {
            prctl(PR_SET_NAME, "klient");
            klient_handler(i + 1);
            exit(0);
        } else if (klient_pid < 0) {
            perror("Błąd tworzenia procesu klienta");
            exit(EXIT_FAILURE);
        }
        usleep(rand() % 500000); // Klienci przychodzą w losowych odstępach czasu
    }

    // Symulacja czasu pracy salonu
    symuluj_czas();

    // Czekanie na zakończenie wszystkich procesów
    wait_for_processes();

    // Sprzątanie zasobów
    cleanup_resources();
    printf("Salon został zamknięty.\n");

    return 0;
}