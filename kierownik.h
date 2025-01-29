#ifndef KIEROWNIK_H
#define KIEROWNIK_H

void createFryzjer();
void createKlient();
void sendSignal1();
void sendSignal2();
void waitProcesses(int a);
void processesLimit();
void initResources();
void initCash(int *memory);
void cleanResources();
void *timeSimulator(void *arg);
void endTimeSimulator();
void endProgram(int signal);
void *keyboardListener(void *arg);


#endif
