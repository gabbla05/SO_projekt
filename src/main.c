#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "fryzjer.h"
#include "klient.h"
#include "kierownik.h"
#include "utils.h"

#define F 3 //liczba fryzjerów
#define N 2 //liczba foteli
#define K 5 //liczba miejsc w poczekalni

pthread_mutex_t kasa_mutex;
sem_t poczekalnia_sem;
sem_t fotele_sem;

int kasa[3] = {5,5,5}; //stan kasy 10zl, 20zl, 30zl

int main() {

   printf("test");
   return 0;
}
