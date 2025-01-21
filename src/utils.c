
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/prctl.h>

int msg_queue_id;       // Kolejka komunikatów
int sem_id;             // Semafory
int shm_id;             // Identyfikator pamięci współdzielonej dla kasy
struct Kasa* kasa;      // Wskaźnik na kasę w pamięci współdzielonej
extern pid_t main_process_pid;
extern char *program_invocation_short_name;
struct Queue queue = { .head = 0, .tail = 0, .size = 0 }; // Definicja i inicjalizacja zmiennej globalnej
int poczekalnia_id; // Semafor do kontrolowania poczekalni
int fotele_id;      // Semafor do kontrolowania foteli
int fryzjer_signal_id;
int* sleeping_fryzjer = NUM_FRYZJEROW;
int sleeping_count_id; // Identyfikator pamięci współdzielonej dla licznika śpiących fryzjerów




const char* get_process_role() {
    char proc_name[16];
    prctl(PR_GET_NAME, proc_name, 0, 0, 0);

    printf("Process name: %s\n", proc_name);

    if (strcmp(proc_name, "kierownik") == 0) {
        return "[KIEROWNIK]";
    } else if (strcmp(proc_name, "fryzjer") == 0) {
        return "[FRYZJER]";
    } else if (strcmp(proc_name, "klient") == 0) {
        return "[KLIENT]";
    } else {
        return "[NIEZNANY]";
    }
}


// Operacje semaforowe
void lock_semaphore() {
    struct sembuf sops = {0, -1, 0};
    semop(sem_id, &sops, 1);
}

void unlock_semaphore() {
    struct sembuf sops = {0, 1, 0};
    semop(sem_id, &sops, 1);
}

// Inicjalizacja zasobów
void init_resources() {

    // Kolejka komunikatów
    msg_queue_id = msgget(QUEUE_KEY, IPC_CREAT | 0666);
    if (msg_queue_id == -1) {
        perror("Błąd tworzenia kolejki komunikatów");
        exit(EXIT_FAILURE);
    }

    // Pamięć współdzielona
    shm_id = shmget(SHM_KEY, sizeof(struct Kasa), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Błąd tworzenia pamięci współdzielonej dla kasy.");
        exit(EXIT_FAILURE);
    }

    kasa = shmat(shm_id, NULL, 0);
    if (kasa == (void*)-1) {
        perror("Błąd dołączania pamięci współdzielonej dla kasy");
        exit(EXIT_FAILURE);
    }

    // Tworzenie pamięci współdzielonej dla licznika śpiących fryzjerów
    sleeping_count_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (sleeping_count_id == -1) {
        perror("Błąd tworzenia pamięci współdzielonej dla licznika śpiących fryzjerów");
        exit(EXIT_FAILURE);
    }

    // Dołączanie pamięci współdzielonej
    sleeping_fryzjer = shmat(sleeping_count_id, NULL, 0);
    if (sleeping_fryzjer == (void*)-1) {
        perror("Błąd dołączania pamięci współdzielonej dla licznika śpiących fryzjerów");
        exit(EXIT_FAILURE);
    }

    // Inicjalizacja wartości
    *sleeping_fryzjer = NUM_FRYZJEROW;
    // Inicjalizacja kasy
    // Inicjalizacja wartości w kasie
    kasa->salon_open = 1;

    kasa->tens = 10;
    kasa->twenties = 5;
    kasa->fifties = 2;

    for (int i = 0; i < NUM_FRYZJEROW; i++) {
        kasa->fryzjer_status[i] = 1; // Wszyscy fryzjerzy początkowo aktywni
    }

    for (int i = 0; i < NUM_KLIENTOW; i++) {
        kasa->client_done[i] = 0; // Wszyscy klienci początkowo nieobsłużeni
    }
    printf("Kasa została zainicjalizowana. Początkowy stan:\n");
    printf(" - 10 zł: %d\n", kasa->tens);
    printf(" - 20 zł: %d\n", kasa->twenties);
    printf(" - 50 zł: %d\n", kasa->fifties);
    
    // Semafory
    sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("Błąd tworzenia semaforów");
        exit(EXIT_FAILURE);
    }
    semctl(sem_id, 0, SETVAL, 1); // Inicjalizacja semafora

    // Inicjalizacja semafora poczekalni
    poczekalnia_id = semget(POCZEKALNIA_KEY, 1, IPC_CREAT | 0666);
    if (poczekalnia_id == -1) {
        perror("Błąd tworzenia semafora poczekalni");
        exit(EXIT_FAILURE);
    }
    semctl(poczekalnia_id, 0, SETVAL, MAX_QUEUE_SIZE);

    // Inicjalizacja semafora sygnału fryzjera
    fryzjer_signal_id = semget(FRYZJER_SIGNAL_KEY, 1, IPC_CREAT | 0666);
    if (fryzjer_signal_id == -1) {
        perror("Błąd tworzenia semafora sygnału fryzjera");
        exit(EXIT_FAILURE);
    }
    semctl(fryzjer_signal_id, 0, SETVAL, 0); // Początkowo fryzjerzy czekają w poczekalni

    // Semafor dla foteli (2 miejsca)
    fotele_id = semget(FOTELE_KEY, 1, IPC_CREAT | 0666);
    if (fotele_id == -1) {
        perror("Błąd tworzenia semafora dla foteli");
        exit(EXIT_FAILURE);
    }
    semctl(fotele_id, 0, SETVAL, MAX_FOTELE); // 2 dostępne fotele
    
}

