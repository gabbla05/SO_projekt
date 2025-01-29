#ifndef KLIENT_H
#define KLIENT_H

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 500

void klient_handler(int client_id);
void handleSignal2(int sig) ;

#endif
