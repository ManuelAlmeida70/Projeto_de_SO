#ifndef SCHEDULER_LINUX_H
#define SCHEDULER_LINUX_H

#define MAX_PRIORITY 15
#define STATIC_PRIORITIES 5
#define DYNAMIC_PRIORITIES 10
#define QUANTUM_BASE 5
#define NICE_MAX 10

typedef struct task_t {
    int id;
    int priority;
    int base_priority;
    int quantum;
    int remaining_quantum;
    int nice;
    struct task_t *next;
} task_t;

void scheduler_init(void);
void scheduler_add_task(task_t *t);
task_t *scheduler_next_task(void);
void scheduler_on_quantum_expire(task_t *t);
void scheduler_on_block(task_t *t);
void scheduler_on_unblock(task_t *t);
void scheduler_dump(void);
int scheduler_nice(int nice_value);

#endif