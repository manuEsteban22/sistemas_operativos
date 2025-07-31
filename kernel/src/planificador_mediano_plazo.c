#include<planificador_mediano_plazo.h>

void* trackear_bloqueo(void* args){
    t_pcb* pcb = (t_pcb*)args;
    usleep(1000 * tiempo_suspension);
    
    log_error(logger, "pmp 7: pthread_mutex_lock(&pcb->mutex_pcb);");
    pthread_mutex_lock(&pcb->mutex_pcb);
    if(pcb->estado_actual == BLOCKED){
        
        log_error(logger,"Se bloqueo en pmp:10");
        log_error(logger, "pmp 12: pthread_mutex_lock(&mutex_blocked);");
        pthread_mutex_lock(&mutex_blocked);
        
        queue_pop(cola_blocked); 
        log_error(logger, "pmp 16: pthread_mutex_unlock(&mutex_blocked);");
        pthread_mutex_unlock(&mutex_blocked);

        int estado_anterior = pcb->estado_actual;
        cambiar_estado_sin_lock(pcb, SUSP_BLOCKED);
        //cambiar_estado(pcb, SUSP_BLOCKED);
        log_info(logger, "(%d) Pasa del estado %s al estado %s",pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));
        
        pthread_mutex_lock(&mutex_susp_blocked);
        queue_push(cola_susp_blocked, pcb);
        informar_memoria_suspension(pcb->pid);
        pthread_mutex_unlock(&mutex_susp_blocked);

        log_info(logger, "PID %d pasa a susp blocked por exceder tiempo", pcb->pid);

        if(pcb->temporal_blocked != NULL){
            temporal_destroy(pcb->temporal_blocked);
            pcb->temporal_blocked = NULL;
        }

        
    }
    log_error(logger, "pmp 37: pthread_mutex_unlock(&pcb->mutex_pcb);");
    pthread_mutex_unlock(&pcb->mutex_pcb);
    
    return NULL;
}

void inicializar_planificador_mp(){
    cola_susp_blocked = queue_create();
}

void planificador_mediano_plazo(){
    log_trace(logger, "Arranque a correr plani mediano plazo");
    while(1){
        
        sem_wait(&sem_procesos_en_blocked);
        log_trace(logger, "arranque una vuelta de plani mediano plazo");

        log_error(logger,"pmp 54: pthread_mutex_lock(&mutex_blocked);");
        pthread_mutex_lock(&mutex_blocked);
        
        bool hay_bloqueados = !queue_is_empty(cola_blocked);
        if (hay_bloqueados){
            t_pcb* pcb = queue_peek(cola_blocked); 
            log_error(logger, "pmp 60: pthread_mutex_unlock(&mutex_blocked);");
            pthread_mutex_unlock(&mutex_blocked);
            
            if (pcb == NULL) {
                log_error(logger, "ERROR: La PCB en cola_blocked es NULL");
                //pthread_mutex_unlock(&mutex_blocked);
                continue;
            }
            if (pcb->temporal_blocked == NULL) {
                log_error(logger, "ERROR: temporal_blocked de la PCB PID %d es NULL", pcb->pid);
                //pthread_mutex_unlock(&mutex_blocked);
                continue;
            }

            pthread_t hilo_chequear_suspension;
            int err = pthread_create(&hilo_chequear_suspension, NULL, trackear_bloqueo, pcb);
            if (err != 0) {
                log_error(logger, "Error al crear hilo de trackeo de bloqueo: %s", strerror(err));
            } else {
                pthread_detach(hilo_chequear_suspension);
            }

            //pthread_mutex_unlock(&mutex_blocked);
            //a partir de aca quiero cambiar la implementacion para que haga un hilo y ese hilo haga un usleep de x tiempo y pasado ese tiempo se fije si sigue bloqueado lo pone en susp blocked o si lo desbloquearon mata el hilo


            //int tiempo_bloqueado = temporal_gettime(pcb->temporal_blocked);
            //if (tiempo_bloqueado >= tiempo_suspension){

        } else{
            log_error(logger, "pmp 90: pthread_mutex_unlock(&mutex_blocked);");
            pthread_mutex_unlock(&mutex_blocked);
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
void informar_memoria_desuspension(int pid){
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OC_DESUSP);
    agregar_a_paquete(paquete, &pid, sizeof(int));
    socket_memoria = operacion_con_memoria();
    enviar_paquete(paquete, socket_memoria, logger);
    borrar_paquete(paquete);

    int confirmacion = recibir_operacion(socket_memoria);
    if(confirmacion != OK){log_error(logger, "No se recibio confirmacion de la desuspension (%d)", confirmacion);}
    cerrar_conexion_memoria(socket_memoria);
}