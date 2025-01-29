#include "fryzjer.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/prctl.h>
#include <errno.h>
#include <signal.h>

volatile sig_atomic_t gotSignal1 = 0;
volatile sig_atomic_t gotMessageP = 0;
volatile sig_atomic_t chairOccupied = 0;
volatile sig_atomic_t cashOccupied = 0;
volatile sig_atomic_t priceSend = 0;
volatile sig_atomic_t gotMoney = 0;
volatile sig_atomic_t changeSend = 0;

long fryzjer_id;
int *memory;
int messageQueue;
int fotele;
int kasa;
int memory_id;

// Funkcja obsługi sygnału SIGUSR1
void handleSignal1(int sig) {
    printf("%s [FRYZJER %ld] Otrzymałem sygnał do wyłączenia. Kończę pracę.\n", get_timestamp(), getpid());
    if(gotMessageP) {
        gotSignal1 = 1;
        if (priceSend != 1) {
            priceSend = -1;
        } else if (gotMoney != 1) {
            gotMoney = -1;
        } else if (changeSend != 1) {
            changeSend = -1;
        } else {
            cleanResourcesFryzjer();
            printf("%s [FRYZJER %ld] Koncze prace.\n", get_timestamp(), getpid());
            exit(0);
        }
    }
}

void cleanResourcesFryzjer() {
    if (chairOccupied) {
        increaseSemaphore(fotele, 1);
    }

    if(cashOccupied) {
        increaseSemaphore(kasa, 1);
    }
    detachSharedMemory(memory);
}

void fryzjer_handler(int fryzjer_id) {
    prctl(PR_SET_NAME, "fryzjer", 0, 0, 0);
    srand(time(NULL));
    fryzjer_id = getpid();
    
    // Rejestracja obsługi sygnału SIGUSR1
    if(signal(SIGHUP, handleSignal1) == SIG_ERR) {
        perror("Blad sygnalu 1.");
        exit(EXIT_FAILURE);
    }

    struct message msg;

    initResources();

    while (1) {
       if(gotSignal1) break;

       if (gotMessageP != 1) {
            recieveMessage(messageQueue, &msg, MESSAGE_P);
            gotMessageP = 1;
       }

       client_id = msg.pid;

       if(!chairOccupied) {
            decreaseSemaphore(fotele, 1);
            chairOccupied = 1;
       }

       int cost = (rand() % 9 + 4) * 10;

       printf("%s [FRYZJER %ld]: Cena uslugi dla klienta %d: %d\n", get_timestamp(),getpid(), client_id, cost);
       msg.mtype = client_id;
       msg.pid = getpid();
       msg.message[0] = cost;

       if(priceSend != 1) {
            sendMessage(messageQueue, &msg);
            priceSend = 1;
       }

       if (gotMoney != 1) {
            recieveMessage(messageQueue, &msg, getpid());
            gotMoney = 1;
       }

        int change = processPayment(client_id, cost,  messageQueue, memory, kasa);

        printf("%s [FRYZJER %ld] Obsluguje klienta %ld\n",get_timestamp(), getpid(), client_id);
        sleep(SERVICE_TIME);

        if (chairOccupied) {
            increaseSemaphore(fotele, 1);
            chairOccupied = 0;
        }

        printf("%s [FRYZJER %ld] Zwalniam fotel po kliencie %ld.\n", get_timestamp(), getpid(), client_id);

        giveChange(change, messageQueue, client_id, memory, kasa);

        printf("%s [FRYZJER %ld] Wydaje reszte %d dla klienta %ld.\n", get_timestamp(), getpid(), change, client_id);
        if(changeSend != 1) {
            sendMessage(messageQueue, &msg);
            changeSend = 1;
        }
        gotMessageP = 0;
        priceSend = 0;
        gotMoney = 0;
        changeSend = 0;
    }
    cleanResourcesFryzjer();
    printf("%s [FRYZJER %dl] Koncze prace.\n", get_timestamp(), getpid());
}

int processPayment(int client_id, int price, int messageQueue, int *memory, int kasa) {
    struct message msg;
    int suma, change;
    msg.mtype = client_id;
    msg.pid = getpid();
    msg.message[0] = price;

    sendMessage(messageQueue, &msg);
    recieveMessage(messageQueue, &msg, getpid());

    decreaseSemaphore(kasa, 1);
    memory[0] += msg.message[0];
    memory[1] += msg.message[1];
    memory[2] += msg.message[2];
    increaseSemaphore(kasa, 1);

    // Obliczenie sumy zapłaty klienta
    suma = msg.message[0] * 10 + msg.message[1] * 20 + msg.message[2] * 50;
    change = suma - price;

    printf("%s [FRYZJER %ld] Klient %ld zaplacil %d zl za usluge kosztujaca %d zl.\n",get_timestamp(), getpid(), client_id, suma, price);
    return change;
}

void giveChange(int change, int messageQueue, int client_id, int *memory, int kasa) {
    struct message msg;
    msg.mtype = client_id;
    msg.pid = getpid();
    msg.message[0] = 0;
    msg.message[1] = 0;
    msg.message[2] = 0;
    if (change) {
        decreaseSemaphore(kasa, 1);
        cashOccupied = 1;

        while (change > 0) {
            if (change >= 50 && memory[0] > 0){
                memory[2]--;
                msg.message[2]++;
                change -= 50;
            } else if (change >= 20 && memory[1] > 0) {
                memory[1]--;
                msg.message[0]++;
                change -= 20;
            } else if (change >= 10 && memory[2] > 0) {
                memory[0]--;
                msg.message[0]++;
                change -= 10;
            } else {
                printf("%s [FRYZJER %ld] Czekam na odpowiednie nominaly...",get_timestamp(), getpid());
                increaseSemaphore(kasa, 1);
                sleep(1);
                decreaseSemaphore(kasa, 1);
            }
        }
        increaseSemaphore(kasa, 1);
        cashOccupied = 0;
    }
}