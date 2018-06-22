#include <zephyr.h>
#include <kernel.h>

#if defined(CONFIG_STDOUT_CONSOLE)
	#include <stdio.h>
#else
	#include <misc/printk.h>
#endif

#include <misc/__assert.h>

#include <stdarg.h>

#ifndef NUM_THR														// Numero de threads
#define NUM_THR 3
#endif

#define STACK_SIZE 768

void tarefa (bool done, int period, int computation){					//Função que vai executar dentro das threads, com id, periodo e tempo de computação
	int newid = id;
	printf("A tarefa %d esta executando", newid);
}

static void start_threads(void)										// Iniciar as threads, alocadas manualmente para setar periodos e computation time
{

	k_thread_create(&threads[1], &stacks[1][0], STACK_SIZE,
				tarefa, 0, 7, 3, 3,
				K_USER, K_FOREVER);

	k_thread_start(&threads[1]);

	k_thread_create(&threads[2], &stacks[2][0], STACK_SIZE,
				tarefa, 0, 12, 3, 2,
				K_USER, K_FOREVER);

	k_thread_start(&threads[2]);
				
	k_thread_create(&threads[3], &stacks[3][0], STACK_SIZE,
				tarefa, 0, 20, 5, 1,
				K_USER, K_FOREVER);

	k_thread_start(&threads[3]);
	
}

void main(void){
	
	start_threads();	//começar as threads
	int i = 0;			//operador que irá funcionar como marco temporal
	do{
		
		if(i==0){		//Inicío de todo procedimento, suspende as duas menores, e deixa a maior executando
			k_thread_suspend(&threads[2]);	//&threads refere-se à struct na qual as threads foram instanciadas
			k_thread_suspend(&threads[3]);
		}
		
		//k_current_get pega a thread que ta rodando atualmente, retorna o endereço
		if(k_current_get() == &threads[1]){		//Primeiro caso, thread com maior prioridade, nada pode impedi-la, dou delay de 3 segundos, a suspendo
			k_seconds(3);						// seto o parametro done como 1, adiciona duas unidades pro i, pra atualizar com o global, em primeira instancia
			k_thread_suspend(&threads[1]);		// chamo a segunda thread
			*threads[1].done=1;
			i=i+2;
			if(*threads[2].done == 0)			//Quero saber se *threads.done retorna o valor da variavel, realmente não sei
				k_wakeup(&threads[2]);			
			else if(*threads[3].done ==0)
				k_wakeup(*threads[3]);
		}
		
		if(k_current_get()==&threads[2]){
			for(int j =0; j <3; j++){			//Há o problema, e se for chamada dps de uma preempção e faltar apenas um segundo pra ser concluida?
				k_seconds(1);					//ainda não sei como solucionar isso 
				i = i+1;
				if(i % 7 ==0){
					k_thread_suspend(&threads[2]);
					k_wakeup(&threads[1]);
					*threads[1].done = 0;
					break;	//Queria sair do laço, pra ele voltar por do/while e verificar a primeira condição, não sei se vai sair de tu
				}
			}
			*threads[2].done =1;	//Só aconteceria caso a primeira terminasse
		}
		
		if(k_current_get()==&threads[3]){
			for(int j =0; j <5; j++){
				k_seconds(1);
				i = i+1;
				if(i % 7 ==0){
					k_thread_suspend(&threads[3]);
					k_wakeup(&threads[1]);
					break;
				}
				if(i % 12 ==0){
					k_thread_suspend(&threads[3]);
					k_wakeup(&threads[2]);
					break;
				}
			}
		}	
	
		if(i % 7 ==0){
			*threads[1].done=0;
			k_thread_suspend(k_current_get());
			k_wakeup(&threads[1]);
		}

		if(i%12 == 0 && *threads[1].done == 1){
			k_thread_suspend(k_current_get());
			k_wakeup(&threads[2]);
		}

		if(i%20==0 && *threads[1].done==1 && *threads[2].done ==1){
			k_thread_suspend(k_current_get());
			k_wakeup(&threads[3]);
		}

		k_seconds(1);	//Tempo global
		i=i+1;			//Indicador do tempo global
	}while(1);
}