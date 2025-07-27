#include <syscalls.h>


void llamar_a_io(int socket_dispatch) {
    int estado_anterior;
    t_list* campos = recibir_paquete(socket_dispatch);

    int* pid_raw = list_get(campos, 0);
    int* pc_raw = list_get(campos, 1);
    int* size_disp = list_get(campos, 2);
    char* dispositivo = list_get(campos, 3);
    int* tiempo_raw = list_get(campos, 4);
    int* cpuid_raw = list_get(campos, 5);

    int pid = *(int*)pid_raw;
    int pc = *(int*)pc_raw;
    int tiempo = *(int*)tiempo_raw;
    int cpu_id = *(int*)cpuid_raw;

    log_info(logger, "Recibi syscall IO - PID %d - PC %d - Dispositivo [%s] - Tiempo %d", pid, pc, dispositivo, tiempo);

    t_dispositivo_io* io = dictionary_get(dispositivos_io, dispositivo);

    if(io == NULL) {
        log_error(logger, "dispositivo IO [%s] no esta conectado. Enviando proceso a EXIT", dispositivo);
        t_pcb* pcb = obtener_pcb(pid);
        estado_anterior = pcb->estado_actual;
        cambiar_estado(pcb, EXIT);
        log_info(logger, "(%d) Pasa del estado %s al estado %s",pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));

        borrar_pcb(pcb);
        list_destroy_and_destroy_elements(campos, free);
        free(dispositivo);
        return;
    }

    
    t_pcb* pcb = obtener_pcb(pid);
    pcb->pc = pc;

    temporal_stop(pcb->temporal_estado);
    actualizar_estimacion_rafaga(pcb);
    estado_anterior = pcb->estado_actual;
    cambiar_estado(pcb, BLOCKED);
    log_info(logger, "(%d) Pasa del estado %s al estado %s",pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));

    asignar_timer_blocked(pcb);

    log_debug(logger, "Se bloquea el proceso PID %d", pcb->pid);
    pthread_mutex_lock(&mutex_blocked);
    queue_push(cola_blocked, pcb);
    pthread_mutex_unlock(&mutex_blocked);

    char* cpu_id_str = string_itoa(cpu_id);
    pthread_mutex_lock(&mutex_interrupt);
    int* socket_interrupt_ptr = dictionary_get(tabla_interrupt, cpu_id_str);
    pthread_mutex_unlock(&mutex_interrupt);
    if(!socket_interrupt_ptr){
        log_error(logger, "No se encontro el socket_interrupt para CPU ID %d", cpu_id);
        list_destroy_and_destroy_elements(campos, free);
        free(cpu_id_str);
        return;
    }
    socket_interrupt = *socket_interrupt_ptr;
    free(cpu_id_str);

    log_debug(logger, "Mando una interrupcion desde IO");
    t_paquete* senial_bloqueante = crear_paquete();
    cambiar_opcode_paquete(senial_bloqueante, OC_INTERRUPT);
    enviar_paquete(senial_bloqueante, socket_interrupt, logger);
    borrar_paquete(senial_bloqueante);
    
    t_paquete* confirmacion = crear_paquete();
    cambiar_opcode_paquete(confirmacion, OK);
    enviar_paquete(confirmacion, socket_dispatch, logger);
    borrar_paquete(confirmacion);

    t_instancia_io* instancia = obtener_instancia_disponible(io);


    if(instancia == NULL) {
        log_info(logger, "Dispositivo ocupado, mando PID: %d a cola bloqueados", pid);
        t_pcb_io* bloqueado = malloc(sizeof(t_pcb_io));
        bloqueado->pid = pid;
        bloqueado->tiempo = tiempo;
        queue_push(io->cola_bloqueados, bloqueado);
    } else {

        pthread_mutex_lock(&io->mutex_dispositivos);
        instancia->ocupado = true;
        instancia->pid_ocupado = pid;
        pthread_mutex_unlock(&io->mutex_dispositivos);

        t_paquete* paquete = crear_paquete();
        cambiar_opcode_paquete(paquete, SOLICITUD_IO);
        agregar_a_paquete(paquete, &pid, sizeof(int));
        agregar_a_paquete(paquete, &tiempo, sizeof(int));
        agregar_a_paquete(paquete, &cpu_id, sizeof(int));
        agregar_a_paquete(paquete, dispositivo, strlen(dispositivo) + 1);
        enviar_paquete(paquete, instancia->socket, logger);
        borrar_paquete(paquete);
        log_trace(logger, "Proceso PID %d enviado a IO por socket %d", pid, instancia->socket);
    }

    int* cpu_id_ptr = malloc(sizeof(int));
    *cpu_id_ptr = cpu_id;

    log_warning(logger, "Pusheo cpu %d a cpus libres", cpu_id);
    pthread_mutex_lock(&mutex_cpus_libres);
    queue_push(cpus_libres, cpu_id_ptr);
    pthread_mutex_unlock(&mutex_cpus_libres);

    sem_post(&cpus_disponibles);
    pthread_mutex_lock(&mutex_ready);
    if(!queue_is_empty(cola_ready)){
        sem_post(&sem_procesos_ready);
    }
    pthread_mutex_unlock(&mutex_ready);

    list_destroy_and_destroy_elements(campos, free);

    
}

