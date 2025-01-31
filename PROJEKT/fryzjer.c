#include "fryzjer.h"
#include "utils.h"

long fryzjer_id;
long client_id;
int fotele;
int kasa; 
int messageQueue; 
int sharedMemoryId; 
int *memory;

volatile sig_atomic_t chairOccupied = 0;
volatile sig_atomic_t cashOccupied = 0;
volatile sig_atomic_t gotMessageP = 0;
volatile sig_atomic_t priceSend = 0;
volatile sig_atomic_t gotMoney = 0;
volatile sig_atomic_t changeSend = 0;
volatile sig_atomic_t gotSignal1 = 0;

void handleSignal1(int sig) {
    printf("%s [FRYZJER %ld]: Otrzymalem sygnal do wylaczenia. Koncze prace.\n",get_timestamp(), fryzjer_id);
    if (gotMessageP) {
        gotSignal1 = 1;
        if (priceSend != 1) {
            priceSend = -1;
        } else if (gotMoney != 1) {
            gotMoney = -1;
        } else if (changeSend != 1) {
            changeSend = -1;
        }
    } else {
        cleanResourcesFryzjer();
        printf("%s [FRYZJER %ld]: Koncze prace.\n",get_timestamp(), fryzjer_id);    
        exit(EXIT_SUCCESS);
    }
    
}

void cleanResourcesFryzjer() {
    if (chairOccupied) {
        increaseSemaphore(fotele, 1);
    }
    if (cashOccupied) {
        increaseSemaphore(kasa, 1);
    }
    detachSharedMemory(memory);
}

// Funkcja do wyswietlania stanu kasy
void displayCashState(int *memory) {
    printf("%s Stan kasy: %d x 10zl, %d x 20zl, %d x 50zl.\n", get_timestamp(), memory[0], memory[1], memory[2]);
}

int  main() {
    srand(time(NULL));
    fryzjer_id = getpid();


    if (signal(SIGHUP, handleSignal1) == SIG_ERR)
    {
        perror("Blad sygnalu 1");
        exit(EXIT_FAILURE);
    }

    struct message msg;

    //initResourcesFryzjer();

    key_t key;

    key = ftok(KEY_PATH, KEY_CHAR_MESSAGEQUEUE);
    messageQueue = createMessageQueue(key);

    key = ftok(KEY_PATH, KEY_CHAR_FOTELE);
    fotele = createSemaphore(key);

    key = ftok(KEY_PATH, KEY_CHAR_KASA);
    kasa = createSemaphore(key);

    key = ftok(KEY_PATH,KEY_CHAR_SHAREDMEMORY);
    sharedMemoryId = createSharedMemory(key);
    memory = attachSharedMemory(sharedMemoryId);

    while (1) {
        if (gotSignal1) break;
        
        if (gotMessageP != 1) {
            recieveMessage(messageQueue, &msg, MESSAGE_P);
            gotMessageP = 1;
        }

        client_id = msg.pid;

        if (!chairOccupied) {
            decreaseSemaphore(fotele, 1); 
            chairOccupied = 1;
        }
        printf("%s [FRYZJER %ld]: Zajmuje fotel dla klienta %ld.\n",get_timestamp(), fryzjer_id, client_id);

        int cost = (rand() % 8 + 3) * 10;

        printf("%s [FRYZJER %ld]: Cena uslugi dla klienta %ld: %d zl.\033[0m\n",get_timestamp(), fryzjer_id, client_id, cost);
        
        msg.mtype = client_id;
        msg.pid = fryzjer_id;
        msg.message[0] = cost;
        if (priceSend != 1) {
            sendMessage(messageQueue, &msg);
            priceSend = 1;
        }

        if (gotMoney != 1) {
            recieveMessage(messageQueue, &msg, fryzjer_id);
            gotMoney = 1;
        }

        decreaseSemaphore(kasa, 1);
        cashOccupied = 1;
        memory[0] += msg.message[0];
        memory[1] += msg.message[1];
        memory[2] += msg.message[2];
        increaseSemaphore(kasa, 1);
        cashOccupied = 0;

        printf("%s [FRYZJER %ld]: Klient %ld zaplacil: %d x 10zl, %d x 20zl, %d x 50zl.\n", get_timestamp(), fryzjer_id, client_id, msg.message[0], msg.message[1], msg.message[2]);

        displayCashState(memory);
        
        printf("%s [FRYZJER %ld]: Zaczynam strzyzenie klienta %ld.\n",get_timestamp(), fryzjer_id, client_id);
        sleep(SERVICE_TIME + rand()%3);  

        if (chairOccupied) {
            increaseSemaphore(fotele, 1);
            chairOccupied = 0;
        }
        printf("%s [FRYZJER %ld]: Zwalniam fotel po kliencie %ld.\n",get_timestamp(), fryzjer_id, client_id);

        msg.mtype = client_id;
        msg.pid = fryzjer_id;

        int paid10 = msg.message[0];
        int paid20 = msg.message[1];
        int paid50 = msg.message[2];

        msg.message[0] = 0;
        msg.message[1] = 0;
        msg.message[2] = 0;

        int change = (paid10 * 10) + (paid20 * 20) + (paid50 * 50) - cost;

        if (change) {
            decreaseSemaphore(kasa, 1);
            cashOccupied = 1;
            
            //memory[0] = 0; 
            
            while (change > 0) { 
                if (memory[2] > 0 && change >= 50) {
                    memory[2]--;  
                    msg.message[2]++; 
                    change -= 50;  
                } else if (memory[1] > 0 && change >= 20) {
                    memory[1]--;  
                    msg.message[1]++; 
                    change -= 20;  
                } else if (memory[0] > 0 && change >= 10) {
                    memory[0]--;  
                    msg.message[0]++; 
                    change -= 10;  
                } else if (memory[0] == 0) {
                    printf("%s [FRYZJER %ld]: Czekam na odpowiednie nominaly...\n",get_timestamp(), fryzjer_id);
                    increaseSemaphore(kasa, 1);
                    cashOccupied = 0;
                    sleep(3);  
                    decreaseSemaphore(kasa, 1);
                    cashOccupied = 1;
                }
            }
            increaseSemaphore(kasa, 1);
            cashOccupied = 0;
        }

        printf("%s [FRYZJER %ld]: Wydaje reszte: %d x 10zl, %d x 20zl, %d x 50zl dla klienta %ld.\n", get_timestamp(), fryzjer_id, msg.message[0], msg.message[1], msg.message[2], client_id);
        if (changeSend != 1) {
            sendMessage(messageQueue, &msg);
            changeSend = 1;
        }

        displayCashState(memory);

        gotMessageP = 0;
        priceSend = 0;
        gotMoney = 0;
        changeSend = 0;
    }

    cleanResourcesFryzjer();
    printf("%s [FRYZJER %ld]: Koncze prace.\n",get_timestamp(), fryzjer_id);
}