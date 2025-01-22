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
    usleep(rand() % 500000);

    struct sembuf zajmij_miejsce = {0, -1, IPC_NOWAIT};
    if (semop(poczekalnia_id, &zajmij_miejsce, 1) == -1) {
        printf("%s [KLIENT %d] Poczekalnia pełna, klient nie wchodzi.\n",get_timestamp(), client_id);
        exit(0);
    }

    printf("%s [KLIENT %d] Zajął miejsce w poczekalni.\n",get_timestamp(), client_id);
    enqueue(client_id);

    // Wysyłanie komunikatu do fryzjera
    struct Message message;
    message.mtype = 1; // Typ komunikatu (można dostosować do ID fryzjera)
    message.client_id = client_id;

    if (msgsnd(msg_queue_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
        perror("Błąd wysyłania komunikatu do fryzjera");
        exit(EXIT_FAILURE);
    }

    while (1) {
        lock_semaphore();
        if (kasa->client_done[client_id]) { // to sie nigdy nie wykonuje
            printf("XDDDDDDDD");
            unlock_semaphore();
            break;
        }
        unlock_semaphore();
        usleep(100000);
    }

    printf("%s [KLIENT %d] Klient opuszcza salon po obsłudze.\n",get_timestamp(), client_id);

    struct sembuf zwolnij_miejsce = {0, 1, 0};
    semop(poczekalnia_id, &zwolnij_miejsce, 1);

    exit(0);
}