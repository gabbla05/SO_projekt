#ifndef FRYZJER_H
#define FRYZJER_H

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 500

void handleSignal1(int sig);
void initResourcesFryzjer();
void cleanResourcesFryzjer();
int processPayment(int client_id, int price, int messageQueue, int *memory, int kasa);
void giveChange(int change, int messageQueue, int client_id, int *memory, int kasa);
void displayCashState(int *memory);

#endif
