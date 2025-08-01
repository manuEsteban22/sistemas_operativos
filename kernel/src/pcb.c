#include<pcb.h>
#include<planificador_corto_plazo.h>

t_pcb* crear_pcb(int pid, int tamanio_proceso) {
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->pid = pid;
    pcb->pc = 0;
    pcb->tamanio = tamanio_proceso;
    pcb->estado_actual = NEW;
    pcb->temporal_estado = temporal_create();
    pcb->temporal_blocked = NULL;
    //Inicializo el pcb en estado NEW y creo el cronometro

    for(int i = 0; i < CANTIDAD_ESTADOS; i++) {
        pcb->metricas_estado[i] = 0;
        pcb->metricas_tiempo[i] = 0;
        //inicializo ambos arrays en 0 para cada estado
    }

    pcb->metricas_estado[NEW]++;//sumo 1 a las veces que pasó por estado NEW

    pcb->estimacion_rafaga = estimacion_inicial;
    pcb->rafaga_real_anterior = 0;
    pthread_mutex_init(&pcb->mutex_pcb, NULL);

    log_info(logger, "%d Se crea el proceso - Estado: NEW", pcb->pid);

    return pcb;
}

//para ver a que pcb esta asignado dicho pid
t_pcb* obtener_pcb(int pid) {
    char* pid_str = string_itoa(pid);
   // log_error(logger, "34: pthread_mutex_locK(&mutex_tabla_pcbs);");
    pthread_mutex_lock(&mutex_tabla_pcbs);
    t_pcb* pcb = dictionary_get(tabla_pcbs, pid_str);
   // log_error(logger, "pthread_mutex_unlock(&mutex_tabla_pcbs);");
    pthread_mutex_unlock(&mutex_tabla_pcbs);
    if (pcb == NULL) {
        log_error(logger, "Error para obtener PCB NULL");
    }
    free(pid_str);
    return pcb;
}

bool cpu_esta_en_lista(int cpu_id){
    for(int i = 0; i < list_size(cpus_libres); i++){
        int* id = list_get(cpus_libres, i);
        if(*id == cpu_id){
            return true;
        }
    }
    return false;
}

void cambiar_estado_sin_lock(t_pcb* pcb, t_estado_proceso nuevo_estado) {
    if (pcb == NULL) {
        log_error(logger, "Error en cambio de estado PCB NULL");
        return;
    }
    int duracion = temporal_gettime(pcb->temporal_estado);
    pcb->metricas_tiempo[pcb->estado_actual] += duracion;
    //devuelve el tiempo que tomó el cronometro y lo suma a la metrica de tiempo del estado actual

    pcb->estado_actual = nuevo_estado;
    pcb->metricas_estado[nuevo_estado]++;
    //cambia el estado del pcb y suma 1 a la metrica de estado del estado nuevo
    if (nuevo_estado == READY && (strcmp(algoritmo_corto_plazo, "SRT") == 0)){
    chequear_sjf_con_desalojo(pcb);
    }

    temporal_destroy(pcb->temporal_estado);
    pcb->temporal_estado = temporal_create();
    //borra el cronometro del estado anterior y crea uno nuevo
}

void cambiar_estado(t_pcb* pcb, t_estado_proceso nuevo_estado) {
   // log_error(logger, "68:pthread_mutex_lock(&pcb->mutex_pcb);");
    pthread_mutex_lock(&pcb->mutex_pcb);
    cambiar_estado_sin_lock(pcb, nuevo_estado);
   // log_error(logger, "71: pthread_mutex_unlock(&pcb->mutex_pcb);");
    pthread_mutex_unlock(&pcb->mutex_pcb);
}