// Sprzątanie zasobów
void cleanup_resources() {
    if (getpid() != main_process_pid) {
        printf("Proces %d nie ma uprawnień do usunięcia zasobów.\n", getpid());
        return;
    }

    lock_semaphore();
    kasa->salon_open = 0;
    unlock_semaphore();

    if (msgctl(msg_queue_id, IPC_RMID, NULL) == -1) {
        perror("Błąd usuwania kolejki komunikatów");
    }
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("Błąd usuwania pamięci współdzielonej");
    }
    // Usuwanie pamięci współdzielonej dla licznika śpiących fryzjerów
    if (shmdt(sleeping_fryzjer) == -1) {
        perror("Błąd odłączania pamięci współdzielonej dla licznika śpiących fryzjerów");
    }
    if (shmctl(sleeping_count_id, IPC_RMID, NULL) == -1) {
        perror("Błąd usuwania pamięci współdzielonej dla licznika śpiących fryzjerów");
    }


    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("Błąd usuwania semaforów");
    }

    if (semctl(poczekalnia_id, 0, IPC_RMID) == -1) {
        perror("Błąd usuwania semafora poczekalni");
    }
    if (semctl(fotele_id, 0, IPC_RMID) == -1) {
        perror("Błąd usuwania semafora foteli");
    }
    if (semctl(fryzjer_signal_id, 0, IPC_RMID) == -1) {
        perror("Błąd usuwania semafora budzenia fryzjera");
    }

    printf("Zasoby zostały usunięte.\n");
}


// Obsługa kolejki
int enqueue(int client_id) {
    struct sembuf zajmij_miejsce = {0, -1, 0};
    if (semop(poczekalnia_id, &zajmij_miejsce, 1) == -1) {
        printf("[KLIENT %d] Poczekalnia pełna, klient %d opuszcza salon.\n", getpid(), client_id);
        return -1;
    }

    lock_semaphore();
    queue.clients[queue.tail] = client_id;
    queue.tail = (queue.tail + 1) % MAX_QUEUE_SIZE;
    queue.size++;
    printf("[KLIENT %d] Klient %d został dodany do poczekalni. Liczba klientów w poczekalni: %d.\n",
           getpid(), client_id, queue.size);
    

    if (queue.size == 1 && sleeping_fryzjer > 0) { // Jeśli to pierwszy klient w poczekalni
        struct sembuf budz_fryzjera = {0, 1, 0};
        if (semop(fryzjer_signal_id, &budz_fryzjera, 1) == -1) {
            perror("[KLIENT] Błąd budzenia fryzjera");
        } else {
            sleeping_fryzjer--; // Zmniejsz licznik śpiących fryzjerów
        }
    }
    unlock_semaphore();

    return 0;
}

