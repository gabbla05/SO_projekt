#ifndef FRYZJER_H
#define FRYZJER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include "utils.h"

void handleSignal1(int sig);
void cleanResourcesFryzjer();
void displayCashState(int *memory);

#endif 