void borrar_pcb(t_pcb* pcb){
    if (pcb == NULL) return;
   // log_error(logger, "77: pthread_mutex_lock(&pcb->mutex_pcb);");
    pthread_mutex_lock(&pcb->mutex_pcb);
    char* pid_str = string_itoa(pcb->pid);

   // log_error(logger, "81: pthread_mutex_lock(&mutex_exec);");
    pthread_mutex_lock(&mutex_exec);
    int* cpu_id = dictionary_remove(tabla_exec, pid_str);
    //log_error(logger, "84  pthread_mutex_unlock(&mutex_exec);");
    pthread_mutex_unlock(&mutex_exec);

    if(cpu_id != NULL){
        free(cpu_id);
    }

   // log_error(logger, "92 pthread_mutex_lock(&mutex_tabla_pcbs);");
    pthread_mutex_lock(&mutex_tabla_pcbs);
    t_pcb* pcb_borrar = dictionary_remove(tabla_pcbs, pid_str);
    //log_error(logger, "95: pthread_mutex_unlock(&mutex_tabla_pcbs);");
    pthread_mutex_unlock(&mutex_tabla_pcbs);

    free(pid_str);
   // log_error(logger, "pthread_mutex_unlock(&pcb->mutex_pcb);");
    pthread_mutex_unlock(&pcb->mutex_pcb);

    pthread_mutex_destroy(&pcb->mutex_pcb);
    temporal_destroy(pcb->temporal_estado);
    free(pcb);
    

}

void actualizar_estimacion_rafaga(t_pcb* pcb) {
    if (pcb == NULL) {
        log_error(logger, "No se pudo actualizar la estimacion, el PCB es NULL");
        return;
    }
   // log_error(logger, "bloquea pcb pcb:102");
    pthread_mutex_lock(&pcb->mutex_pcb);
    double rafaga_real = temporal_gettime(pcb->temporal_estado);
    pcb->rafaga_real_anterior = rafaga_real;

    log_debug(logger, "Se actualizo la estimacion de PID %d - Rafaga %f - Estimacion previa %f", pcb->pid, pcb->rafaga_real_anterior, pcb->estimacion_rafaga);
    double nueva_estimacion = (alfa * rafaga_real) + ((1 - alfa) * pcb->estimacion_rafaga);
    pcb->estimacion_rafaga = nueva_estimacion;
    log_debug(logger, "Nueva estimacion = %f", nueva_estimacion);
    temporal_destroy(pcb->temporal_estado);
    pcb->temporal_estado = temporal_create();
   // log_error(logger, "125: pthread_mutex_unlock(&pcb->mutex_pcb);");
    pthread_mutex_unlock(&pcb->mutex_pcb);
}

