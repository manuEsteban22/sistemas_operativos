#include <planificador_corto_plazo.h>
#include <planificador_largo_plazo.h>

t_algoritmo_planificacion algoritmo_cp;

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

void ejecutar_proceso(t_pcb* pcb, int socket_dispatch){
    cambiar_estado(pcb, EXEC);
    int pid = pcb->pid;
    int pc = pcb->pc;

    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OC_EXEC);
    agregar_a_paquete(paquete, pid, sizeof(int));
    agregar_a_paquete(paquete, pc, sizeof(int));
    enviar_paquete(paquete, socket_dispatch, logger);
    borrar_paquete(paquete);

    log_info(logger, "envie el proceso PID=%d a CPU - PC=%d", pid, pc);
}