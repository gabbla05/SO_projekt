#include "fryzjer.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/prctl.h>
#include <errno.h>
#include <signal.h>

extern int fotele_id;
extern int fryzjer_signal_id;
extern struct Kasa* kasa;

// Funkcja obsługi sygnału SIGUSR1
void handle_sigusr1(int sig) {
    if (sig == SIGUSR1) {
        printf("%s [FRYZJER %d] Otrzymałem sygnał do wyłączenia. Kończę pracę.\n", get_timestamp(), getpid());
        exit(0); // Zakończ proces fryzjera
    }
}

void fryzjer_handler(int fryzjer_id) {
    prctl(PR_SET_NAME, "fryzjer", 0, 0, 0);
    srand(getpid());
    pid_t pid = getpid();
    
    // Rejestracja obsługi sygnału SIGUSR1
    struct sigaction sa;
    sa.sa_handler = handle_sigusr1;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Błąd rejestracji SIGUSR1");
        exit(EXIT_FAILURE);
    }

    struct Message message;

    while (1) {
        // Sprawdzenie, czy salon jest otwarty
        lock_semaphore();
        if (kasa->salon_open == 0) {
            unlock_semaphore();
            printf("%s [FRYZJER %d] Salon jest zamknięty. Kończę pracę.\n", get_timestamp(), fryzjer_id);
            exit(0);
        }
        unlock_semaphore();

        // Oczekiwanie na klienta w poczekalni
        printf("%s [FRYZJER %d] Oczekuję na klienta w poczekalni.\n", get_timestamp(), fryzjer_id);
        if (msgrcv(msg_queue_id, &message, sizeof(message) - sizeof(long), 1, 0) == -1) {
            if (errno == EINTR) continue; // Ignoruj przerwania sygnałami
            perror("[FRYZJER] Błąd odbioru komunikatu");
            exit(EXIT_FAILURE);
        }

        int client_id = message.client_id;
        int client_pid = message.client_pid;
        printf("%s [FRYZJER %d] Obsługuję klienta %d.\n", get_timestamp(), fryzjer_id, client_pid);

        struct sembuf zwolnij_miejsce = {0, 1, 0};
        if (semop(poczekalnia_id, &zwolnij_miejsce, 1) == -1) {
            perror("[FRYZJER] Błąd zwalniania miejsca w poczekalni");
        }

        // Zajęcie fotela
        struct sembuf zajmij_fotel = {0, -1, 0};
        while (semop(fotele_id, &zajmij_fotel, 1) == -1) {
            if (errno == EINTR) {
                // Operacja została przerwana przez sygnał, ponów próbę
                continue;
            }
            perror("[FRYZJER] Błąd zajmowania fotela");
            break; // Wyjdź z pętli w przypadku innych błędów
        }

        // Zaktualizuj pamięć współdzieloną: klient zajmuje fotel
        lock_semaphore();
        for (int i = 0; i < NUM_FOTELE; i++) {
            if (kasa->client_on_chair[i] == 0) { // Znajdź wolny fotel
                kasa->client_on_chair[i] = client_pid; // Zajmij fotel przez klienta
                break;
            }
        }
        unlock_semaphore();

        printf("%s [FRYZJER %d] Zajęto fotel dla klienta %d.\n", get_timestamp(), fryzjer_id, client_pid);

        // Rozpoczęcie obsługi klienta
        printf("%s [FRYZJER %d] Rozpoczynam obsługę klienta %d.\n", get_timestamp(), fryzjer_id, client_pid);
        usleep(rand() % 1000000 + 1000000); // Symulacja czasu obsługi

        // Symulacja czasu obsługi z regularnym sprawdzaniem flagi continueFlag
        int time_elapsed = 0;
        int total_time = rand() % 1000000 + 1000000; // Całkowity czas obsługi
        while (time_elapsed < total_time) {
            usleep(100000); // Czekaj 100 ms
            time_elapsed += 100000;

            // Sprawdź, czy klient został przerwany sygnałem SIGUSR2
            lock_semaphore();
            if (kasa->continueFlag == 1) {
                unlock_semaphore();
                printf("%s [FRYZJER %d] Klient %d został przerwany sygnałem SIGUSR2. Przerywam obsługę.\n", get_timestamp(), fryzjer_id, client_pid);
                break; // Przerwij obsługę klienta
            }
            unlock_semaphore();
        }

        // Jeśli klient został przerwany, zakończ obsługę
        lock_semaphore();
        if (kasa->continueFlag == 1) {
            unlock_semaphore();
            continue; // Przerwij obsługę klienta
        }
        unlock_semaphore();
        
        // Generowanie kosztu usługi i płatności
        int cost = (rand() % 8 + 3) * 10;
        int payment = 0, tens = 0, twenties = 0, fifties = 0;

        while (payment < cost) {
            int denomination = rand() % 3;
            if (denomination == 0) {
                payment += 10;
                tens++;
            } else if (denomination == 1) {
                payment += 20;
                twenties++;
            } else {
                payment += 50;
                fifties++;
            }
        }

        printf("%s [FRYZJER %d] Klient %d zapłacił %d zł za usługę kosztującą %d zł.\n", get_timestamp(), fryzjer_id, client_pid, payment, cost);

        // Przetwarzanie płatności
        lock_semaphore();
        process_payment(tens, twenties, fifties);
        unlock_semaphore();

        // Wydanie reszty
        int change = payment - cost;
        if (change > 0) {
            printf("%s [FRYZJER %d] Wydaję resztę: %d zł.\n", get_timestamp(), fryzjer_id, change);
            give_change(change);
        }

        // Zwolnienie fotela
        struct sembuf zwolnij_fotel = {0, 1, 0};
        if (semop(fotele_id, &zwolnij_fotel, 1) == -1) {
            perror("[FRYZJER] Błąd zwalniania fotela");
        }

        // Zaktualizuj pamięć współdzieloną: klient opuszcza fotel
        lock_semaphore();
        for (int i = 0; i < NUM_FOTELE; i++) {
            if (kasa->client_on_chair[i] == client_pid) { // Znajdź fotel zajęty przez klienta
                kasa->client_on_chair[i] = 0; // Zwolnij fotel
                break;
            }
        }
        unlock_semaphore();

        // Oznaczenie klienta jako obsłużonego
        lock_semaphore();
        kasa->client_done[client_id] = 1; // Ustaw flagę, że klient został obsłużony
        unlock_semaphore();

        printf("%s [FRYZJER %d] Klient %d obsłużony, fotel zwolniony.\n", get_timestamp(), fryzjer_id, client_pid);
    }
}