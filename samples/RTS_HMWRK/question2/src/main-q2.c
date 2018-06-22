#include <zephyr.h>

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#else
#include <misc/printk.h>
#endif

#include <misc/__assert.h>

#define SEMAPHORES 1
#define MUTEXES 2
#define STACKS 3
#define FIFOS 4
#define LIFOS 5

/**************************************/
/* control the behaviour of the demo **/

#ifndef DEBUG_PRINTF
#define DEBUG_PRINTF 0
#endif

#ifndef NUM_PHIL
#define NUM_PHIL 3
#endif

#ifndef STATIC_OBJS
#define STATIC_OBJS 0
#endif

#ifndef FORKS
#define FORKS MUTEXES
#if 0
#define FORKS SEMAPHORES
#define FORKS STACKS
#define FORKS FIFOS
#define FORKS LIFOS
#endif
#endif

#ifndef SAME_PRIO
#define SAME_PRIO 0
#endif

/* end - control behaviour of the demo */
/***************************************/

#define STACK_SIZE 768

/*
 * There are multiple threads doing printfs and they may conflict.
 * Therefore use puts() instead of printf().
 */
#if defined(CONFIG_STDOUT_CONSOLE)
#define PRINTF(...) { char output[256]; \
		      sprintf(output, __VA_ARGS__); puts(output); }
#else
#define PRINTF(...) printk(__VA_ARGS__)
#endif

#if DEBUG_PRINTF
#define PR_DEBUG PRINTF
#else
#define PR_DEBUG(...)
#endif

#include "phil_obj_abstract.h"

#define fork(x) (forks[x])

static void set_phil_state_pos(int id)
{
#if !DEBUG_PRINTF
	PRINTF("\x1b[%d;%dH", id + 1, 1);
#endif
}

#include <stdarg.h>
static void print_phil_state(int id, const char *fmt, s32_t delay)
{
	int prio = k_thread_priority_get(k_current_get());

	set_phil_state_pos(id);

	PRINTF("Philosopher %d [%s:%s%d] ",
	       id, prio < 0 ? "C" : "P",
	       prio < 0 ? "" : " ",
	       prio);

	if (delay) {
		PRINTF(fmt, delay < 1000 ? " " : "", delay);
	} else {
		PRINTF(fmt, "");
	}

	PRINTF("\n");
}

static s32_t get_random_delay(int id, int period_in_ms)
{

	k_enable_sys_clock_always_on();
	s32_t delay = (k_uptime_get_32()/100 * (id + 1)) & 0x1f;
	k_disable_sys_clock_always_on();

	/* add 1 to not generate a delay of 0 */
	s32_t ms = (delay + 1) * period_in_ms;

	return ms;
}

static inline int is_last_philosopher(int id)
{
	return id == (NUM_PHIL - 1);
}

static void busy_wait_ms(int32_t ms){

	int tempo = k_uptime_get();
	int i = 0;
	while(i<ms){
		if(tempo != k_uptime_get()){
			i = i+10;
			tempo = k_uptime_get();
		}
	}	
	/*int32_t deadline = k_uptime_get() + ms;

	volatile int32_t now = k_uptime_get();

	while (now < deadline) {
		now = k_uptime_get();
		k_yield();
	}*/
}

