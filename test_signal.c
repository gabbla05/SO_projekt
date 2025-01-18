#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void handle_signal(int sig) {
    if (sig == SIGUSR1) {
        printf("Otrzymano SIGUSR1\n");
    } else if (sig == SIGUSR2) {
        printf("Otrzymano SIGUSR2\n");
        exit(0);
    }
}

int main() {
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Błąd rejestracji SIGUSR1");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("Błąd rejestracji SIGUSR2");
        exit(EXIT_FAILURE);
    }

    printf("Czekam na sygnały...\n");

    while (1) {
        pause(); // Oczekiwanie na sygnały
    }

    return 0;
}
