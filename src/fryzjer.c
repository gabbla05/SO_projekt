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
        while (1) {
            lock_semaphore();
            if (kasa->salon_open == 1) {
                unlock_semaphore();
                break; // Salon został otwarty, fryzjer wraca do pracy
            }
            unlock_semaphore();
            sleep(1); // Czekaj 1 sekundę przed kolejnym sprawdzeniem
        }
        printf("%s [FRYZJER %d] Salon został otwarty. Wracam do pracy.\n", get_timestamp(), getpid());    }
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
        //printf("%s [DEBUG FRYZJER %d] PRZED LOCK_SEMAPHORE (SPRAWDZENIE SALONU)\n", get_timestamp(), fryzjer_id);
        lock_semaphore();
        //printf("%s [DEBUG FRYZJER %d] PO LOCK_SEMAPHORE (SPRAWDZENIE SALONU)\n", get_timestamp(), fryzjer_id);
        if (kasa->salon_open == 0) {
            //printf("%s [DEBUG FRYZJER %d] PRZED UNLOCK_SEMAPHORE W IF (SPRAWDZENIE SALONU)\n", get_timestamp(), fryzjer_id);
            unlock_semaphore();
            //printf("%s [DEBUG FRYZJER %d] PRZED UNLOCK_SEMAPHORE W IF(SPRAWDZENIE SALONU)\n", get_timestamp(), fryzjer_id);
            printf("%s [FRYZJER %d] Salon jest zamknięty. Kończę pracę.\n", get_timestamp(), fryzjer_id);
            exit(0);
        }

        //printf("%s [DEBUG FRYZJER %d] PRZED UNLOCK_SEMAPHORE (SPRAWDZENIE SALONU)\n", get_timestamp(), fryzjer_id);
        unlock_semaphore();
        //printf("%s [DEBUG FRYZJER %d] PO  UNLOCK_SEMAPHORE (SPRAWDZENIE SALONU)\n", get_timestamp(), fryzjer_id);
        
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
        //printf("%s [DEBUG FRYZJER %d] PRZED ZWOLNIENIEM MIEJSCA W POCZEKALNI\n", get_timestamp(), fryzjer_id);
        if (semop(poczekalnia_id, &zwolnij_miejsce, 1) == -1) {
            perror("[FRYZJER] Błąd zwalniania miejsca w poczekalni");
        }
        //printf("%s [DEBUG FRYZJER %d] PO ZWOLNIENIU MIEJSCA W POCZEKALNI\n", get_timestamp(), fryzjer_id);
        
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
        //printf("%s [DEBUG FRYZJER %d] PRZED LOCK_SEMAPHORE (PRZETWARZANIE PŁATNOŚCI)\n", get_timestamp(), fryzjer_id);
        lock_semaphore();
        //printf("%s [DEBUG FRYZJER %d] PO LOCK_SEMAPHORE (PRZETWARZANIE PŁATNOŚCI)\n", get_timestamp(), fryzjer_id);
        process_payment(tens, twenties, fifties);
        //printf("%s [DEBUG FRYZJER %d] PRZED LOCK_SEMAPHORE (PRZETWARZANIE PŁATNOŚCI)\n", get_timestamp(), fryzjer_id);
        unlock_semaphore();
        //printf("%s [DEBUG FRYZJER %d] PO UNLOCK_SEMAPHORE (PRZETWARZANIE PŁATNOŚCI)\n", get_timestamp(), fryzjer_id);
        
        // Zajęcie fotela
        struct sembuf zajmij_fotel = {0, -1, 0};
        //printf("%s [DEBUG FRYZJER %d] PRZED ZAJECIEM FOTELA\n", get_timestamp(), fryzjer_id);
        while (semop(fotele_id, &zajmij_fotel, 1) == -1) {
            if (errno == EINTR) {
                // Operacja została przerwana przez sygnał, ponów próbę
                continue;
            }
            perror("[FRYZJER] Błąd zajmowania fotela");
            break; // Wyjdź z pętli w przypadku innych błędów
        }
        //printf("%s [DEBUG FRYZJER %d] PO ZAJECIU FOTELA\n", get_timestamp(), fryzjer_id);

        // Zaktualizuj pamięć współdzieloną: klient zajmuje fotel
        //printf("%s [DEBUG FRYZJER %d] PRZED LOCK_SEMAPHORE (AKTUALIZACJA FOTELA)\n", get_timestamp(), fryzjer_id);
        lock_semaphore();
        //printf("%s [DEBUG FRYZJER %d] PO LOCK_SEMAPHORE (AKTUALIZACJA FOTELA)\n", get_timestamp(), fryzjer_id);
        for (int i = 0; i < NUM_FOTELE; i++) {
            if (kasa->client_on_chair[i] == 0) { // Znajdź wolny fotel
                kasa->client_on_chair[i] = client_pid; // Zajmij fotel przez klienta
                break;
            }
        }
        //printf("%s [DEBUG FRYZJER %d] PRZED UNLOCK_SEMAPHORE (AKTUALIZACJA FOTELA)\n", get_timestamp(), fryzjer_id);
        unlock_semaphore();
        //printf("%s [DEBUG FRYZJER %d] PO UNLOCK_SEMAPHORE (AKTUALIZACJA FOTELA)\n", get_timestamp(), fryzjer_id);

        printf("%s [FRYZJER %d] Zajęto fotel dla klienta %d.\n", get_timestamp(), fryzjer_id, client_pid);

        // Rozpoczęcie obsługi klienta
        printf("%s [FRYZJER %d] Rozpoczynam obsługę klienta %d.\n", get_timestamp(), fryzjer_id, client_pid);
        sleep(rand() % 10 + 1); // Symulacja czasu obsługi

        // Symulacja czasu obsługi z regularnym sprawdzaniem flagi continueFlag
        int time_elapsed = 0;
        int total_time = rand() % 1000000 + 1000000; // Całkowity czas obsługi
        while (time_elapsed < total_time) {
            usleep(100000); // Czekaj 100 ms
            time_elapsed += 100000;

            // Sprawdź, czy klient został przerwany sygnałem SIGUSR2
            //printf("%s [DEBUG FRYZJER %d] PRZED LOCK_SEMAPHORE (SPRAWDZENIE FLAGI)\n", get_timestamp(), fryzjer_id);
            lock_semaphore();
            //printf("%s [DEBUG FRYZJER %d] PO LOCK_SEMAPHORE (SPRAWDZENIE FLAGI)\n", get_timestamp(), fryzjer_id);
            if (kasa->continueFlag == 1 || kasa->salon_open == 0) {
                unlock_semaphore();
                //printf("%s [DEBUG FRYZJER %d] PO UNLOCK_SEMAPHORE W IF (SPRAWDZENIE FLAGI)\n", get_timestamp(), fryzjer_id);
                printf("%s [FRYZJER %d] Klient %d został przerwany sygnałem SIGUSR2. Przerywam obsługę.\n", get_timestamp(), fryzjer_id, client_pid);
                break; // Przerwij obsługę klienta
            }
            //printf("%s [DEBUG FRYZJER %d] PRZED UNLOCK_SEMAPHORE (SPRAWDZENIE FLAGI)\n", get_timestamp(), fryzjer_id);
            unlock_semaphore();
            //printf("%s [DEBUG FRYZJER %d] PO UNLOCK_SEMAPHORE (SPRAWDZENIE FLAGI)\n", get_timestamp(), fryzjer_id);
        }

        // Jeśli klient został przerwany, zakończ obsługę
        //printf("%s [DEBUG FRYZJER %d] PRZED LOCK_SEMAPHORE (SPRAWDZENIE FLAGI PO OBSŁUDZE)\n", get_timestamp(), fryzjer_id);
        lock_semaphore();
        //printf("%s [DEBUG FRYZJER %d] PO LOCK_SEMAPHORE (SPRAWDZENIE FLAGI PO OBSŁUDZE)\n", get_timestamp(), fryzjer_id);
        if (kasa->continueFlag == 1) {
            //printf("%s [DEBUG FRYZJER %d] PRZED UNLOCK_SEMAPHORE W IF(SPRAWDZENIE FLAGI PO OBSŁUDZE)\n", get_timestamp(), fryzjer_id);
            unlock_semaphore();
            //printf("%s [DEBUG FRYZJER %d] PO UNLOCK_SEMAPHORE W IF (SPRAWDZENIE FLAGI PO OBSŁUDZE)\n", get_timestamp(), fryzjer_id);
            continue; // Przerwij obsługę klienta
        }
        //printf("%s [DEBUG FRYZJER %d] PRZED UNLOCK_SEMAPHORE W IF(SPRAWDZENIE FLAGI PO OBSŁUDZE)\n", get_timestamp(), fryzjer_id);
        unlock_semaphore();
        //printf("%s [DEBUG FRYZJER %d] PO UNLOCK_SEMAPHORE W IF(SPRAWDZENIE FLAGI PO OBSŁUDZE)\n", get_timestamp(), fryzjer_id);
        
       

        // Wydanie reszty
        int change = payment - cost;
        if (change > 0) {
            printf("%s [FRYZJER %d] Wydaję resztę: %d zł.\n", get_timestamp(), fryzjer_id, change);
            give_change(change);
        }

        // Zwolnienie fotela
        struct sembuf zwolnij_fotel = {0, 1, 0};
        //printf("%s [DEBUG FRYZJER %d] PRZED ZWOLNIENIEM FOTELA\n", get_timestamp(), fryzjer_id);
        if (semop(fotele_id, &zwolnij_fotel, 1) == -1) {
            perror("[FRYZJER] Błąd zwalniania fotela");
        }
        //printf("%s [DEBUG FRYZJER %d] PO ZWOLNIENIU FOTELA\n", get_timestamp(), fryzjer_id);

        // Zaktualizuj pamięć współdzieloną: klient opuszcza fotel
        //printf("%s [DEBUG FRYZJER %d] PRZED LOCK_SEMAPHORE (ZWOLNIENIE FOTELA)\n", get_timestamp(), fryzjer_id);
        lock_semaphore();
        //printf("%s [DEBUG FRYZJER %d] PO LOCK_SEMAPHORE (ZWOLNIENIE FOTELA)\n", get_timestamp(), fryzjer_id);
        for (int i = 0; i < NUM_FOTELE; i++) {
            if (kasa->client_on_chair[i] == client_pid) { // Znajdź fotel zajęty przez klienta
                kasa->client_on_chair[i] = 0; // Zwolnij fotel
                break;
            }
        }
        //printf("%s [DEBUG FRYZJER %d] PRZED UNLOCK_SEMAPHORE (ZWOLNIENIE FOTELA)\n", get_timestamp(), fryzjer_id);
        unlock_semaphore();
        //printf("%s [DEBUG FRYZJER %d] PO UNLOCK_SEMAPHORE (ZWOLNIENIE FOTELA)\n", get_timestamp(), fryzjer_id);

        // Oznaczenie klienta jako obsłużonego
        //printf("%s [DEBUG FRYZJER %d] PRZED LOCK_SEMAPHORE (OZNACZENIE KLIENTA)\n", get_timestamp(), fryzjer_id);
        lock_semaphore();
        //printf("%s [DEBUG FRYZJER %d] PO LOCK_SEMAPHORE (OZNACZENIE KLIENTA)\n", get_timestamp(), fryzjer_id);
        kasa->client_done[client_id] = 1; // Ustaw flagę, że klient został obsłużony
        //printf("%s [DEBUG FRYZJER %d] PRZED UNLOCK_SEMAPHORE (OZNACZENIE KLIENTA)\n", get_timestamp(), fryzjer_id);
        unlock_semaphore();
        //printf("%s [DEBUG FRYZJER %d] PO UNLOCK_SEMAPHORE (OZNACZENIE KLIENTA)\n", get_timestamp(), fryzjer_id);

        printf("%s [FRYZJER %d] Klient %d obsłużony, fotel zwolniony.\n", get_timestamp(), fryzjer_id, client_pid);
    }
}