#ifndef KLIENT_H
#define KLIENT_H

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 500

void rozbijNaNominaly(int amountPaid, int *nominaly);
void handleSignal2(int sig);

#endif
