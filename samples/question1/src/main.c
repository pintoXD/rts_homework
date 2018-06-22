/*
 * Copyright (c) 2011-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#else
#include <misc/printk.h>
#endif

#include <misc/__assert.h>

#define MUTEXES 2

#ifndef DEBUG_PRINTF
#define DEBUG_PRINTF 0
#endif

#ifndef NUM_PHIL
#define NUM_PHIL 6
#endif

#ifndef STATIC_OBJS
#define STATIC_OBJS 0
#endif

#ifndef FORKS
#define FORKS MUTEXES
#endif

#ifndef SAME_PRIO
#define SAME_PRIO 0
#endif

/* end - control behaviour of the demo */
/***************************************/

#define STACK_SIZE 768

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

static void filosofo_position(int id)
{
#if !DEBUG_PRINTF
	PRINTF("\x1b[%d;%dH", id + 1, 1);
#endif
}

#include <stdarg.h>
static void print_filosofo_estado(int id, const char *flsf_msg, s32_t delay)
{
	int prio = k_thread_priority_get(k_current_get());

	filosofo_position(id);
	//Esse bloco abaixo basicamente o tipo de prioridade da thread. Dá pra apagar.
	PRINTF("Filosofo %d [%s:%s%d] ",
	       id, prio < 0 ? "C" : "P",
	       prio < 0 ? "" : " ",
	       prio);


	if (delay) {
		PRINTF(flsf_msg, delay < 1000 ? " " : "", delay);
	} else {
		PRINTF(flsf_msg, "");
	}

	PRINTF("\n");
}

//Nome sugerido: gen_shuffle_delay()
			// gerarDelay()
static s32_t delay_gerar(int id, int period_in_ms)
{
	/*
	 * The random delay is unit-less, and is based on the philosopher's ID
	 * and the current uptime to create some pseudo-randomness. It produces
	 * a value between 0 and 31.
	 */


	/*

		Gerador de delay aleatório baseado no id do filósofo e no tempo atual 
		do sistema desde que ele foi ligado.

	*/

	k_enable_sys_clock_always_on();
	s32_t delay = (k_uptime_get_32()/100 * (id + 1)) & 0x1f;
	k_disable_sys_clock_always_on();

	//Gambiarra: Adiciona 1 pra não gerar um delay 0, se for o caso
	s32_t ms = (delay + 1) * period_in_ms;

	return ms;
}

static inline int is_last_philosopher(int id)
{
	return id == (NUM_PHIL - 1);
}

void filosofo(void *id, void *unused1, void *unused2)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	fork_t fork1;
	fork_t fork2;

	int my_id = (int)id;

	// Pegar o garfo de menor número primeiro (Solução de Djkstra)
	if (is_last_philosopher(my_id)) {
		fork1 = fork(0);
		fork2 = fork(my_id);
	} else {
		fork1 = fork(my_id);
		fork2 = fork(my_id + 1);
	}

	while (1) {
		s32_t delay;

		print_filosofo_estado(my_id, "       Faminto       ", 0);
		take(fork1);
		print_filosofo_estado(my_id, "   Segurando garfo 1  ", 0);
		take(fork2);

		delay = delay_gerar(my_id, 25);
		print_filosofo_estado(my_id, "  Comendo  [ %s%d ms ] ", delay);
		k_sleep(delay);

		drop(fork2);
		print_filosofo_estado(my_id, "   Soltando garfo 1  ", 0);
		drop(fork1);

		delay = delay_gerar(my_id, 25);
		print_filosofo_estado(my_id, " Pensando [ %s%d ms ] ", delay);
		k_sleep(delay);
	}

}

static int new_prio(int phil)
{

	//Não entendi essa função
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
	for (int i = 0; i < NUM_PHIL; i++) {
		int prio = new_prio(i);

		k_thread_create(&threads[i], &stacks[i][0], STACK_SIZE,
				filosofo, (void *)i, NULL, NULL, prio,
				K_USER, K_FOREVER);
		//K_USER = Restrições de usuários		
		//K_FOREVER = Timeout delay infinito
		k_object_access_grant(fork(i), &threads[i]);
		k_object_access_grant(fork((i + 1) % NUM_PHIL), &threads[i]);

		k_thread_start(&threads[i]);
	}
}

#define DEMO_DESCRIPTION  \
	"\x1b[2J\x1b[15;1H"   \
	"Hue\n"  \
	"----------------\n"  \
	"Implementação de um exemplo do jantar dos filósosfos\n" \
	" com diferentes prioridades assim como\n" \
	"%s %s e threads adormecidas\n", obj_init_type, fork_type_str

static void display_demo_description(void)
{
#if !DEBUG_PRINTF
	PRINTF(DEMO_DESCRIPTION);
#endif
}

void main(void)
{
	display_demo_description();
#if CONFIG_TIMESLICING
	k_sched_time_slice_set(5000, 0);
#endif

	init_objects();
	start_threads();
}
