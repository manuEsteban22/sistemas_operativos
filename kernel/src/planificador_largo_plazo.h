#ifndef PLANIFICADOR_LARGO_PLAZO_H_
#define PLANIFICADOR_LARGO_PLAZO_H_
#include<utils/utils.h>
#include<pcb.h>
#include<kernel.h>

extern t_queue* cola_new;
extern t_queue* cola_ready;

extern pthread_mutex_t mutex_new;
extern pthread_mutex_t mutex_ready;

extern sem_t sem_procesos_en_new;
extern sem_t sem_procesos_en_memoria;

extern int pid_global;

void inicializar_planificador_lp();
bool enviar_pedido_memoria(t_pcb* pcb);

#endif