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
    double estimacion_rafaga;
    double rafaga_real_anterior;
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
void actualizar_estimacion_rafaga(t_pcb* pcb, t_config* config);
void chequear_sjf_con_desalojo(t_pcb* nuevo);
void asignar_timer_blocked(t_pcb* pcb);

#include<planificador_largo_plazo.h>

#endif