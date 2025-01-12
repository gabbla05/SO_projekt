#ifndef KLIENT_H
#define KLIENT_H

struct Client {
    int clientID;
    int money;
};

void* client_handler(void* arg);

#endif
