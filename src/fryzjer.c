
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/prctl.h>
#include <sys/shm.h>
#include <errno.h>

extern int shm_id;
extern struct Kasa* kasa;
extern pid_t main_process_pid;
extern volatile sig_atomic_t fryzjer_active[NUM_FRYZJEROW];


void fryzjer_handler(int fryzjer_id) {

    prctl(PR_SET_NAME, "fryzjer", 0, 0, 0); // Ustaw nazwę procesu
    srand(time(NULL) + fryzjer_id);
    pid_t pid = getpid();

    while (1) {

        
        // Sprawdzenie, czy salon jest otwarty
        lock_semaphore();
        if (kasa->salon_open == 0) {
            unlock_semaphore();
            printf("%s [PID %d] Fryzjer %d kończy pracę, salon jest zamknięty.\n",
                   get_process_role(), pid, fryzjer_id);
            exit(0);
        }
        unlock_semaphore();

        /// Oczekiwanie na sygnał od klienta
        struct sembuf czekaj_na_klienta = {0, -1, 0};
        while (semop(fryzjer_signal_id, &czekaj_na_klienta, 1) == -1) {
            if (errno == EINTR) {
                continue; // Ponawiaj, jeśli przerwane przez sygnał
            }
            perror("[FRYZJER] Błąd oczekiwania na sygnał klienta");
            exit(EXIT_FAILURE);
        }


        lock_semaphore();
        (*sleeping_fryzjer)--; // Zmniejszenie licznika śpiących fryzjerów
        unlock_semaphore();
        printf("[FRYZJER %d] Liczba śpiących fryzjerów: %d\n", fryzjer_id, *sleeping_fryzjer);

        int client_id = dequeue();
        if (client_id == -1) {
            lock_semaphore();
            (*sleeping_fryzjer)++;
            unlock_semaphore();
            printf("[FRYZJER %d] Brak klientów, fryzjer śpi...\n", fryzjer_id);
            usleep(1000000);
            continue;
        }
        

        
        // Próba zajęcia fotela
        struct sembuf zajmij_fotel = {0, -1, 0};
        if (semop(fotele_id, &zajmij_fotel, 1) == -1) {
            perror("[FRYZJER] Błąd zajmowania fotela");
            continue;
        }

        printf("%s [PID %d] Fryzjer %d zajmuje fotel dla klienta %d.\n",
               get_process_role(), pid, fryzjer_id, client_id);

        if (client_id != -1) {
            int service_cost = (rand() % 10 + 1) * 10;
            int payment = 0, tens = 0, twenties = 0, fifties = 0;

            // Generowanie płatności
            while (payment < service_cost) {
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

            printf("%s [PID %d] Obsługuje klienta %d. Usługa kosztuje %d zł, klient płaci %d zł.\n",
                   get_process_role(), pid, client_id, service_cost, payment);
            printf("%s [PID %d] Płatność klienta %d: %d x 10 zł, %d x 20 zł, %d x 50 zł.\n",
                   get_process_role(), pid, client_id, tens, twenties, fifties);

            process_payment(tens, twenties, fifties); // Przetwarzanie płatności

            int change = payment - service_cost;
            if (change > 0) {
                printf("%s [PID %d] Wydaje resztę %d zł klientowi %d.\n", get_process_role(), pid, change, client_id);
                give_change(change); // Wydanie reszty
            }

            printf("%s [PID %d] Wykonuje usługę dla klienta %d.\n", get_process_role(), pid, client_id);
            usleep(rand() % 1000000 + 1000000); // Symulacja czasu obsługi
            printf("%s [PID %d] Zakończył obsługę klienta %d.\n", get_process_role(), pid, client_id);

            lock_semaphore();
            kasa->client_done[client_id] = 1; // Zaznacz, że klient został obsłużony
            unlock_semaphore();

            // Zwolnienie fotela
            struct sembuf zwolnij_fotel = {0, 1, 0}; // Zwolnienie fotela
            if (semop(fotele_id, &zwolnij_fotel, 1) == -1) {
                perror("Błąd zwalniania fotela");
                exit(EXIT_FAILURE);
            }

            printf("%s [PID %d] Fryzjer %d zwalnia fotel dla klienta %d.\n",
               get_process_role(), pid, fryzjer_id, client_id);

        } else {
            printf("%s [PID %d] Brak klientów, fryzjer %d śpi...\n", get_process_role(), pid, fryzjer_id);
            usleep(1000000); // Fryzjer czeka na klienta
        }
    }
}