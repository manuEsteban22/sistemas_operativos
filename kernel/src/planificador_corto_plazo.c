#include <planificador_corto_plazo.h>

t_pcb* planificar_proceso_fifo() {
    pthread_mutex_lock(&mutex_ready);

    if (queue_is_empty(cola_ready)) {
        pthread_mutex_unlock(&mutex_ready);
        return NULL;
    }

    t_pcb* pcb = queue_pop(cola_ready);
    pthread_mutex_unlock(&mutex_ready);

    return pcb;
}