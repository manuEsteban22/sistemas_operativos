#include<planificador_mediano_plazo.h>


void planificador_mediano_plazo(){
    log_trace(logger, "arranque a correr plani mediano plazo");
    while(1){
        
        sem_wait(&sem_procesos_en_blocked);
        log_trace(logger, "arranque una vuelta de plani mediano plazo");

    if (!queue_is_empty(cola_blocked)){
            t_pcb* pcb = queue_peek(cola_blocked); 

            int tiempo_bloqueado = temporal_gettime(pcb->temporal_blocked);

            if (tiempo_bloqueado >= tiempo_suspension){
                queue_pop(cola_blocked); 
                cambiar_estado(pcb, SUSP_BLOCKED);

                pthread_mutex_lock(&mutex_susp_blocked);
                queue_push(cola_susp_blocked, pcb);
                informar_memoria_suspension(pcb->pid);
                pthread_mutex_unlock(&mutex_susp_blocked);

                log_info(logger, "PID %d pasa a susp blocked por exceder tiempo", pcb->pid);

                temporal_destroy(pcb->temporal_blocked);
            }

        }
    }
}

void informar_memoria_suspension(int pid){
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OC_SUSP);
    agregar_a_paquete(paquete, &pid, sizeof(int));
    socket_memoria = operacion_con_memoria();
    enviar_paquete(paquete, socket_memoria, logger);
    cerrar_conexion_memoria(socket_memoria);
    borrar_paquete(paquete);
}