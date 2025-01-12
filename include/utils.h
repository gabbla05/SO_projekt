#ifndef UTILS_H
#define UTILS_H

#include <semaphore.h>

extern sem_t poczekalnia_sem;
extern sem_t fotele_sem;
extern pthread_mutex_t queue_mutex;
extern pthread_mutex_t kasa_mutex;

void init_resources(int K, int N);

#endif
