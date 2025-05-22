#ifndef PCB_H_
#define PCB_H_
#include<utils/utils.h>
#include<commons/temporal.h>

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
} t_pcb;

t_pcb* crear_pcb(int pid);
void cambiar_estado(t_pcb* pcb, t_estado_proceso nuevo_estado);


#endif