void chequear_sjf_con_desalojo(t_pcb* nuevo) {
    if (strcmp(algoritmo_corto_plazo, "SRT") != 0){
        return;
    }

    if (!hay_proceso_en_exec()){
        return;
    }
    
    t_pcb* ejecutando = obtener_proceso_en_exec();
    double tiempo_ejecutando = temporal_gettime(ejecutando->temporal_estado);
    double estimacion_restante = ejecutando->estimacion_rafaga - tiempo_ejecutando;

    if(estimacion_restante < 0){
        log_debug(logger, "La estimacion restante fue menor a 0");
        estimacion_restante = 0;
    }
    log_debug(logger, "PID %d (ejecutando) - Estimación restante: %.2f", ejecutando->pid, estimacion_restante);
    log_debug(logger, "PID %d (nuevo) - Estimación: %.2f", nuevo->pid, nuevo->estimacion_rafaga);
    
    //el chequeo de aca tiene que ser con la estimacion - tiempo ejecutado
    if (nuevo->estimacion_rafaga < estimacion_restante) {
        char* pid_str = string_itoa(ejecutando->pid);
       // log_error(logger, "152 pthread_mutex_lock(&mutex_exec);");
        pthread_mutex_lock(&mutex_exec);
        int* cpu_id_ptr = dictionary_get(tabla_exec, pid_str);
       // log_error(logger, "155 pthread_mutex_unlock(&mutex_exec);");
        pthread_mutex_unlock(&mutex_exec);
        log_debug(logger,"hasta aca llegue 7");
        if(cpu_id_ptr == NULL){
            log_error(logger, "chequear_sjf: no existe exec para PID %d", ejecutando->pid);
            free(pid_str);
            return;
        }

        int cpu_id = *cpu_id_ptr;
        free(pid_str);
        log_debug(logger,"hasta aca llegue 8");
        char* cpu_id_str = string_itoa(cpu_id);
       // log_error(logger, "168: pthread_mutex_lock(&mutex_interrupt);");
        pthread_mutex_lock(&mutex_interrupt);
        int* socket_interrupt_ptr = dictionary_get(tabla_interrupt, cpu_id_str);
       // log_error(logger, "171: pthread_mutex_unlock(&mutex_interrupt);");
        pthread_mutex_unlock(&mutex_interrupt);
        if(socket_interrupt_ptr != NULL){
            int socket_interrupt = *socket_interrupt_ptr;
            enviar_interrupcion_a_cpu(socket_interrupt);
            log_info(logger, "## (%d) - Desalojado por algoritmo SJF/SRT", ejecutando->pid);

            //log_error(logger, "178: pthread_mutex_lock(&mutex_exec);");
            pthread_mutex_lock(&mutex_exec);
            int* cpu_id = dictionary_remove(tabla_exec, ejecutando->pid);
            //log_error(logger, "181: pthread_mutex_unlock(&mutex_exec);");
            pthread_mutex_unlock(&mutex_exec);
            
            ejecutando->estimacion_rafaga = estimacion_restante;
            //pthread_mutex_unlock(&ejecutando->mutex_pcb);
            //log_error(logger, "aca se hace un push a cola de ready de PID %d", ejecutando->pid);
            //log_error(logger, "187: pthread_mutex_lock(&mutex_ready);");
            pthread_mutex_lock(&mutex_ready);
            queue_push(cola_ready, ejecutando);
            //log_error(logger, "190: pthread_mutex_unlock(&mutex_ready);");
            pthread_mutex_unlock(&mutex_ready);
            
            //log_warning(logger, "Pusheo cpu %d a cpus libres", *cpu_id);
            //log_error(logger, "194: pthread_mutex_lock(&mutex_cpus_libres);");
            pthread_mutex_lock(&mutex_cpus_libres);
            if(!cpu_esta_en_lista(*cpu_id)){
                list_add(cpus_libres, cpu_id);
            } else{
                free(cpu_id);
            }
            log_debug(logger, "La cola de CPUs libres tiene un tamaño de %d", list_size(cpus_libres));
            //log_error(logger, "197: pthread_mutex_unlock(&mutex_cpus_libres);");
            pthread_mutex_unlock(&mutex_cpus_libres);
            //pushear ejecutando a ready
            //pushear a la cola de cpus libres tambien
            //popear ejecutando de tabla exec
            sem_post(&cpus_disponibles);
            sem_post(&sem_procesos_ready);
        } else {
            log_error(logger, "No se encontró socket de interrupción para CPU %d", cpu_id);
        }
        log_debug(logger,"hasta aca llegue chequear");
        // y hay que replanificar
    }
}


t_pcb* obtener_proceso_en_exec(){
    t_pcb* pcb_en_exec = NULL;
    log_debug(logger,"hasta aca llegue 3");
    void buscar_exec(char* key, void* pcb_void){
        t_pcb* pcb = (t_pcb*)pcb_void;
        if (pcb->estado_actual == EXEC){
            pcb_en_exec = pcb;
        }
    }
   // log_error(logger, "222: pthread_mutex_lock(&mutex_tabla_pcbs);");
    pthread_mutex_lock(&mutex_tabla_pcbs);
    dictionary_iterator(tabla_pcbs, buscar_exec);
   // log_error(logger, "225: pthread_mutex_unlock(&mutex_tabla_pcbs);");
    pthread_mutex_unlock(&mutex_tabla_pcbs);
    return pcb_en_exec;
}

