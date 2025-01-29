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
long client_id;
int *memory;
int messageQueue;
int fotele;
int kasa;
int sharedMemoryId;

// Funkcja do obslugi sygnalu 1 od kierownika - fryzjer konczy prace.
void handleSignal1(int sig) {
    printf("%s [FRYZJER %ld] Otrzymałem sygnał do wyłączenia. Kończę pracę.\n", get_timestamp(), fryzjer_id);
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
            printf("%s [FRYZJER %ld] Koncze prace.\n", get_timestamp(), fryzjer_id);
            exit(0);
        }
    }
}

// Funkcja do tworzenia zasobow dla fryzjera - kolejka komunikatow, semafor kasa, semafor fotele, pamiec wspoldzielona
void initResourcesFryzjer() {
    key_t key;

    key = ftok(KEY_PATH, KEY_CHAR_MESSAGEQUEUE);
    messageQueue = createMessageQueue(key);

    key = ftok(KEY_PATH, KEY_CHAR_KASA);
    kasa = createSemaphore(key);
    setSemaphore(kasa, 1);

    key = ftok(KEY_PATH, KEY_CHAR_FOTELE);
    fotele = createSemaphore(key);
    setSemaphore(fotele, NUM_FOTELE);

    key = ftok(KEY_PATH, KEY_CHAR_SHAREDMEMORY);
    sharedMemoryId = createSharedMemory(key);
    memory = attachSharedMemory(sharedMemoryId);
}

// Funkcja do czyszczenia zasobow gdy fryzjer konczy prace - zwalnia semafory i pamiec wspoldzielona.
void cleanResourcesFryzjer() {
    if (chairOccupied) {
        increaseSemaphore(fotele, 1);
    }

    if(cashOccupied) {
        increaseSemaphore(kasa, 1);
    }
    detachSharedMemory(memory);
}

// Funkcja do obslugi platnosci klienta i obliczenia reszty.
int processPayment(int client_id, int price, int messageQueue, int *memory, int kasa) {
    struct message msg;
    int suma, change;
    msg.mtype = client_id;
    msg.pid = getpid();
    msg.message[0] = price;

    sendMessage(messageQueue, &msg);
    recieveMessage(messageQueue, &msg, getpid());

    decreaseSemaphore(kasa, 1);
    cashOccupied = 1;
    memory[0] += msg.message[0];
    memory[1] += msg.message[1];
    memory[2] += msg.message[2];
    increaseSemaphore(kasa, 1);
    cashOccupied = 0;

    // Obliczenie sumy zapłaty klienta
    suma = msg.message[0] * 10 + msg.message[1] * 20 + msg.message[2] * 50;
    change = suma - price;

    printf("%s [FRYZJER %ld] Klient %ld zaplacil %d zl za usluge kosztujaca %d zl.\n",get_timestamp(), fryzjer_id, client_id, suma, price);
    displayCashState(memory);
    return change;
}

// Funkcja do dobrania odpowiednich nominalow i wydania reszty klientowi.
void giveChange(int change, int messageQueue, int client_id, int *memory, int kasa) {
    (void)messageQueue;
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
                printf("%s [FRYZJER %ld] Czekam na odpowiednie nominaly...",get_timestamp(), fryzjer_id);
                increaseSemaphore(kasa, 1);
                cashOccupied = 0;
                sleep(1);
                decreaseSemaphore(kasa, 1);
                cashOccupied = 1;
            }
        }
        increaseSemaphore(kasa, 1);
        cashOccupied = 0;
    }
    displayCashState(memory); 
}

// Funkcja do wyswietlania stanu kasy
void displayCashState(int *memory) {
    printf("%s Stan kasy: %d x 10zl, %d x 20zl, %d x 50zl.\n", get_timestamp(), memory[0], memory[1], memory[2]);
}

// Funkcja glowna programu fryzjer
int main() {
    srand(time(NULL));
    fryzjer_id = getpid();
    
    // Rejestracja obslugi sygnalu 1 
    if(signal(SIGHUP, handleSignal1) == SIG_ERR) {
        perror("Blad sygnalu 1.");
        exit(EXIT_FAILURE);
    }

    struct message msg;

    initResourcesFryzjer();

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

       printf("%s [FRYZJER %ld]: Cena uslugi dla klienta %d: %d\n", get_timestamp(),fryzjer_id, client_id, cost);
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

        printf("%s [FRYZJER %ld] Obsluguje klienta %ld\n",get_timestamp(), fryzjer_id, client_id);
        sleep(SERVICE_TIME);

        if (chairOccupied) {
            increaseSemaphore(fotele, 1);
            chairOccupied = 0;
        }

        printf("%s [FRYZJER %ld] Zwalniam fotel po kliencie %ld.\n", get_timestamp(), fryzjer_id, client_id);

        giveChange(change, messageQueue, client_id, memory, kasa);

        printf("%s [FRYZJER %ld] Wydaje reszte %d dla klienta %ld.\n", get_timestamp(), fryzjer_id, change, client_id);
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
    printf("%s [FRYZJER %dl] Koncze prace.\n", get_timestamp(), fryzjer_id);
}

