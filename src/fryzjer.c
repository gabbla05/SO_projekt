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
extern struct SharedMemory* sharedMemory;

// Funkcja obsługi sygnału SIGUSR1
void handle_sigusr1_hairdresser(int sig) {
    if (sig == SIGUSR1) {
        printf("%s [FRYZJER %d] Otrzymałem sygnał do wyłączenia. Kończę pracę.\n", get_timestamp(), getpid());
        exit(0); // Zakończ proces fryzjera
    }
}

void handle_sigusr2_hairdresser(int sig) {
    if (sig == SIGUSR2) {
        printf("%s [FRYZJER %d] Otrzymałem sygnał do przerwania obsługi.\n", get_timestamp(), getpid());
        // Nie kończymy procesu fryzjera, tylko przerywamy obsługę
    }
}

void fryzjer_handler(int fryzjer_id) {
    prctl(PR_SET_NAME, "fryzjer", 0, 0, 0);
    srand(getpid());

    // Rejestracja obsługi obu sygnałów
    struct sigaction sa1, sa2;
    
    sa1.sa_handler = handle_sigusr1_hairdresser;
    sa1.sa_flags = 0;
    sigemptyset(&sa1.sa_mask);
    sigaction(SIGUSR1, &sa1, NULL);

    sa2.sa_handler = handle_sigusr2_hairdresser;
    sa2.sa_flags = 0;
    sigemptyset(&sa2.sa_mask);
    sigaction(SIGUSR2, &sa2, NULL);

    struct Message message;
    volatile sig_atomic_t interrupted = 0;

    while (1) {
        // Sprawdzenie, czy salon jest otwarty
        if (sharedMemory->salon_open == 0) continue;

        // Oczekiwanie na klienta w poczekalni
       // printf("%s [FRYZJER %d] Oczekuję na klienta w poczekalni.\n", get_timestamp(), fryzjer_id);

        // Próba odbioru komunikatu z kolejki
        if (msgrcv(msg_queue_id, &message, sizeof(message) - sizeof(long), 1, IPC_NOWAIT) == -1) {
            if (errno == ENOMSG) {
                usleep(100000);
                continue;
            }
            // Obsługa innych błędów...
            continue;
        }

        int client_id = message.client_id;
        int client_pid = message.client_pid;

        // Obsługa klienta z możliwością przerwania
        printf("%s [FRYZJER %d] Rozpoczynam obsługę klienta %d.\n", get_timestamp(), fryzjer_id, client_pid);
        
        lock_semaphore();
        // Sprawdź, czy klient nadal jest aktywny (czy nie został zmuszony do wyjścia)
        if (kill(client_pid, 0) == -1) {
            printf("%s [FRYZJER %d] Klient %d nie jest już dostępny.\n", get_timestamp(), fryzjer_id, client_pid);
            unlock_semaphore();
            continue;
        }

        // Zajmij fotel i oznacz klienta
        for (int i = 0; i < NUM_FOTELE; i++) {
            if (sharedMemory->client_on_chair[i] == 0) {
                sharedMemory->client_on_chair[i] = client_pid;
                break;
            }
        }
        unlock_semaphore();

        // Symulacja strzyżenia z możliwością przerwania
        int strzyzenie_czas = rand() % 2000000 + 2000000;
        int czas_miniety = 0;
        while (czas_miniety < strzyzenie_czas) {
            usleep(100000); // Śpij krócej, by sprawdzać przerwania
            czas_miniety += 100000;
            
            // Sprawdź, czy klient nadal jest dostępny
            if (kill(client_pid, 0) == -1) {
                interrupted = 1;
                break;
            }
        }

        // Jeśli obsługa została przerwana, wyczyść stan i kontynuuj
        if (interrupted) {
            printf("%s [FRYZJER %d] Obsługa klienta %d została przerwana.\n", 
                   get_timestamp(), fryzjer_id, client_pid);
            
            lock_semaphore();
            // Znajdź i zwolnij fotel klienta
            for (int i = 0; i < NUM_FOTELE; i++) {
                if (sharedMemory->client_on_chair[i] == client_pid) {
                    sharedMemory->client_on_chair[i] = 0;
                    break;
                }
            }
            unlock_semaphore();
            
            struct sembuf zwolnij_fotel = {0, 1, 0};
            semop(fotele_id, &zwolnij_fotel, 1);
            
            interrupted = 0;
            continue;
        }

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
            if (sharedMemory->client_on_chair[i] == client_pid) { // Znajdź fotel zajęty przez klienta
                sharedMemory->client_on_chair[i] = 0; // Zwolnij fotel
                break;
            }
        }
        unlock_semaphore();

        // Oznaczenie klienta jako obsłużonego
        lock_semaphore();
        sharedMemory->client_done[client_id] = 1; // Ustaw flagę, że klient został obsłużony
        unlock_semaphore();

        printf("%s [FRYZJER %d] Klient %d obsłużony, fotel zwolniony.\n", get_timestamp(), fryzjer_id, client_pid);
    }
}