#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 500
#include "utils.h"
#include "klient.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <signal.h>

void handle_sigusr2(int sig) {
    if (sig == SIGUSR2) {
        printf("%s [KLIENT %d] Otrzymałem nakaz opuszczenia salonu.\n", get_timestamp(), getpid());
        // Zwalniamy miejsce w poczekalni jeśli klient tam był
        struct sembuf zwolnij_miejsce = {0, 1, 0};
        semop(poczekalnia_id, &zwolnij_miejsce, 1);
        exit(0);
    }
}

void klient_handler(int client_id) {
    prctl(PR_SET_NAME, "klient", 0, 0, 0);
    
    // Rejestracja obsługi sygnału z flagą SA_RESTART
    struct sigaction sa;
    sa.sa_handler = handle_sigusr2;
    sa.sa_flags = SA_RESTART;  // Dodajemy flagę SA_RESTART
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR2, &sa, NULL);
    
    usleep(rand() % 800000);

    // Próba zajęcia miejsca w poczekalni
    struct sembuf zajmij_miejsce = {0, -1, IPC_NOWAIT};
    if (semop(poczekalnia_id, &zajmij_miejsce, 1) == -1) {
        printf("%s [KLIENT %d] Poczekalnia pełna, klient nie wchodzi.\n", get_timestamp(), getpid());
        exit(0);
    }

    printf("%s [KLIENT %d] Zajął miejsce w poczekalni.\n", get_timestamp(), getpid());

    // Dodanie klienta do kolejki
    if (enqueue(client_id) == -1) {
        printf("%s [KLIENT %d] Błąd dodania do kolejki, klient opuszcza salon.\n", get_timestamp(), getpid());
        exit(0);
    }

    // Wysyłanie komunikatu do fryzjera
    struct Message message;
    message.mtype = 1; // Typ komunikatu (można dostosować do ID fryzjera)
    message.client_id = client_id;
    message.client_pid = getpid();

    if (msgsnd(msg_queue_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
        perror("Błąd wysyłania komunikatu do fryzjera");
        exit(EXIT_FAILURE);
    }

    // Oczekiwanie na obsługę
    while (1) {
        lock_semaphore();
        if (sharedMemory->client_done[client_id]) {
            unlock_semaphore();
            break;
        }
        unlock_semaphore();
        usleep(100000);
    }

    printf("%s [KLIENT %d] Klient opuszcza salon po obsłudze.\n", get_timestamp(), getpid());
    exit(0);
}