#define _POSIX_C_SOURCE 199309L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/prctl.h>
#include "utils.h"
#include "kierownik.h"
#include <pthread.h>
/* ma tworzyc kolejke komunikatow
    ma tworzyc semafory
    ma tworzyc pamiec dzielona
    ma sprawdzac godzine startu
    watek do symulaowania czasu
    ma tworzyc fryzjerow i klientow
    sygnal 1
    sygnal 2
    koniec symulacji
    zwalnianie zasobow
    wyswietla stan kasy
    */

pid_t fryzjer_pids[NUM_FRYZJEROW];
pid_t klient_pids[NUM_KLIENTOW];

int signal1 = 0;
int signal2 = 0;

void createFryzjer() {
    for (int i = 0; i < NUM_FRYZJEROW; i++) {
        fryzjer_pids[i] = fork();
        if (fryzjer_pids[i] == 0) {
            fryzjer_handler(i + 1);
            exit(0);
        }
        printf("%s Nowy fryzjer %d\n",get_timestamp(), fryzjer_pids[i]);
    }
}

void createKlient() {
    for (int i = 0; i < NUM_KLIENTOW; i++) {
        klient_pids[i] = fork();
        if (klient_pids[i] == 0) {
            klient_handler(i + 1);
            exit(0);
        }
        printf("%s Nowy klient %d\n", get_timestamp(), klient_pids[i]);
        int opoznienie = rand() % 3 + 1;
        sleep(opoznienie);
    }
}

void sendSignal1() {
    for (int i = 0; i < NUM_FRYZJEROW; i++) {
        kill(fryzjer_pids[i], 1);
    }
    waitProcesses(NUM_FRYZJEROW);
}

void sendSignal2() {
    for (int i = 0; i < NUM_KLIENTOW; i++) {
        kill(klient_pids[i], 1);
    }
    waitProcesses(NUM_KLIENTOW);
}

void endProgram(int signal) {
    endTimeSimulator();
    sendSignal1();
    sendSignal2();
    cleanResources();
    exit(EXIT_SUCCESS);
}

void kierownik_handler(pid_t fryzjer_pids[]) {
    prctl(PR_SET_NAME, "kierownik", 0, 0, 0);
    srand(time(NULL));

    initResources();
    initCash(memory);

    pthread_create(&timeSimulation, NULL, timeSimulator, (void *)NULL);

    createFryzjer();
    createKlient();

    struct timespec startTime, currentTime;
    clock_gettime(CLOCK_MONOTONIC, &startTime);
    int counter = 0;

    while(1) { 
        clock_gettime(CLOCK_MONOTONIC, &currentTime);
        double elapsed = currentTime.tv_sec - startTime.tv_sec;

        if(elapsed >= 5) {
            if (counter % 2 == 0) {
                printf("%s [KIEROWNIK] Wysylam sygnal do fryzjerow.\n", get_timestamp());
                sendSignal1();
            } else {
                printf("%s [KIEROWNIK] Wysylam sygnal do klientow.\n", get_timestamp());
                sendSignal2;
            }
            counter++;
            clock_gettime(CLOCK_MONOTONIC, &startTime);
        }
    }
    exit(EXIT_FAILURE);
}