void philosopher(void *id, void *ct, void *periodo)
{	
	int my_id = (int)id;
	int i =0;
	//int my_ct = (int)ct;
	int my_peri = (int) periodo;
	uint32_t my_ct = (uint32_t) ct;	
	int64_t tempo;
	int64_t tiempo;

	/*ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	fork_t fork1;
	fork_t fork2;

	int my_id = (int)id;

	/* Djkstra's solution: always pick up the lowest numbered fork first 
	if (is_last_philosopher(my_id)) {
		fork1 = fork(0);
		fork2 = fork(my_id);
	} else {
		fork1 = fork(my_id);
		fork2 = fork(my_id + 1);
	}
	*/
	while (1) {
		s32_t delay;
		//k_sched_lock();		

		k_mutex_lock(&my_mutex, K_FOREVER);

		//TEMPO QUE "ENTRA" NA THREAD
		int t = k_uptime_get();
		//while(t>my_peri){
		//	t = t/10;
		//}

		//TUDO MULTIPLICADO POR 1000 POR CAUSA DO PASSO DO TIMER
		delay =1000*(my_peri-my_ct);

		//FUNÇÃO PARA DEIXAR A THREAD OCUPADA, DESCRITA ACIMA
		busy_wait_ms(1000*my_ct);
		
		//TEMPO AO FINAL DO EXECUÇÃO
		tempo = k_uptime_get();

		//LAÇO PARA CALCULAR O NOVO PERIODO		
		while((((int)delay+tempo)/1000)%(my_peri) !=0){
			delay = (int)delay-1000;
		}
	
		//PRINT PARA PEGAR OS TEMPOS DE COMEÇO E DE TÉRMINO
		PRINTF("THREAD: %d  %d    %d\n", my_id,t,tempo);

		//A THREAD DORME COM O DELAY CALCULADO PRA SER CHAMADA
		//k_sched_unlock();
		k_mutex_unlock(&my_mutex);		
		k_sleep(delay);	
		
		//CÓDIGOS DAS TENTATIVAS PASSADAS

		//for(i = 0; i< 10000*my_ct; i=i+1){
		//	PRINTF("thread :%d  %d    %d\n",my_id,tempo, i);			
		//	k_yield();				
		//}
		//k_busy_wait(1000*my_ct);		
		
		/*tempo = k_uptime_get();
		
		tiempo = my_peri*10000;
		*/
		//if(k_uptime_get()%my_peri == 0){
		//	PRINTF("deveria acordar");			
			//k_wakeup(&threads(my_id));
		//}

		//k_yield();	
	}

}

static int new_prio(int phil)
{
#if defined(CONFIG_COOP_ENABLED) && defined(CONFIG_PREEMPT_ENABLED)
#if SAME_PRIO
	return 0;
#else
	return -(phil - (NUM_PHIL/2));
#endif
#else
#if defined(CONFIG_COOP_ENABLED)
	return -phil - 2;
#elif defined(CONFIG_PREEMPT_ENABLED)
	return phil;
#else
	#error unpossible
#endif
#endif
}

static void init_objects(void)
{
#if !STATIC_OBJS
	for (int i = 0; i < NUM_PHIL; i++) {
		fork_init(fork(i));
	}
#endif
}

static void start_threads(void)
{
	/*
	 * create two coop. threads (prios -2/-1) and four preemptive threads
	 * : (prios 0-3)
	 */

	//PRIMEIRA THREAD
	//int prio1 = new_prio(0);

	k_thread_create(&threads[0], &stacks[0][0], STACK_SIZE,
			philosopher, (void *)0, (void *)3, (void *)7, 0,
			K_INHERIT_PERMS, K_FOREVER);

	k_thread_start(&threads[0]);

	//SEGUNDA THREAD
	//int prio2 = new_prio(1);

	k_thread_create(&threads[1], &stacks[1][0], STACK_SIZE,
			philosopher, (void *)1, (void *) 3, (void *)12, 1,
			K_INHERIT_PERMS, K_FOREVER);

	k_thread_start(&threads[1]);

	//TERCEIRA THREAD
	//int prio3 = new_prio(2);

	k_thread_create(&threads[2], &stacks[2][0], STACK_SIZE,
			philosopher, (void *)2, (void *) 5, (void *) 20, 2,
			K_INHERIT_PERMS, K_FOREVER);

	k_thread_start(&threads[2]);
	
}

void main(void)
{
#if CONFIG_TIMESLICING
	k_sched_time_slice_set(10000, 0);
#endif
//#define &my_mutex = 0x00414000

	//init_objects();
	k_mutex_init(&my_mutex);
	start_threads();
}
