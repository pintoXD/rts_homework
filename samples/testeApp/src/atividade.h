#ifndef atividade__h
#define atividade__h

static struct k_thread __kernel threads[NUM_THR];
typedef struct k_thread *k_tid_t;
static K_THREAD_STACK_ARRAY_DEFINE(stacks, NUM_THR, STACK_SIZE);

void tarefa (bool done, int period, int computation);


#endif