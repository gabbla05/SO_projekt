#include "klient.h"
#include "utils.h"

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
    printf("%s [KLIENT %ld]: Otrzymałem sygnał do opuszczenia salonu.\n",get_timestamp(), client_id);
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
        if (waitingRoomOccupied) {
            printf("%s [KLIENT %ld]: Wychodze z poczekalni.\n",get_timestamp(), client_id);
            increaseSemaphore(poczekalnia, 1);
        }
        printf("%s [KLIENT %ld]: Wychodze z salonu.\n",get_timestamp(), client_id);
        exit(EXIT_SUCCESS);
    }
}

void splitIntoDenominations(int amountPaid, int *denominations) {
    int remainingAmount = amountPaid;

    denominations[0] = 0; 
    denominations[1] = 0; 
    denominations[2] = 0; 

    while (remainingAmount > 0) {
        int denomination = rand() % 3;  

        if (denomination == 2 && remainingAmount >= 50) {
            denominations[2]++;
            remainingAmount -= 50;
        } else if (denomination == 1 && remainingAmount >= 20) {
            denominations[1]++;
            remainingAmount -= 20;
        } else if (denomination == 0 && remainingAmount >= 10) {
            denominations[0]++;
            remainingAmount -= 10;
        }
    }
}

int main() {
    srand(time(NULL));
    client_id = getpid();
    if (signal(SIGINT, handleSignal2) == SIG_ERR)
    {
        perror("Blad sygnalu 2");
        exit(EXIT_FAILURE);
    }

    struct message msg;
    key_t key;
    int free;

    key = ftok(KEY_PATH, KEY_CHAR_MESSAGEQUEUE);
    messageQueue = createMessageQueue(key);

    key = ftok(KEY_PATH,KEY_CHAR_POCZEKALNIA);
    poczekalnia = createSemaphore(key);

    while (1) {
        if (gotSignal2) {
            break;
        }

        if (!waitingRoomOccupied) {
            free = decreaseSemaphoreNowait(poczekalnia, 1);
        }

        if (free == 0) {
            waitingRoomOccupied = 1;
            printf("%s [KLIENT %ld]: Zajmuje miejsce w poczekalni.\n",get_timestamp(), client_id);

            msg.mtype = MESSAGE_P;
            msg.pid = client_id;
            sendMessage(messageQueue, &msg);
            sendMessageP = 1;
            
            if (receivedCost != 1) {
                recieveMessage(messageQueue, &msg, client_id);
                receivedCost = 1;
            }
            
            if (waitingRoomOccupied) {
                increaseSemaphore(poczekalnia, 1);
                printf("%s [KLIENT %ld]: Wychodzę z poczekalni.\n",get_timestamp(), client_id);
                waitingRoomOccupied = 0;
            }

            fryzjer_id = msg.pid; 

            int price = msg.message[0];
            int extraMoney = (rand() % 4) * 10; 
            int amountPaid = price + extraMoney;  
            int sumPaid = 0;  

            msg.mtype = fryzjer_id;
            msg.pid = client_id;
            
            msg.message[0] = 0;
            msg.message[1] = 0;
            msg.message[2] = 0;

            splitIntoDenominations(amountPaid, msg.message);

            sumPaid = (msg.message[0] * 10) + (msg.message[1] * 20) + (msg.message[2] * 50);

            if (sendMoney != 1) {
                sendMessage(messageQueue, &msg);
                sendMoney = 1;
            }

            printf("%s [KLIENT %ld]: Zapłacono %d zł za usluge warta %d zl.\n",get_timestamp(), client_id, sumPaid, price);

            if (receivedChange != 1) {
                recieveMessage(messageQueue, &msg, client_id);
                receivedChange = 1;
            }

            sendMessageP = 0;
            receivedCost = 0;
            sendMoney = 0;
            receivedChange = 0;
        } else {
            printf("%s [KLIENT %ld]: Poczekalnia pelna, wracam do pracy.\n",get_timestamp(), client_id);
        }

        if (gotSignal2) {
            break;
        }

        sleep(rand() % 100 + 10);
        //sleep(0); 
    }
    
    if (waitingRoomOccupied) {
        printf("%s [KLIENT %ld]: Zwalniam miejsce w poczekalni.\n",get_timestamp(), client_id);
        increaseSemaphore(poczekalnia, 1);
    }
    printf("%s [KLIENT %ld]: Wychodze z salonu.\n",get_timestamp(), client_id);
}