void manejar_finaliza_io(int socket_io){
    t_list* recibido = recibir_paquete(socket_io);
    int* pid = list_get(recibido, 0);
    char* nombre_dispositivo_raw = list_get(recibido, 1);
    int* cpu_id_ptr = list_get(recibido, 2);
    int cpu_id = *cpu_id_ptr;
    char* nombre_dispositivo = string_duplicate(nombre_dispositivo_raw);

    log_trace(logger, "Recibi finalizacion de io - pid %d - dispositivo %s - cpuid %d", *pid, nombre_dispositivo, cpu_id);
    t_pcb* pcb = obtener_pcb(*pid);
    
    if (pcb == NULL){
        log_error(logger, "FINALIZA_IO: No se encontró el PCB del PID %d", *pid);
        list_destroy_and_destroy_elements(recibido, free);
        return;
    }

    int estado_anterior = pcb->estado_actual;

    if (pcb->estado_actual == SUSP_BLOCKED){

        pthread_mutex_lock(&mutex_susp_blocked);
        sacar_pcb_de_cola(cola_susp_blocked, pcb->pid);
        pthread_mutex_unlock(&mutex_susp_blocked);

        cambiar_estado(pcb, SUSP_READY);
        log_info(logger, "(%d) Pasa del estado %s al estado %s",pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));

        pthread_mutex_lock(&mutex_susp_ready);
        queue_push(cola_susp_ready, pcb);
        pthread_mutex_unlock(&mutex_susp_ready);

    } else if (pcb->estado_actual == BLOCKED){
        pthread_mutex_lock(&mutex_blocked);
        sacar_pcb_de_cola(cola_blocked, pcb->pid);
        pthread_mutex_unlock(&mutex_blocked);

        cambiar_estado(pcb, READY);
        log_info(logger, "(%d) Pasa del estado %s al estado %s",pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));

        pthread_mutex_lock(&mutex_ready);
        queue_push(cola_ready, pcb);
        pthread_mutex_unlock(&mutex_ready);
        log_info(logger, "## (%d) finalizó IO y pasa a READY", pcb->pid);
        sem_post(&sem_procesos_ready);

    }

    t_dispositivo_io* io = dictionary_get(dispositivos_io, nombre_dispositivo);
    t_instancia_io* instancia = NULL;

    for(int i = 0; i < list_size(io->sockets_io); i++){
        t_instancia_io* actual = list_get(io->sockets_io, i);
        if(actual->socket == socket_io){
            instancia = actual;
            break;
        }
    }

    if(instancia != NULL){
        pthread_mutex_lock(&io->mutex_dispositivos);
        instancia->ocupado = false;
        instancia->pid_ocupado = -1;
        pthread_mutex_unlock(&io->mutex_dispositivos);
    }

    t_instancia_io* libre = obtener_instancia_disponible(io);

    if (!queue_is_empty(io->cola_bloqueados)) {
        t_pcb_io* siguiente = queue_pop(io->cola_bloqueados);

        if(libre == NULL){
            log_error(logger, "No quedan instancias del dispositivo [%s]", nombre_dispositivo);
            log_debug(logger, "Este caso no se deberia dar");//no se contempla pasar procesos a exit
            free(siguiente);
            list_destroy_and_destroy_elements(recibido, free);
            return;
        }

        // Mandar a IO
        t_paquete* paquete = crear_paquete();
        cambiar_opcode_paquete(paquete, SOLICITUD_IO);
        agregar_a_paquete(paquete, &(siguiente->pid), sizeof(int));
        agregar_a_paquete(paquete, &(siguiente->tiempo), sizeof(int));
        agregar_a_paquete(paquete, &cpu_id, sizeof(int));
        agregar_a_paquete(paquete, nombre_dispositivo, strlen(nombre_dispositivo) + 1);
        enviar_paquete(paquete, libre->socket, logger);
        borrar_paquete(paquete);

        pthread_mutex_lock(&io->mutex_dispositivos);
        libre->ocupado = true;
        libre->pid_ocupado = siguiente->pid;
        pthread_mutex_unlock(&io->mutex_dispositivos);

        int* cpu_id_ptr_copia = malloc(sizeof(int));
        *cpu_id_ptr_copia = cpu_id;

        log_warning(logger, "Pusheo cpu %d a cpus libres", cpu_id);
        pthread_mutex_lock(&mutex_cpus_libres);
        queue_push(cpus_libres, cpu_id_ptr_copia);
        pthread_mutex_unlock(&mutex_cpus_libres);
        sem_post(&cpus_disponibles);

        log_trace(logger, "Despierto proceso PID %d para usar dispositivo %s", siguiente->pid, nombre_dispositivo);

        free(siguiente);
    }

    if(pcb->temporal_blocked != NULL){
        if(estado_anterior == BLOCKED){
            temporal_destroy(pcb->temporal_blocked);
            pcb->temporal_blocked = NULL;
        }
    }
    free(nombre_dispositivo);
    list_destroy_and_destroy_elements(recibido, free);
    //list_destroy_and_destroy_elements(recibido, free); esto estaba liberando cosas que iban a un diccionario creo
}