bool hay_proceso_en_exec(){
    bool hay_exec = false;
    
    void buscar_exec(char* key, void* pcb){
        if (((t_pcb*) pcb)->estado_actual == EXEC){
            hay_exec = true;
        }
    }
   // log_error(logger, "229:pthread_mutex_lock(&mutex_tabla_pcbs);");
    pthread_mutex_lock(&mutex_tabla_pcbs);
    dictionary_iterator(tabla_pcbs, buscar_exec);
   // log_error(logger, "232:pthread_mutex_unlock(&mutex_tabla_pcbs);");
    pthread_mutex_unlock(&mutex_tabla_pcbs);
    return hay_exec;
}

void asignar_timer_blocked(t_pcb* pcb){
    //log_error(logger, "238:pthread_mutex_lock(&pcb->mutex_pcb);");
    pthread_mutex_lock(&pcb->mutex_pcb);
    
    if (pcb->temporal_blocked != NULL) {
        temporal_destroy(pcb->temporal_blocked);
        pcb->temporal_blocked = NULL;
    }
    pcb->temporal_blocked = temporal_create();
   // log_error(logger, "246: pthread_mutex_unlock(&pcb->mutex_pcb);");
    pthread_mutex_unlock(&pcb->mutex_pcb);
    sem_post(&sem_procesos_en_blocked);
}

//saca un pcb de la cola de susp_blocked o blocked mediante el pid
t_pcb* sacar_pcb_de_cola(t_queue* cola, int pid){
    t_list* aux = list_create();
    t_pcb* pcb_encontrado = NULL;

    while (!queue_is_empty(cola)){
        t_pcb* actual = queue_pop(cola);

        if (actual->pid == pid){
            pcb_encontrado = actual;
        } else {
            list_add(aux, actual);
        }
    }

    for (int i = 0; i < list_size(aux); i++) {
        queue_push(cola, list_get(aux, i));
    }

    list_destroy(aux);
    return pcb_encontrado;
}

char* parsear_estado(int estado){
    switch (estado) {
        case NEW: return "NEW";
        case READY: return "READY";
        case EXEC: return "EXEC";
        case BLOCKED: return "BLOCKED";
        case SUSP_BLOCKED: return "SUSP_BLOCKED";
        case SUSP_READY: return "SUSP_READY";
        case EXIT: return "EXIT";
        default: return "DESCONOCIDO";
    }
}

void log_metricas_estado(t_pcb* pcb){
    if (pcb == NULL) return;

    log_info(logger, "%d - Métricas de estado:", pcb->pid);
    log_info(logger, "NEW (%d) (%d)", pcb->metricas_estado[NEW], (int)pcb->metricas_tiempo[NEW]);
    log_info(logger, "READY (%d) (%d)", pcb->metricas_estado[READY], (int)pcb->metricas_tiempo[READY]);
    log_info(logger, "EXEC (%d) (%d)", pcb->metricas_estado[EXEC], (int)pcb->metricas_tiempo[EXEC]);
    log_info(logger, "BLOCKED (%d) (%d)", pcb->metricas_estado[BLOCKED], (int)pcb->metricas_tiempo[BLOCKED]);
    log_info(logger, "SUSP_BLOCKED (%d) (%d)", pcb->metricas_estado[SUSP_BLOCKED], (int)pcb->metricas_tiempo[SUSP_BLOCKED]);
    log_info(logger, "SUSP_READY (%d) (%d)", pcb->metricas_estado[SUSP_READY], (int)pcb->metricas_tiempo[SUSP_READY]);
    log_info(logger, "EXIT (%d) (%d)", pcb->metricas_estado[EXIT], (int)pcb->metricas_tiempo[EXIT]);
}