#ifndef KIEROWNIK_H
#define KIEROWNIK_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include "utils.h"

void createKlient();
void createFryzjer();
void sendSignal1();
void sendSignal2();
void waitProcesses(int n);
void processesLimit();
void initCash(int *memory);
void cleanResources();
void *timeSimulator(void *arg);
void endTimeSimulator();
void endProgram(int s);
void ctrlC(int s);

#endif /* KIEROWNIK_H */