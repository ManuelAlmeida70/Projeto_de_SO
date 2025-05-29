#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler_linux.h"

#define MAX_TASKS 100

// Estruturas das runqueues
static task_t *active_runqueue[MAX_PRIORITY] = {NULL};
static task_t *expired_runqueue[MAX_PRIORITY] = {NULL};
static task_t *blocked_list = NULL;

static int epoch_active = 1;

// Função auxiliar para inserir tarefa no final de uma fila
static void enqueue(task_t **queue, task_t *t) {
    t->next = NULL;
    if (!*queue) {
        *queue = t;
    } else {
        task_t *cur = *queue;
        while (cur->next) cur = cur->next;
        cur->next = t;
    }
}

// Função auxiliar para remover a primeira tarefa de uma fila
static task_t *dequeue(task_t **queue) {
    if (!*queue) return NULL;
    task_t *t = *queue;
    *queue = t->next;
    t->next = NULL;
    return t;
}

// Inicializa o escalonador
void scheduler_init(void) {
    memset(active_runqueue, 0, sizeof(active_runqueue));
    memset(expired_runqueue, 0, sizeof(expired_runqueue));
    blocked_list = NULL;
}

// Adiciona tarefa à runqueue correta (ativa ou expiradas)
void scheduler_add_task(task_t *t) {
    if (t->priority < STATIC_PRIORITIES) {
        // Prioridade fixa
        t->quantum = QUANTUM_BASE;
    } else {
        // Prioridade dinâmica
        t->quantum = QUANTUM_BASE;
        t->remaining_quantum = t->quantum;
    }
    enqueue(&active_runqueue[t->priority], t);
}

// Devolve próxima tarefa a executar
task_t *scheduler_next_task(void) {
    for (int i = 0; i < MAX_PRIORITY; i++) {
        if (active_runqueue[i]) {
            return dequeue(&active_runqueue[i]);
        }
    }

    // Se todas as filas ativas estiverem vazias, trocar as runqueues
    task_t **tmp = active_runqueue;
    active_runqueue = expired_runqueue;
    expired_runqueue = tmp;

    // Tentar novamente
    for (int i = 0; i < MAX_PRIORITY; i++) {
        if (active_runqueue[i]) {
            return dequeue(&active_runqueue[i]);
        }
    }

    return NULL; // Nenhuma tarefa disponível
}

// Quando quantum termina
void scheduler_on_quantum_expire(task_t *t) {
    if (t->priority >= STATIC_PRIORITIES) {
        int unused = t->remaining_quantum;
        t->quantum = QUANTUM_BASE + unused / 2;
        int new_priority = t->base_priority - unused + t->nice;
        if (new_priority < STATIC_PRIORITIES) new_priority = STATIC_PRIORITIES;
        if (new_priority >= MAX_PRIORITY) new_priority = MAX_PRIORITY - 1;
        t->priority = new_priority;
    }
    enqueue(&expired_runqueue[t->priority], t);
}

// Quando a tarefa bloqueia (entra em fila de bloqueados)
void scheduler_on_block(task_t *t) {
    enqueue(&blocked_list, t);
}

// Quando desbloqueia, vai direto para fila ativa
void scheduler_on_unblock(task_t *t) {
    int unused = t->remaining_quantum;
    if (t->priority >= STATIC_PRIORITIES) {
        t->quantum = QUANTUM_BASE + unused / 2;
        int new_priority = t->base_priority - unused + t->nice;
        if (new_priority < STATIC_PRIORITIES) new_priority = STATIC_PRIORITIES;
        if (new_priority >= MAX_PRIORITY) new_priority = MAX_PRIORITY - 1;
        t->priority = new_priority;
    }
    enqueue(&active_runqueue[t->priority], t);
}

// Altera o valor do nice da tarefa atual (simulada)
int scheduler_nice(int nice_value) {
    if (nice_value < 0) nice_value = 0;
    if (nice_value > NICE_MAX) nice_value = NICE_MAX;
    // Este valor seria armazenado na thread atual (deve ser ajustado na integração)
    return nice_value;
}

// Imprime o estado atual das filas
void scheduler_dump(void) {
    printf("=== dump start ===\n");

    printf("active runqueue\n");
    for (int i = 0; i < MAX_PRIORITY; i++) {
        printf("[%d] ", i);
        task_t *t = active_runqueue[i];
        while (t) {
            printf("%d,%d ", t->id, t->remaining_quantum);
            t = t->next;
        }
        printf("\n");
    }

    printf("expired runqueue\n");
    for (int i = 0; i < MAX_PRIORITY; i++) {
        printf("[%d] ", i);
        task_t *t = expired_runqueue[i];
        while (t) {
            printf("%d,%d ", t->id, t->remaining_quantum);
            t = t->next;
        }
        printf("\n");
    }

    printf("blocked list\n");
    task_t *b = blocked_list;
    while (b) {
        printf("%d,%d ", b->id, b->remaining_quantum);
        b = b->next;
    }
    printf("\n=== dump end ===\n");
}

static void recalculate_priority(task_t *t, int unused) {
    int new_priority = t->base_priority - unused + t->nice;
    if (new_priority < STATIC_PRIORITIES) new_priority = STATIC_PRIORITIES;
    if (new_priority >= MAX_PRIORITY) new_priority = MAX_PRIORITY - 1;
    t->priority = new_priority;
}

void scheduler_tick(void) {
    if (current_thread) {
        current_thread->remaining_quantum--;
        if (current_thread->remaining_quantum <= 0) {
            scheduler_on_quantum_expire(current_thread);
            current_thread = scheduler_next_task();
        }
    }
}
