#ifndef PLANIFICADOR_CORTO_PLAZO_H_
#define PLANIFICADOR_CORTO_PLAZO_H_
#include<utils/utils.h>

extern sem_t cpus_disponibles;

#include <planificador_largo_plazo.h>
#include <pthread.h>
#include <commons/collections/queue.h>
#include <pcb.h>
#include <kernel.h>

t_pcb* planificador_corto_plazo ();
t_pcb* planificar_sjf(t_queue* cola);
void* planificador_corto_plazo_loop(void*);

#endif