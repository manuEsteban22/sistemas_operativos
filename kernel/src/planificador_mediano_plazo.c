#include<planificador_mediano_plazo.h>

void inicializar_planificador_mp(){
    cola_susp_blocked = queue_create();
}

void planificador_mediano_plazo(){
    int estado_anterior;
    log_trace(logger, "arranque a correr plani mediano plazo");
    while(1){
        
        sem_wait(&sem_procesos_en_blocked);
        log_trace(logger, "arranque una vuelta de plani mediano plazo");

        pthread_mutex_lock(&mutex_blocked);
        if (!queue_is_empty(cola_blocked)){
            t_pcb* pcb = queue_peek(cola_blocked); 
            
            if (pcb == NULL) {
                log_error(logger, "ERROR: La PCB en cola_blocked es NULL");
                pthread_mutex_unlock(&mutex_blocked);
                continue;
            }
            if (pcb->temporal_blocked == NULL) {
                log_error(logger, "ERROR: temporal_blocked de la PCB PID %d es NULL", pcb->pid);
                pthread_mutex_unlock(&mutex_blocked);
                continue;
            }


            int64_t tiempo_bloqueado = temporal_gettime(pcb->temporal_blocked);

            if (tiempo_bloqueado >= tiempo_suspension){
                queue_pop(cola_blocked); 

                estado_anterior = pcb->estado_actual;
                cambiar_estado(pcb, SUSP_READY);
                log_info(logger, "(%d) Pasa del estado %s al estado %s",pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));


                pthread_mutex_lock(&mutex_susp_blocked);
                queue_push(cola_susp_blocked, pcb);
                informar_memoria_suspension(pcb->pid);
                pthread_mutex_unlock(&mutex_susp_blocked);

                log_info(logger, "PID %d pasa a susp blocked por exceder tiempo", pcb->pid);

                temporal_destroy(pcb->temporal_blocked);
            }

        }
        pthread_mutex_unlock(&mutex_blocked);
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