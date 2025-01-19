#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void* klient_handler(void* arg) {
    int client_id = *((int*)arg); // ID klienta przekazany jako argument

    enqueue(client_id); // Dodanie klienta do kolejki
    printf("Klient %d czeka na obsługę\n", client_id);

    // Klient czeka na zakończenie obsługi (symulacja)
    usleep(rand() % 5000000 + 1000000);
    printf("Klient %d opuścił salon\n", client_id);

    return NULL;
}