void* esperar_confirmacion_dump(void* args_void){
    t_args_dump* args = (t_args_dump*) args_void;
    int pid = args->pid;
    int socket_memoria = args->socket_memoria;
    free(args);

    int respuesta = recibir_operacion(socket_memoria);
    int estado_anterior;

    t_pcb* pcb = obtener_pcb(pid);

    if (respuesta == MEMORY_DUMP){
        estado_anterior = pcb->estado_actual;
        cambiar_estado(pcb, READY);
        log_debug(logger, "El dump memory se llevo a cabo correctamente");
        log_info(logger, "(%d) Pasa del estado %s al estado %s", pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));
        
        pthread_mutex_lock(&mutex_blocked);
        sacar_pcb_de_cola(cola_blocked, pid);
        pthread_mutex_unlock(&mutex_blocked);

        pthread_mutex_lock(&mutex_ready);
        queue_push(cola_ready, pcb);
        pthread_mutex_unlock(&mutex_ready);

        sem_post(&sem_procesos_ready);
    } else{
        log_error(logger, "No se logró llevar a cabo el MEMORY DUMP");
        estado_anterior = pcb->estado_actual;
        cambiar_estado(pcb, EXIT);
        log_info(logger, "(%d) Pasa del estado %s al estado %s", pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));
        borrar_pcb(pcb);
    }
    if(pcb->temporal_blocked != NULL){
        temporal_destroy(pcb->temporal_blocked);
        pcb->temporal_blocked = NULL;
    }
    cerrar_conexion_memoria(socket_memoria);
    log_debug(logger, "Se recibio confirmacion de memory dump");
    return NULL;
}