int dequeue() {
    lock_semaphore(); // Blokujemy dostęp do kolejki
    if (queue.size == 0) {
        printf("[FRYZJER %d] Brak klientów, fryzjer śpi...\n", getpid());
        unlock_semaphore(); // Odblokowanie semafora
        return -1; // Brak klientów
    }

    int client_id = queue.clients[queue.head];
    queue.head = (queue.head + 1) % MAX_QUEUE_SIZE;
    queue.size--;

    // Zwalnianie miejsca w poczekalni
    struct sembuf zwolnij_miejsce = {0, 1, 0}; 
    if (semop(poczekalnia_id, &zwolnij_miejsce, 1) == -1) {
        perror("[FRYZJER] Błąd zwalniania miejsca w poczekalni");
        unlock_semaphore(); // Odblokowanie semafora przed wyjściem
        return -1; // Zwróć błąd
    }

    printf("[FRYZJER %d] Klient %d został pobrany z poczekalni. Liczba klientów w poczekalni: %d.\n", getpid(), client_id, queue.size);
    unlock_semaphore(); // Odblokowanie semafora
    return client_id;
}

// Obsługa sygnału SIGUSR2 (zamykanie salonu)
void sigusr2_handler(int signo) {
    if (signo == SIGUSR2) {
        printf("[PROCES %d] Otrzymano SIGUSR2: Salon zamknięty. Klienci opuszczają salon.\n", getpid());

        // Klienci opuszczają salon
        if (strcmp(get_process_role(), "[KLIENT]") == 0) {
            printf("[KLIENT %d] Klient opuszcza salon z powodu zamknięcia.\n", getpid());
            exit(0);
        }

        // Fryzjerzy kończą pracę
        if (strcmp(get_process_role(), "[FRYZJER]") == 0) {
            printf("[FRYZJER %d] Fryzjer kończy pracę, salon zamknięty.\n", getpid());
            exit(0);
        }
    }
}

// Rejestracja obsługi sygnałów
void register_signal_handlers() {
    struct sigaction sa;
    sa.sa_handler = sigusr2_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("Błąd rejestracji SIGUSR2");
        exit(EXIT_FAILURE);
    }

    printf("Obsługa sygnałów została zarejestrowana.\n");
}




// Obsługa płatności
void process_payment(int tens, int twenties, int fifties) {
    lock_semaphore();
    kasa->tens += tens;
    kasa->twenties += twenties;
    kasa->fifties += fifties;

    printf("Dodano do kasy: %d x 10 zł, %d x 20 zł, %d x 50 zł\n", tens, twenties, fifties);
    printf("Nowy stan kasy: %d x 10 zł, %d x 20 zł, %d x 50 zł\n", kasa->tens, kasa->twenties, kasa->fifties);
    unlock_semaphore();
}

void give_change(int amount) {
    lock_semaphore();
    int remaining = amount;
    int tens_used = 0, twenties_used = 0, fifties_used = 0;

    while (remaining > 0) {
        if (remaining >= 50 && kasa->fifties > 0) {
            kasa->fifties--;
            remaining -= 50;
            fifties_used++;
        } else if (remaining >= 20 && kasa->twenties > 0) {
            kasa->twenties--;
            remaining -= 20;
            twenties_used++;
        } else if (remaining >= 10 && kasa->tens > 0) {
            kasa->tens--;
            remaining -= 10;
            tens_used++;
        } else {
            printf("Brak odpowiednich nominałów, czekanie na uzupełnienie kasy...\n");
            unlock_semaphore();
            usleep(500000); // Oczekiwanie przed kolejną próbą
            lock_semaphore();
        }
    }

    printf("Reszta %d zł została wydana: %d x 10 zł, %d x 20 zł, %d x 50 zł\n",
           amount, tens_used, twenties_used, fifties_used);
    unlock_semaphore();
}

void wait_for_processes() {
    int status;
    while (1) {
        pid_t pid = waitpid(-1, &status, 0);
        if (pid == -1) {
            if (errno == ECHILD) {
                break; // Brak więcej dzieci do oczekiwania
            }
            perror("Błąd podczas oczekiwania na procesy potomne");
            break;
        }

        if (WIFEXITED(status)) {
            printf("Proces o PID %d zakończył się poprawnie (kod wyjścia: %d).\n", pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Proces o PID %d zakończył się z sygnałem %d.\n", pid, WTERMSIG(status));
        }
    }
}

// Funkcja do usuwania procesów zombie
void* reap_zombies(void* arg) {
    while (1) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0) {
            if (WIFEXITED(status)) {
                printf("Proces o PID %d zakończył się poprawnie (kod wyjścia: %d).\n", pid, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Proces o PID %d zakończył się z sygnałem %d.\n", pid, WTERMSIG(status));
            }
        }
        usleep(100000); // Sprawdzenie co 100ms
    }
    return NULL;
}
