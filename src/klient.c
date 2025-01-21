
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/prctl.h>

extern pid_t main_process_pid;

void klient_handler(int client_id) {

    prctl(PR_SET_NAME, "klient", 0, 0, 0); // Ustaw nazwę procesu
    srand(getpid()); // Ziarno losowania zależne od PID

    pid_t pid = getpid(); // Pobierz PID klienta
    usleep(rand() % 500000); // Losowe opóźnienie przy przyjściu klientów

    // Próba zajęcia miejsca w poczekalni
    struct sembuf zajmij_miejsce = {0, -1, 0};
    if (semop(poczekalnia_id, &zajmij_miejsce, 1) == -1) {
        printf("[KLIENT %d] Poczekalnia pełna, klient %d opuszcza salon.\n", pid, client_id);
        exit(0);
    }

    printf("[KLIENT %d] Klient %d zajął miejsce w poczekalni.\n", pid, client_id);

    lock_semaphore();
    enqueue(client_id); // Dodanie klienta do kolejki
    unlock_semaphore();

    // Budzenie fryzjera tylko, jeśli żaden nie jest zajęty
    if (*sleeping_fryzjer > 0) {
        struct sembuf budz_fryzjera = {0, 1, 0};
        semop(fryzjer_signal_id, &budz_fryzjera, 1);
        printf("[KLIENT %d] Budzę fryzjera.\n", client_id);
    }


    // Czekanie na obsługę
    while (1) {
        lock_semaphore();
        if (kasa->client_done[client_id] == 1) {
            unlock_semaphore();
            break;
        }
        unlock_semaphore();
        usleep(100000); // Odczekaj przed kolejną próbą
    }

    printf("[KLIENT %d] Klient %d opuścił salon po obsłudze.\n", pid, client_id);

    // Zwolnienie miejsca w poczekalni
    struct sembuf zwolnienie = {0, 1, 0};
    if (semop(poczekalnia_id, &zwolnienie, 1) == -1) {
        perror("Błąd zwalniania miejsca w poczekalni");
    }

    exit(0);
}

