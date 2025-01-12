#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <limits.h>
#include "fryzjer.h"
#include "klient.h"
#include "kierownik.h"
#include "utils.h"

int main() {
    int liczbaKlientow = 3, liczbaFryzjerow = 2, liczbaFoteli = 1;

    // Pobieranie liczby fryzjerów
    while (1) {
        printf("Podaj liczbę fryzjerów (F > 1): \n");
        scanf("%d", &liczbaFryzjerow);
        if (liczbaFryzjerow <= 1) {
            printf("Błąd: liczba fryzjerów musi być większa od 1. Spróbuj ponownie.\n");
        } else {
            break;
        }
    }

    // Pobieranie liczby foteli
    while (1) {
        printf("Podaj liczbę foteli: \n");
        scanf("%d", &liczbaFoteli);
        if (liczbaFoteli >= liczbaFryzjerow || liczbaFoteli <= 0) {
            printf("Błąd: liczba foteli musi być mniejsza od liczby fryzjerów. Spróbuj ponownie.\n");
        } else {
            break;
        }
    }

    init_resources(2, liczbaFoteli);

    pthread_t fryzjer;
    pthread_t klient;

    // Tworzenie wątku dla fryzjera
    pthread_create(&fryzjer, NULL, &fryzjer_handler, NULL);

    // Tworzenie wątku dla klienta
    struct Client client = {1, 10};
    pthread_create(&klient, NULL, &client_handler, (void*)&client);

    pthread_join(fryzjer, NULL);
    pthread_join(klient, NULL);

    return 0;
}
