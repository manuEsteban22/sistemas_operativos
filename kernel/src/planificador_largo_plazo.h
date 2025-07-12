#ifndef PLANIFICADOR_LARGO_PLAZO_H_
#define PLANIFICADOR_LARGO_PLAZO_H_
#include<utils/utils.h>
#include<pcb.h>
#include<kernel.h>

typedef enum {
    FIFO,
    MENOR_MEMORIA,
    SJF_SIN_DESALOJO,
    SJF_CON_DESALOJO
} t_algoritmo_planificacion;

//hacer un archivo aparte para las colas
extern t_queue* cola_new;
extern t_queue* cola_ready;
extern t_queue* cola_susp_ready;
extern t_queue* cola_susp_blocked;
extern t_queue* cola_blocked;

extern pthread_mutex_t mutex_new;
extern pthread_mutex_t mutex_ready;
extern pthread_mutex_t mutex_blocked;
extern pthread_mutex_t mutex_susp_blocked;
extern pthread_mutex_t mutex_susp_ready;

extern sem_t sem_procesos_en_new;
extern sem_t sem_procesos_en_memoria;
extern sem_t sem_procesos_ready;
extern sem_t sem_procesos_en_blocked;

extern int pid_global;

void inicializar_planificador_lp(char* algoritmo_planificacion_lp);
bool enviar_pedido_memoria(t_pcb* pcb);
void chequear_algoritmo_planificacion (char* algoritmo_planificacion_lp);
void planificador_largo_plazo();
void crear_proceso(int tamanio_proceso);
void insertar_en_orden_por_memoria(t_queue* cola, t_pcb* nuevo);

#endif