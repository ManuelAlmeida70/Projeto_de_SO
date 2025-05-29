#include <stdio.h>
#include "sthread.h"

void func(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < 5; i++) {
        printf("Thread %d running...\n", id);
        sthread_yield();
    }
}

int main() {
    int id1 = 1, id2 = 2, id3 = 3;
    sthread_create(func, &id1, 5);  // prioridade dinâmica
    sthread_create(func, &id2, 1);  // prioridade fixa
    sthread_create(func, &id3, 10); // prioridade dinâmica baixa
    sthread_start();
    return 0;
}
