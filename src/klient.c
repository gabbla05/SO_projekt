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

        // Zwolnij miejsce w poczekalni, jeśli klient je zajmował
        struct sembuf zwolnij_miejsce = {0, 1, 0};
        semop(poczekalnia_id, &zwolnij_miejsce, 1);

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

    while (1) {
        // Sprawdź, czy salon jest otwarty
        //printf("%s [DEBUG KLIENT %d] PRZED LOCK_SEMAPHORE (SPRAWDZENIE SALONU)\n", get_timestamp(), getpid());
        lock_semaphore();
        //printf("%s [DEBUG KLIENT %d] PO LOCK_SEMAPHORE (SPRAWDZENIE SALONU)\n", get_timestamp(), getpid());
        int salon_otwarty = kasa->salon_open;
        //printf("%s [DEBUG KLIENT %d] PRZED UNLOCK_SEMAPHORE (SPRAWDZENIE SALONU)\n", get_timestamp(), getpid());
        unlock_semaphore();
        //printf("%s [DEBUG KLIENT %d] PO UNLOCK_SEMAPHORE (SPRAWDZENIE SALONU)\n", get_timestamp(), getpid());

        if (!salon_otwarty) {
            printf("%s [KLIENT %d] Salon jest zamknięty. Czekam na otwarcie.\n", get_timestamp(), getpid());
            sleep(1); // Czekaj 1 sekundę przed kolejną próbą
            continue;
        }
    

        struct sembuf zajmij_miejsce = {0, -1, IPC_NOWAIT};
        //printf("%s [DEBUG KLIENT %d] PRZED ZAJECIEM MIEJSCA W POCZEKALNI\n", get_timestamp(), getpid());
        if (semop(poczekalnia_id, &zajmij_miejsce, 1) == -1) {
            printf("%s [KLIENT %d] Poczekalnia pełna, klient nie wchodzi.\n",get_timestamp(), getpid());
            sleep(1); // Czekaj 1 sekundę przed kolejną próbą
            continue;
        }
        //printf("%s [DEBUG KLIENT %d] PO ZAJECIU MIEJSCA W POCZEKALNI\n", get_timestamp(), getpid());

        printf("%s [KLIENT %d] Zajął miejsce w poczekalni.\n",get_timestamp(), getpid());
        enqueue(getpid());

        // Wysyłanie komunikatu do fryzjera
        struct Message message;
        message.mtype = 1; // Typ komunikatu (można dostosować do ID fryzjera)
        message.client_id = client_id;
        message.client_pid = getpid();
        //printf("%s [DEBUG KLIENT %d] PRZED WYSŁANIEM KOMUNIKATU DO FRYZJERA\n", get_timestamp(), getpid());
        if (msgsnd(msg_queue_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
            perror("Błąd wysyłania komunikatu do fryzjera");
            exit(EXIT_FAILURE);
        }
        //printf("%s [DEBUG KLIENT %d] PO WYSŁANIU KOMUNIKATU DO FRYZJERA\n", get_timestamp(), getpid());

        while (1) {
            //printf("%s [DEBUG KLIENT %d] PRZED LOCK_SEMAPHORE (SPRAWDZENIE FLAGI)\n", get_timestamp(), getpid());
            lock_semaphore();
            //printf("%s [DEBUG KLIENT %d] PO LOCK_SEMAPHORE (SPRAWDZENIE FLAGI)\n", get_timestamp(), getpid());
            if (kasa->client_done[client_id]) { 
                    //printf("XDDDDDDDD");
                //printf("%s [DEBUG KLIENT %d] PRZED UNLOCK_SEMAPHORE W IF(SPRAWDZENIE FLAGI)\n", get_timestamp(), getpid());
                unlock_semaphore();
                //printf("%s [DEBUG KLIENT %d] PO UNLOCK_SEMAPHORE W IF (SPRAWDZENIE FLAGI)\n", get_timestamp(), getpid());
                break;
            }
            //printf("%s [DEBUG KLIENT %d] PRZED UNLOCK_SEMAPHORE (SPRAWDZENIE FLAGI)\n", get_timestamp(), getpid());
                unlock_semaphore();
            //printf("%s [DEBUG KLIENT %d] PO UNLOCK_SEMAPHORE (SPRAWDZENIE FLAGI)\n", get_timestamp(), getpid());
            usleep(100000);
        }

        printf("%s [KLIENT %d] Klient opuszcza salon po obsłudze.\n",get_timestamp(), getpid());

        struct sembuf zwolnij_miejsce = {0, 1, 0};
        //printf("%s [DEBUG KLIENT %d] PRZED ZWOLNIENIEM MIEJSCA W POCZEKALNI\n", get_timestamp(), getpid());
        semop(poczekalnia_id, &zwolnij_miejsce, 1);
        //printf("%s [DEBUG KLIENT %d] PO ZWOLNIENIU MIEJSCA W POCZEKALNI\n", get_timestamp(), getpid());
        sleep(1);
    }

}