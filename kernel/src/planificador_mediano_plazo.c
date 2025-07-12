#include<planificador_mediano_plazo.h>


void planificador_mediano_plazo() {
    cambiar_estado_pcb(pcb, SUSP_BLOCKED);
    pthread_mutex_unlock(&mutex_susp_ready);
}
