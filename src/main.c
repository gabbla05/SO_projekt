// main.c
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "fryzjer.h"
#include "klient.h"
#include "kierownik.h"
#include "utils.h"

#define F 3   // Liczba fryzjerów
#define N 2   // Liczba foteli
#define K 5   // Liczba miejsc w poczekalni

pthread_mutex_t kasa_mutex;
sem_t poczekalnia_sem;
sem_t fotele_sem;

int kasa[3] = {5, 5, 5};  // Stan kasy: 10zł, 20zł, 50zł

int main()
{
   
}
