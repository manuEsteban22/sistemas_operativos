#ifndef PLANIFICADOR_LARGO_PLAZO_H_
#define PLANIFICADOR_LARGO_PLAZO_H_
#include<utils/utils.h>
#include<pcb.h>
#include<kernel.h>

typedef enum {
    FIFO,
    PMCP,   //proceso mas chico primero
    SJF,
    SRT
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

extern sem_t sem_plp;
extern sem_t sem_procesos_ready;
extern sem_t sem_procesos_en_blocked;

extern int pid_global;

void finalizar_proceso(t_pcb* pcb);
void inicializar_planificador_lp();
bool enviar_pedido_memoria(t_pcb* pcb);
void chequear_algoritmo_planificacion ();
void planificador_largo_plazo();
int crear_proceso(int tamanio_proceso);
void insertar_en_orden_por_memoria(t_queue* cola, t_pcb* nuevo);
void enviar_finalizacion_a_memoria(int pid);
int operacion_con_memoria();
void cerrar_conexion_memoria(int socket);
void informar_memoria_desuspension(int pid);
void pushear_a_new(t_pcb* pcb);

#include<planificador_corto_plazo.h>
#endif