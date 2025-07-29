#include<planificador_mediano_plazo.h>

void* trackear_bloqueo(void* args){
    t_pcb* pcb_bloqueado = (t_pcb)args;
    usleep(1000 * tiempo_suspension);
    pthread_mutex_lock(&mutex_blocked);
    if(pcb->BLOCKED){
        queue_pop(cola_blocked); 

        estado_anterior = pcb->estado_actual;
        cambiar_estado(pcb, SUSP_BLOCKED);
        log_info(logger, "(%d) Pasa del estado %s al estado %s",pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));
        
        pthread_mutex_lock(&mutex_susp_blocked);
        queue_push(cola_susp_blocked, pcb);
        informar_memoria_suspension(pcb->pid);
        pthread_mutex_unlock(&mutex_susp_blocked);

        log_info(logger, "PID %d pasa a susp blocked por exceder tiempo", pcb->pid);

        temporal_destroy(pcb->temporal_blocked);

        pthread_mutex_unlock(&mutex_blocked);
    }
}

void inicializar_planificador_mp(){
    cola_susp_blocked = queue_create();
}

void planificador_mediano_plazo(){
    int estado_anterior;
    log_trace(logger, "Arranque a correr plani mediano plazo");
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

            
            pthread_t hilo_chequear_suspension;
            pthread_create(&hilo_chequear_suspension, NULL, trackear_bloqueo, pcb);
            pthread_detach(hilo_chequear_suspension);

            //a partir de aca quiero cambiar la implementacion para que haga un hilo y ese hilo haga un usleep de x tiempo y pasado ese tiempo se fije si sigue bloqueado lo pone en susp blocked o si lo desbloquearon mata el hilo


            //int tiempo_bloqueado = temporal_gettime(pcb->temporal_blocked);
            //if (tiempo_bloqueado >= tiempo_suspension){

        }
        //pthread_mutex_unlock(&mutex_blocked);
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