void dump_memory(int socket_dispatch){
    int estado_anterior;

    t_list* recibido = recibir_paquete(socket_dispatch);
    int pid = *((int*)list_get(recibido, 0));
    int pc = *((int*)list_get(recibido, 1));
    int cpu_id = *((int*)list_get(recibido, 2));

    log_trace(logger, "## DUMP MEMORY - PID %d - PC %d - CPU_ID %d", pid, pc, cpu_id);

    pthread_mutex_lock(&mutex_blocked);
    t_pcb* pcb = obtener_pcb(pid);
    pc++;
    pcb->pc = pc;

    actualizar_estimacion_rafaga(pcb);
    estado_anterior = pcb->estado_actual;
    cambiar_estado(pcb, BLOCKED);
    log_info(logger, "(%d) Pasa del estado %s al estado %s", pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));
    
    asignar_timer_blocked(pcb);
    queue_push(cola_blocked, pcb);
    pthread_mutex_unlock(&mutex_blocked);

    char* cpu_id_str = string_itoa(cpu_id);
    pthread_mutex_lock(&mutex_interrupt);
    int* socket_interrupt_ptr = dictionary_get(tabla_interrupt, cpu_id_str);
    pthread_mutex_unlock(&mutex_interrupt);
    socket_interrupt = *socket_interrupt_ptr;
    free(cpu_id_str);

    log_debug(logger, "Mando una interrupcion desde dump memory");
    t_paquete* senial_bloqueante = crear_paquete();
    cambiar_opcode_paquete(senial_bloqueante, OC_INTERRUPT);
    enviar_paquete(senial_bloqueante, socket_interrupt, logger);
    borrar_paquete(senial_bloqueante);

    t_paquete* confirmacion = crear_paquete();
    cambiar_opcode_paquete(confirmacion, OK);
    enviar_paquete(confirmacion, socket_dispatch, logger);
    borrar_paquete(confirmacion);

    int* nuevo_cpu_id = malloc(sizeof(int));
    *nuevo_cpu_id = cpu_id;

    log_warning(logger, "Pusheo cpu %d a cpus libres", cpu_id);
    pthread_mutex_lock(&mutex_cpus_libres);
    queue_push(cpus_libres, nuevo_cpu_id);
    pthread_mutex_unlock(&mutex_cpus_libres);
    sem_post(&cpus_disponibles);

    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, SOLICITUD_DUMP_MEMORY);
    agregar_a_paquete(paquete, &pid, sizeof(int));
    socket_memoria = operacion_con_memoria();
    enviar_paquete(paquete, socket_memoria, logger);
    borrar_paquete(paquete);

    //cerrar_conexion_memoria(socket_memoria);
    //esto no va a ser necesario ya que lo va a cerrar cuando reciba el dumpeo

    t_args_dump* args = malloc(sizeof(t_args_dump));
    args->socket_memoria = socket_memoria;
    args->pid = pid;

    pthread_t hilo_confirmacion_dump;
    pthread_create(&hilo_confirmacion_dump, NULL, esperar_confirmacion_dump, (void*) args);
    pthread_detach(hilo_confirmacion_dump);

}



void iniciar_proceso(int socket_cpu){
    t_list* recibido = recibir_paquete(socket_cpu);
    int* pid_anterior_raw = list_get(recibido, 0);
    int* tamanio_proceso_raw = list_get(recibido, 1);
    char* nombre_archivo = list_get(recibido, 2);

    int pid_anterior = *pid_anterior_raw;
    int tamanio_proceso = *tamanio_proceso_raw;
    int pid = crear_proceso(tamanio_proceso);


    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OC_INIT);
    agregar_a_paquete(paquete, &pid, sizeof(int));
    agregar_a_paquete(paquete, &tamanio_proceso, sizeof(int));
    agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo)+1);
    socket_memoria = operacion_con_memoria();
    enviar_paquete(paquete, socket_memoria, logger);
    cerrar_conexion_memoria(socket_memoria);
    borrar_paquete(paquete);
    log_debug(logger, "Se va a iniciar el proceso (%s), tamanio [%d]", nombre_archivo, tamanio_proceso);

    list_destroy_and_destroy_elements(recibido, free);    
}

void enviar_finalizacion_a_memoria(int pid){
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, KILL_PROC);
    agregar_a_paquete(paquete, &pid, sizeof(int));
    socket_memoria = operacion_con_memoria();
    enviar_paquete(paquete, socket_memoria, logger);
    borrar_paquete(paquete);
    if(recibir_operacion(socket_memoria) == OK){
        log_info(logger, "## (%d) - Finaliza el proceso", pid);
    }
    cerrar_conexion_memoria(socket_memoria);
    
}

void ejecutar_exit(int socket_cpu){
    t_list* recibido = recibir_paquete(socket_cpu);
    int* pid_raw = list_get(recibido, 0);
    int pid = *pid_raw;

    t_pcb* pcb = obtener_pcb(pid);
    actualizar_estimacion_rafaga(pcb);
    finalizar_proceso(pcb);
    list_destroy_and_destroy_elements(recibido, free);
}