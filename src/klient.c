#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 500
#include "utils.h"
#include "klient.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <signal.h>

volatile sig_atomic_t waitingRoomOccupied = 0;
volatile sig_atomic_t sendMessageP = 0;
volatile sig_atomic_t receivedCost = 0;
volatile sig_atomic_t sendMoney = 0;
volatile sig_atomic_t receivedChange = 0;
volatile sig_atomic_t gotSignal2 = 0;

long client_id;
long fryzjer_id;

int poczekalnia;
int messageQueue;

void handleSignal2(int sig) {
    if (sendMessageP == 1) {
        gotSignal2 = 1;
    
        if (receivedCost != 1) {
            receivedCost = -1;
        } else if (sendMoney != 1) {
            sendMoney = -1; 
        } else if (receivedChange != 1) {
            receivedChange = -1;
        }
    } else {
        if(waitingRoomOccupied) {
            printf("[KLIENT %dl] Zwalniam miejsce w poczekalni.\n", client_id);
            increaseSemaphore(poczekalnia, 1);
        }
    }
}

void rozbijNaNominaly(int amountPaid, int *nominaly) {
    int pozostalaKwota = amountPaid;
    
    // Wyzerowanie liczników banknotów
    nominaly[0] = 0; // Banknoty 10 zł
    nominaly[1] = 0; // Banknoty 20 zł
    nominaly[2] = 0; // Banknoty 50 zł

    while (pozostalaKwota > 0) {
        int nominal = rand() % 3;  // Wybierz losowy nominał: 0 = 10 zł, 1 = 20 zł, 2 = 50 zł

        if (nominal == 2 && pozostalaKwota >= 50) {
            nominaly[2]++;
            pozostalaKwota -= 50;
        } else if (nominal == 1 && pozostalaKwota >= 20) {
            nominaly[1]++;
            pozostalaKwota -= 20;
        } else if (nominal == 0 && pozostalaKwota >= 10) {
            nominaly[0]++;
            pozostalaKwota -= 10;
        }
    }
}


void klient_handler(int client_id) {
    prctl(PR_SET_NAME, "klient", 0, 0, 0);
    srand(time(NULL));
    client_id = getpid();

    if (signal(SIGINT, handleSignal2) == SIG_ERR) {
        perror("Blad sygnalu 2");
        exit(EXIT_FAILURE);
    }

    struct message msg;
    key_t key;
    int free;

    key = ftok(KEY_PATH, KEY_CHAR_MESSAGEQUEUE);
    messageQueue = createMessageQueue(key);
    key = ftok(KEY_PATH, KEY_CHAR_POCZEKALNIA);
    poczekalnia = createSemaphore(key);

    while(1) {
        if (gotSignal2) {
            break;
        }

        if (!waitingRoomOccupied) {
            free = decreaseSemaphoreNowait(poczekalnia, 1);
        }

        if(free == 0) {
            waitingRoomOccupied = 1;
            printf("[KLIENT %ld] Zajmuje miejsce w poczekalni.\n", client_id);

            msg.mtype = MESSAGE_P;
            msg.pid = client_id;
            sendMessage(messageQueue, &msg);
            sendMessageP = 1;

            int price = (rand() % 11 + 4) * 10; 
            int amountPaid = ((rand() % 6) + (price / 10)) * 10; 
            int nominaly[3];
            rozbijNaNominaly(amountPaid, nominaly);

            msg.mtype = fryzjer_id;

            msg.message[0] = nominaly[0]; // Liczba banknotów 10 zł
            msg.message[1] = nominaly[1]; // Liczba banknotów 20 zł
            msg.message[2] = nominaly[2]; // Liczba banknotów 50 zł

            if(sendMoney != 1) {
                sendMessage(messageQueue, &msg);
                sendMoney = 1;
            }

            printf("[KLIENT %ld]: Zapłacono %d zł za usługę wartą %d zł.\n", client_id, amountPaid, price);

            if (receivedChange != 1) {
                recieveMessage(messageQueue, &msg, getpid());
                receivedChange = 1;
            }

            sendMessageP = 0;
            sendMoney = 0;
            receivedChange = 0;
        } else {
            printf("[KLIENT %ld] Poczekalnia pelna, wracam do pracy.\n", client_id);
        }
        if (gotSignal2) {
            break;
        }
        sleep(rand()%100+10);
    }
    if(waitingRoomOccupied){
        printf("[KLIENT %ld]: Zwalniam miejsce w poczekalni.\n", client_id);
        increaseSemaphore(poczekalnia, 1);
    }
    printf("[KLIENT %ld] Wychodze z salonu.\n", client_id);
}


   
    
    
    
    