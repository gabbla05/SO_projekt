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
        printf("%s [KLIENT %d] Otrzymałem sygnał do natychmiastowego opuszczenia salonu.\n", get_timestamp(), getpid());
        exit(0); // Klient natychmiast opuszcza salon
    }
}

void klient_handler(int client_id) {
    prctl(PR_SET_NAME, "klient", 0, 0, 0);
    srand(getpid());

    struct sigaction sa;
    sa.sa_handler = handle_sigusr2;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("Błąd rejestracji SIGUSR2");
        exit(EXIT_FAILURE);
    }
    
    usleep(rand() % 500000);

    struct sembuf zajmij_miejsce = {0, -1, IPC_NOWAIT};
    if (semop(poczekalnia_id, &zajmij_miejsce, 1) == -1) {
        printf("%s [KLIENT %d] Poczekalnia pełna, klient nie wchodzi.\n",get_timestamp(), getpid());
        exit(0);
    }

    printf("%s [KLIENT %d] Zajął miejsce w poczekalni.\n",get_timestamp(), getpid());
    enqueue(getpid());

    // Wysyłanie komunikatu do fryzjera
    struct Message message;
    message.mtype = 1; // Typ komunikatu (można dostosować do ID fryzjera)
    message.client_id = client_id;
    message.client_pid = getpid();

    if (msgsnd(msg_queue_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
        perror("Błąd wysyłania komunikatu do fryzjera");
        exit(EXIT_FAILURE);
    }

    while (1) {
        lock_semaphore();
        if (kasa->client_done[client_id]) { 
            //printf("XDDDDDDDD");
            unlock_semaphore();
            break;
        }
        unlock_semaphore();
        usleep(100000);
    }

    printf("%s [KLIENT %d] Klient opuszcza salon po obsłudze.\n",get_timestamp(), getpid());

    struct sembuf zwolnij_miejsce = {0, 1, 0};
    semop(poczekalnia_id, &zwolnij_miejsce, 1);

    exit(0);
}