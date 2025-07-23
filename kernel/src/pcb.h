#ifndef PCB_H_
#define PCB_H_
#include<utils/utils.h>
#include<commons/temporal.h>
#include<kernel.h>


typedef enum {
    NEW,
    READY,
    EXEC,
    BLOCKED,  
    SUSP_READY,
    SUSP_BLOCKED,
    EXIT,
    CANTIDAD_ESTADOS
} t_estado_proceso;

typedef struct
{
    int pid;
    int pc;
    int metricas_estado[CANTIDAD_ESTADOS];
    uint64_t metricas_tiempo[CANTIDAD_ESTADOS];
    t_estado_proceso estado_actual;
    t_temporal* temporal_estado;
    int tamanio;
    int estimacion_rafaga;
    int rafaga_real_anterior;
    t_temporal* temporal_blocked;
} t_pcb;

typedef struct
{
    int pid;
    int pc;
    int tamanio;
} t_pcb_cpu;

t_pcb* obtener_pcb(int pid);
t_pcb* crear_pcb(int pid, int tamanio_proceso);
void cambiar_estado(t_pcb* pcb, t_estado_proceso nuevo_estado);
void borrar_pcb(t_pcb* pcb);
void actualizar_estimacion_rafaga(t_pcb* pcb);
void chequear_sjf_con_desalojo(t_pcb* nuevo);
void asignar_timer_blocked(t_pcb* pcb);
t_pcb* sacar_pcb_de_cola(t_queue* cola, int pid);
char* parsear_estado(int estado);
void log_metricas_estado(t_pcb* pcb);

#include<planificador_largo_plazo.h>

#endif