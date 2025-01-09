// main.c
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fryzjer.h"
#include "klient.h"
#include "kierownik.h"
#include "utils.h"
#include <iostream>
#include <queue>

pthread_mutex_t kasa_mutex;  
pthread_mutex_t queue_mutex;
sem_t poczekalnia_sem;
sem_t fotele_sem;
std::queue<int> poczekalnia;

int rozmiarPoczekalni = 2; //FIXME: przeniesC do main albo zrobIC all glboalnie

int kasa[3] = {5, 5, 5};  // Stan kasy: 10zł, 20zł, 50zł

struct Client{ // zamienic na samo money
    int clientID;
    int money;
};

void init_resources(int K, int N) {
    if (pthread_mutex_init(&kasa_mutex, NULL) != 0) {
        perror("Błąd inicjalizacji mutexu kasy");
        exit(EXIT_FAILURE); 
    }
    if (pthread_mutex_init(&queue_mutex, NULL) != 0) {
        perror("Błąd inicjalizacji mutexu kolejki");
        exit(EXIT_FAILURE); //FIXME: może do wywalenia potem
    }
    if (sem_init(&poczekalnia_sem, 0, K) == -1) {
        perror("Błąd inicjalizacji semafora poczekalnia_sem");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&fotele_sem, 0, N) == -1) {
        perror("Błąd inicjalizacji semafora fotele_sem");
        exit(EXIT_FAILURE);
    }
    
}

void* fryzjer_handler(void *arg){
    
    int clientID; //TODO: WYWALIC JAK NIEUZYWANE
    while(1){      
        if(poczekalnia.empty() == false){
            // handle hairdresser
            pthread_mutex_lock(&queue_mutex);
            clientID = poczekalnia.front();
            poczekalnia.pop();
            pthread_mutex_unlock(&queue_mutex);

            sem_wait(&fotele_sem); // czekanie na wolny fotel
            //TODO: pobieranie oplaty


        } else {
            std::cout << "Fryzjer spi" << std::endl;
            usleep(rand() % 1000000 + 1000000);
        }
    }
}

void* client_handler(void *arg){
    struct Client *client_data = (struct Client *) arg;
    int client_payment = client_data->money;  

    pthread_t thread_id = pthread_self();
    std::cout << "ID klienta: " << thread_id << std::endl;
    while(1){
        if(sem_trywait(&poczekalnia_sem) == 0){

            pthread_mutex_lock(&queue_mutex);
            poczekalnia.push(thread_id);
            pthread_mutex_unlock(&queue_mutex);


            sem_post(&poczekalnia_sem);
        } else {
            std::cout << "Klient spi" << std::endl;
            usleep(rand() % 1000000 + 1000000);  //Klient zarabia pieniądze (czeka od 5 do 10 sekund).
        }
    }
}

void* kierownik_handler(void *arg){

    return NULL;
}

int main()
{
    int liczbaKlientow = 3, liczbaFryzjerow = 2, liczbaFoteli = 1;
    // tym mozna walidowac liczbe procesow, nie wiem czy trzeba
    int hardwareThreads = sysconf(_SC_NPROCESSORS_ONLN); 
    //printf("THREADS COUNT: %i \n", hardwareThreads); 
    //printf("Podaj liczbe fryzjerow: ");
    //scanf() 

    init_resources(rozmiarPoczekalni, liczbaFoteli);


    pthread_t fryzjer;
    pthread_t klient;

    pthread_create(&fryzjer, NULL, &fryzjer_handler, NULL);
    struct Client *new_client = (struct Client *) malloc(sizeof(struct Client));
    new_client->clientID = 1; // iterator z petli
    new_client->money = 10; // zrobic rand
    
    pthread_create(&klient, NULL, &client_handler, (void *) new_client);
    //https://hpc-tutorials.llnl.gov/posix/
    
    // co to signal
    
    while(1);
}
