#include "kierownik.h"
#include "utils.h"

int hour = 8;          

//pthread_t keyboardThread;
pthread_t timeSimulation; 

pid_t klient_pids[NUM_KLIENTOW];
pid_t fryzjer_pids[NUM_FRYZJEROW];

int signal1 = 0; 
int signal2 = 0; 

int fotele; 
int poczekalnia; 
int kasa; 
int messageQueue;
int sharedMemoryId; 
int *memory; 

void processesLimit() {
    struct rlimit rl;
    
    if (getrlimit(RLIMIT_NPROC, &rl) != 0) {
        perror("blad w getrlimit");
        exit(EXIT_FAILURE);
    }

    if (rl.rlim_cur < NUM_FRYZJEROW + NUM_KLIENTOW) {
        fprintf(stderr, "Przekroczono limit %ld procesow.", rl.rlim_cur);
        exit(EXIT_FAILURE);    
    }
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

void cleanResources() {
    deleteMessageQueue(messageQueue);
    deleteSemaphore(kasa);
    deleteSemaphore(fotele);
    deleteSemaphore(poczekalnia);
    detachSharedMemory(memory);
    deleteSharedMemory(sharedMemoryId);
    printf("%s [KIEROWNIK] Zasoby zostaly zwolnione.\n");
}

void createFryzjer() {
    for (int i = 0; i < NUM_FRYZJEROW; i++) {
        fryzjer_pids[i] = fork();
        if (fryzjer_pids[i] == 0) {
            execl("./fryzjer", "fryzjer", NULL);
        }
        printf("%s [KIEROWNIK] Nowy fryzjer %d\n",get_timestamp(), fryzjer_pids[i]);
    }
}

void createKlient() {
    for (int i = 0; i < NUM_KLIENTOW; i++) {
        klient_pids[i] = fork();
        if (klient_pids[i] == 0) {
            execl("./klient", "klient", NULL);
        }
        printf("%s [KIEROWNIK] Nowy klient %d\n",get_timestamp(), klient_pids[i]);
        int opoznienie = rand() % 3 + 1;
        sleep(opoznienie);
    }
}

void *timeSimulator(void *arg) {
    while (1) {
        sleep(5); //ten sleep jest konieczny zeby salon mogl sie otworzyc
        hour++;
        if (hour == 8) {
            printf("%s [KIEROWNIK] Godzina 8:00. Salon zostal otwarty.\n", get_timestamp());
            increaseSemaphore(poczekalnia, MAX_QUEUE_SIZE);
        } 
        else if (hour == 17) {
            printf("%s [KIEROWNIK] Za godzine zamkniecie, nie wpuszczamy nowych klientow.\n", get_timestamp());
            decreaseSemaphore(poczekalnia, MAX_QUEUE_SIZE);
        } 
        else if (hour == 24) {
            hour = 0;
        }
        printf("%s [KIEROWNIK] Jest godzina %d.\n", get_timestamp(), hour);
    }
    return NULL;
}

void endTimeSimulator() {
    pthread_cancel(timeSimulation);
    pthread_join(timeSimulation, NULL);
}

void sendSignal1() {
    for (int i = 0; i < NUM_FRYZJEROW; i++) {
        kill(fryzjer_pids[i], 1);
    }
    waitProcesses(NUM_FRYZJEROW);
}

void sendSignal2() {
    for (int i = 0; i < NUM_KLIENTOW; i++) {
        kill(klient_pids[i], 2);
    }
    waitProcesses(NUM_KLIENTOW);
}

void waitProcesses(int n) {
    int pid;
    int s;
	for(int i = 0; i < n; i++) {
		pid = wait(&s);
		if(pid == -1) {
			perror("Blad w funkcji wait");
			exit(EXIT_FAILURE);
		} else {
            printf("%s [KIEROWNIK] Koniec procesu: %d\n",get_timestamp(), pid);
		}
	}
}

void ctrlC(int s) {
    cleanResources();
    for (int i = 0; i < NUM_FRYZJEROW; i++) {
        kill(fryzjer_pids[i], SIGKILL);
    }
    for (int i = 0; i < NUM_KLIENTOW; i++) {
        kill(klient_pids[i], SIGKILL);
    }
    waitProcesses(NUM_FRYZJEROW);
    waitProcesses(NUM_KLIENTOW);
    endTimeSimulator();
    exit(EXIT_SUCCESS);
}

void sigTermHandler(int sig) {
    // Perform cleanup and terminate all processes
    cleanResources();
    for (int i = 0; i < NUM_FRYZJEROW; i++) {
        kill(fryzjer_pids[i], SIGKILL);
    }
    for (int i = 0; i < NUM_KLIENTOW; i++) {
        kill(klient_pids[i], SIGKILL);
    }
    waitProcesses(NUM_FRYZJEROW);
    waitProcesses(NUM_KLIENTOW);
    endTimeSimulator();
    exit(EXIT_FAILURE);
}

int main() {
      if (signal(SIGTERM, sigTermHandler) == SIG_ERR) {
        perror("Błąd obsługi sygnału SIGTERM");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGINT, ctrlC) == SIG_ERR)
    {
        perror("Blad obslugi sygnalu");
        exit(EXIT_FAILURE);
    }

    kierownik_pid = getpid();
    
    processesLimit();
    srand(time(NULL));

    key_t key;

    key = ftok(KEY_PATH, KEY_CHAR_MESSAGEQUEUE);
    messageQueue = createMessageQueue(key);

    key = ftok(KEY_PATH, KEY_CHAR_KASA);
    kasa = createSemaphore(key);
    setSemaphore(kasa, 1);
    
    key = ftok(KEY_PATH, KEY_CHAR_FOTELE);
    fotele = createSemaphore(key);
    setSemaphore(fotele, NUM_FOTELE);

    key = ftok(KEY_PATH,KEY_CHAR_POCZEKALNIA);
    poczekalnia = createSemaphore(key);
    setSemaphore(poczekalnia, MAX_QUEUE_SIZE);

    key = ftok(KEY_PATH,KEY_CHAR_SHAREDMEMORY);
    sharedMemoryId = createSharedMemory(key);
    memory = attachSharedMemory(sharedMemoryId);

    initCash(memory);

    printf("%s [KIEROWNIK] Symulacja rozpoczyna się od godziny 8:00.\n", get_timestamp());

    pthread_create(&timeSimulation, NULL, timeSimulator, (void *)NULL);

    createFryzjer();

    createKlient();

    char signalNumber;
    while (signalNumber != '3') {
        while (getchar() != '\n'); 
        while(scanf("%c", &signalNumber) != 1);
        switch (signalNumber)
        {
        case '1':
            sendSignal1();
            createFryzjer();
            break;
        case '2':
            sendSignal2();
            createKlient();
            break;
        case '0':
            ctrlC(0);
            break;
        default:
            printf("Nie ma takiego sygnalu.\n");
            break;
        }
    }

    exit(EXIT_FAILURE);
}

