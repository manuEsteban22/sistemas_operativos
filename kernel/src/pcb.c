#include<pcb.h>


t_pcb* crear_pcb(int pid, int tamanio_proceso) {
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->pid = pid;
    pcb->pc = 0;
    pcb->tamanio = tamanio_proceso;
    pcb->estado_actual = NEW;
    pcb->temporal_estado = temporal_create();
    //Inicializo el pcb en estado NEW y creo el cronometro

    for(int i = 0; i < CANTIDAD_ESTADOS; i++) {
        pcb->metricas_estado[i] = 0;
        pcb->metricas_tiempo[i] = 0;
        //inicializo ambos arrays en 0 para cada estado
    }

    pcb->metricas_estado[NEW]++;//sumo 1 a las veces que pasó por estado NEW
    return pcb;
}

//para ver a que pcb esta asignado dicho pid
t_pcb* obtener_pcb(int pid) {
    char* pid_str = string_itoa(pid);
    t_pcb* pcb = dictionary_get(tabla_pcbs, pid_str);
    free(pid_str);
    return pcb;
}

void cambiar_estado(t_pcb* pcb, t_estado_proceso nuevo_estado) {
    uint64_t duracion = temporal_gettime(pcb->temporal_estado);
    pcb->metricas_tiempo[pcb->estado_actual] += duracion;
    //devuelve el tiempo que tomó el cronometro y lo suma a la metrica de tiempo del estado actual

    pcb->estado_actual = nuevo_estado;
    pcb->metricas_estado[nuevo_estado]++;
    //cambia el estado del pcb y suma 1 a la metrica de estado del estado nuevo

    temporal_destroy(pcb->temporal_estado);
    pcb->temporal_estado = temporal_create();
    //borra el cronometro del estado anterior y crea uno nuevo
}

void borrar_pcb(t_pcb* pcb){
    temporal_destroy(pcb->temporal_estado);
    free(pcb);
}