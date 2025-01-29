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
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/resource.h>

pthread_t keyboardThread;
pthread_t timeSimulation;

pid_t fryzjer_pids[NUM_FRYZJEROW];
pid_t klient_pids[NUM_KLIENTOW];

int signal1 = 0;
int signal2 = 0;

int salon_open;
int hour = 8;

int poczekalnia;
int fotele;
int kasa;
int messageQueue;
int sharedMemoryId;
int *memory;

// Funkcja sprawdzajaca limit procesow.
void processesLimit() {
    struct rlimit rl;
    if(getrlimit(RLIMIT_NPROC, &rl) != 0){
        perror("Blad getrlimit");
        exit(EXIT_FAILURE);
    }
    if (rl.rlim_cur < NUM_FRYZJEROW + NUM_KLIENTOW) {
        fprintf(stderr, "Osiagnieto limit %ld procesow.", rl.rlim_cur);
    }
    
}

// Funkcja tworzaca kolejke komunikatow, semafory, pamiec wspoldzielona
void initResources() {
    key_t key;

    key = ftok(KEY_PATH, KEY_CHAR_MESSAGEQUEUE);
    messageQueue = createMessageQueue(key);

    key = ftok(KEY_PATH, KEY_CHAR_KASA);
    kasa = createSemaphore(key);
    setSemaphore(kasa, 1);

    key = ftok(KEY_PATH, KEY_CHAR_FOTELE);
    fotele = createSemaphore(key);
    setSemaphore(fotele, NUM_FOTELE);

    key = ftok(KEY_PATH, KEY_CHAR_POCZEKALNIA);
    poczekalnia = createSemaphore(key);
    setSemaphore(poczekalnia, MAX_QUEUE_SIZE);

    key = ftok(KEY_PATH, KEY_CHAR_SHAREDMEMORY);
    sharedMemoryId = createSharedMemory(key);
    memory = attachSharedMemory(sharedMemoryId);
}

// Funkcja inicjujaca poczatkowy stan kasy.
void initCash(int *memory) {
    if (memory == NULL) {
        perror("Pamiec wspoldzielona nie zostala poprawnie zainicjowana.");
        return;
    }
    memory[0] = 10;
    memory[1] = 5;
    memory[2] = 2;
    printf("%s Stan poczatkowy kasy: 10 x 10zl, 5 x 20zl, 2 x 50zl.\n", get_timestamp());
}

// Funkcja czyszczaca wszystkie zasoby.
void cleanResources() {
    deleteMessageQueue(messageQueue);
    deleteSemaphore(kasa);
    deleteSemaphore(fotele);
    deleteSemaphore(poczekalnia);
    detachSharedMemory(memory);
    deleteSharedMemory(sharedMemoryId);
    printf("%s Zasoby zostaly zwolnione.\n", get_timestamp());
}

// Funkcja tworzaca fryzjerow.
void createFryzjer() {
    for (int i = 0; i < NUM_FRYZJEROW; i++) {
        fryzjer_pids[i] = fork();
        if (fryzjer_pids[i] == 0) {
            execl("./fryzjer", "fryzjer", NULL);
        }
        printf("%s [KIEROWNIK] Nowy fryzjer %d\n",get_timestamp(), fryzjer_pids[i]);
    }
}

// Funkcja tworzaca klientow.
void createKlient() {
    for (int i = 0; i < NUM_KLIENTOW; i++) {
        klient_pids[i] = fork();
        if (klient_pids[i] == 0) {
            execl("./klient", "klient", NULL);
        }
        printf("%s [KIEROWNIK] Nowy klient %d\n", get_timestamp(), klient_pids[i]);
        int opoznienie = rand() % 3 + 1;
        sleep(opoznienie);
    }
}

// Watek do obslugi symulacji czasu.
void *timeSimulator(void *arg) {
    while (1) {
        sleep(1);

        hour++;

        if (hour == 8) {
            printf("%s [KIEROWNIK] Godzina 8:00. Salon zostal otwarty.\n", get_timestamp());
            salon_open = 1;
            createFryzjer();
            increaseSemaphore(poczekalnia, MAX_QUEUE_SIZE);
        }

        if (hour == 18) {
            printf("%s [KIEROWNIK] Godzina 18:00. Salon sie zamyka.\n", get_timestamp());
            salon_open = 0;
            decreaseSemaphore(poczekalnia, MAX_QUEUE_SIZE);
            sendSignal2();
            sendSignal1();
        }

        if (hour == 24) {
            hour = 0;
        }

        if (hour % 2 == 0) {
            printf("%s [KIEROWNIK] Aktualna godzina: %d.\n",get_timestamp(), hour);
        }

    }
    return NULL;
}

// Koniec symulacji czasu.
void endTimeSimulator() {
    pthread_cancel(timeSimulation);
    pthread_join(timeSimulation, NULL);
}

// Sygnal 1.
void sendSignal1() {
    for (int i = 0; i < NUM_FRYZJEROW; i++) {
        kill(fryzjer_pids[i], 1);
    }
    waitProcesses(NUM_FRYZJEROW);
}

//Sygnal 2
void sendSignal2() {
    for (int i = 0; i < NUM_KLIENTOW; i++) {
        kill(klient_pids[i], 1);
    }
    waitProcesses(NUM_KLIENTOW);
}

// Czekanie na zakonczenie procesow potomnych.
void waitProcesses(int a) {
    int pid;
    int status;
    for (int i = 0; i < a; i++) {
        pid = wait(&status);
        if(pid == -1) {
            perror("Blad w funkcji wait.");
            exit(EXIT_FAILURE);
        } 
    }
}

// Funkcja do zakonczenia programu.
void endProgram(int signal) {
    endTimeSimulator();
    sendSignal1();
    sendSignal2();
    cleanResources();
    exit(EXIT_SUCCESS);
}

// Watek do obslugi inputu z klawiatury - jesli uzytkownik wpisze q lub Q program sie konczy.
void *keyboardListener(void *arg) {
    char input;
    while (1) {
        input = getchar();
        if (input == 'q' || input == 'Q') {
            printf("%s Koncze dzialanie programu...\n", get_timestamp());
            endProgram(SIGINT);
        }
    }
    return NULL;
}

int main() {
    processesLimit();
    srand(time(NULL));

    signal(SIGINT, endProgram);
    signal(SIGTERM, endProgram);

    initResources();
    initCash(memory);

    pthread_create(&keyboardThread, NULL, keyboardListener, NULL);
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
                sendSignal2();
            }
            counter++;
            clock_gettime(CLOCK_MONOTONIC, &startTime);
        }
    }
    exit(EXIT_FAILURE);
}

