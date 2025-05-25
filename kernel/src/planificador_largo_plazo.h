#ifndef PLANIFICADOR_LARGO_PLAZO_H_
#define PLANIFICADOR_LARGO_PLAZO_H_
#include<utils/utils.h>
#include<pcb.h>
#include<kernel.h>

typedef enum {
    FIFO,
    MENOR_MEMORIA
} t_algoritmo_planificacion;

extern t_queue* cola_new;
extern t_queue* cola_ready;
extern t_queue* cola_susp_ready;

extern pthread_mutex_t mutex_new;
extern pthread_mutex_t mutex_ready;

extern sem_t sem_procesos_en_new;
extern sem_t sem_procesos_en_memoria;

extern int pid_global;

void inicializar_planificador_lp(char* algoritmo_planificacion_lp, char* algoritmo_planificacion_cp);
bool enviar_pedido_memoria(t_pcb* pcb);
void chequear_algoritmo_planificacion (char* algoritmo_planificacion_lp, char* algoritmo_planificacion_cp);
void planificador_largo_plazo();
void crear_proceso(int tamanio_proceso);

#endif