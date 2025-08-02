#include <syscalls.h>


void llamar_a_io(int socket_dispatch) {
    int estado_anterior;
    t_list* campos = recibir_paquete(socket_dispatch);

    int* pid_raw = list_get(campos, 0);
    int* pc_raw = list_get(campos, 1);
    //int* size_disp = list_get(campos, 2);
    char* dispositivo = list_get(campos, 3);
    int* tiempo_raw = list_get(campos, 4);
    int* cpuid_raw = list_get(campos, 5);

    int pid = *(int*)pid_raw;
    int pc = *(int*)pc_raw;
    int tiempo = *(int*)tiempo_raw;
    int cpu_id = *(int*)cpuid_raw;

    log_info(logger, "Recibi syscall IO - PID %d - PC %d - Dispositivo [%s] - Tiempo %d", pid, pc, dispositivo, tiempo);

    //log_error(logger, "22 pthread_mutex_lock(&mutex_dispositivos);");
    pthread_mutex_lock(&mutex_dispositivos);
    t_dispositivo_io* io = dictionary_get(dispositivos_io, dispositivo);
    //log_error(logger, "25 pthread_mutex_unlock(&mutex_dispositivos);");
    pthread_mutex_unlock(&mutex_dispositivos);

    if(io == NULL) {
        log_debug(logger, "Dispositivo IO [%s] no esta conectado. Enviando proceso a EXIT", dispositivo);
        t_pcb* pcb = obtener_pcb(pid);
        if(pcb != NULL){
            estado_anterior = pcb->estado_actual;
            cambiar_estado(pcb, EXIT);
            log_info(logger, "(%d) Pasa del estado %s al estado %s",pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));
            //borrar_pcb(pcb);
        } else{
            log_error(logger, "No se encontro PCB con PID %d al intentar finalizar por IO null", pid);
        }
        
        char* cpu_id_str = string_itoa(cpu_id);
        pthread_mutex_lock(&mutex_interrupt);
        int* socket_interrupt_ptr = dictionary_get(tabla_interrupt, cpu_id_str);
        pthread_mutex_unlock(&mutex_interrupt);

        if(socket_interrupt_ptr != NULL){
            int socket_interrupt = *socket_interrupt_ptr;
            t_paquete* senial_bloqueante = crear_paquete();
            cambiar_opcode_paquete(senial_bloqueante, OC_INTERRUPT);
            enviar_paquete(senial_bloqueante, socket_interrupt, logger);
            borrar_paquete(senial_bloqueante);
            log_debug(logger, "mando interrupcion despues de IO null");
        }
        free(cpu_id_str);

        // t_paquete* confirmacion = crear_paquete();
        // cambiar_opcode_paquete(confirmacion, OK);
        // enviar_paquete(confirmacion, socket_dispatch, logger);
        // borrar_paquete(confirmacion);

        list_destroy_and_destroy_elements(campos, free);
        //free(dispositivo);
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
    //log_error(logger,"54: pthread_mutex_lock(&mutex_blocked);");
    pthread_mutex_lock(&mutex_blocked);
    
    queue_push(cola_blocked, pcb);
    //log_error(logger, "58: pthread_mutex_unlock(&mutex_blocked);");
    pthread_mutex_unlock(&mutex_blocked);

    char* cpu_id_str = string_itoa(cpu_id);
    //log_error(logger, "62: pthread_mutex_lock(&mutex_interrupt);");
    pthread_mutex_lock(&mutex_interrupt);
    int* socket_interrupt_ptr = dictionary_get(tabla_interrupt, cpu_id_str);
    //log_error(logger, "65: pthread_mutex_unlock(&mutex_interrupt);");
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
    

    t_instancia_io* instancia = obtener_instancia_disponible(io);


    if(instancia == NULL) {
        log_info(logger, "Dispositivo ocupado, mando PID: %d a cola bloqueados", pid);
        t_pcb_io* bloqueado = malloc(sizeof(t_pcb_io));
        bloqueado->pid = pid;
        bloqueado->tiempo = tiempo;
        queue_push(io->cola_bloqueados, bloqueado);
    } else {
        //log_error(logger, "97: pthread_mutex_lock(&io->mutex_dispositivos);");
        pthread_mutex_lock(&io->mutex_dispositivos);
        instancia->pid_ocupado = pid;
        //log_error(logger, "101: pthread_mutex_unlock(&io->mutex_dispositivos);");
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

    //log_warning(logger, "Pusheo cpu %d a cpus libres", cpu_id);
    //log_error(logger, "119: pthread_mutex_lock(&mutex_cpus_libres);");
    pthread_mutex_lock(&mutex_cpus_libres);
    if(!cpu_esta_en_lista(*cpu_id_ptr)){
        list_add(cpus_libres, cpu_id_ptr);
    } else{
        //free(cpu_id_ptr);
    }
    log_debug(logger, "La cola de CPUs libres tiene un tamaño de %d", list_size(cpus_libres));
   // log_error(logger, "122: pthread_mutex_unlock(&mutex_cpus_libres);");
    pthread_mutex_unlock(&mutex_cpus_libres);

    sem_post(&cpus_disponibles);
   // log_error(logger, "126: pthread_mutex_lock(&mutex_ready);");
    pthread_mutex_lock(&mutex_ready);
    if(!queue_is_empty(cola_ready)){
        sem_post(&sem_procesos_ready);
    }
   // log_error(logger, "131: pthread_mutex_unlock(&mutex_ready);");
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
        free(nombre_dispositivo);
        return;
    }

    int estado_anterior = pcb->estado_actual;

    //log_error(logger, "158: pthread_mutex_lock(&pcb->mutex_pcb);");
    pthread_mutex_lock(&pcb->mutex_pcb);
    if (pcb->estado_actual == SUSP_BLOCKED){

        //log_error(logger, "162: pthread_mutex_lock(&mutex_susp_blocked);");
        pthread_mutex_lock(&mutex_susp_blocked);
        sacar_pcb_de_cola(cola_susp_blocked, pcb->pid);
        //log_error(logger, "165: pthread_mutex_unlock(&mutex_susp_blocked);");
        pthread_mutex_unlock(&mutex_susp_blocked);
        cambiar_estado_sin_lock(pcb, SUSP_READY);
        log_info(logger, "(%d) Pasa del estado %s al estado %s",pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));

        //log_error(logger, "171: pthread_mutex_lock(&mutex_susp_ready);");
        pthread_mutex_lock(&mutex_susp_ready);
        queue_push(cola_susp_ready, pcb);
        //log_error(logger, "174: pthread_mutex_unlock(&mutex_susp_ready);");
        pthread_mutex_unlock(&mutex_susp_ready);
        sem_post(&sem_plp);

    } else if (pcb->estado_actual == BLOCKED){
        //log_error(logger,"180: pthread_mutex_lock(&mutex_blocked);");
        pthread_mutex_lock(&mutex_blocked);
        sacar_pcb_de_cola(cola_blocked, pcb->pid);
        cambiar_estado_sin_lock(pcb, READY);
        //log_error(logger,"187: pthread_mutex_unlock(&mutex_blocked);");
        pthread_mutex_unlock(&mutex_blocked);

        
        log_info(logger, "(%d) Pasa del estado %s al estado %s",pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));

        //log_error(logger, "aca se hace un push a cola de ready de PID %d", pcb->pid);
        //log_error(logger,"194: pthread_mutex_lock(&mutex_ready);");
        pthread_mutex_lock(&mutex_ready);
        queue_push(cola_ready, pcb);
        //log_error(logger,"197: pthread_mutex_unlock(&mutex_ready);");
        pthread_mutex_unlock(&mutex_ready);
        log_info(logger, "## (%d) finalizó IO y pasa a READY", pcb->pid);
        sem_post(&sem_procesos_ready);

    }
    pthread_mutex_lock(&mutex_dispositivos);
    t_dispositivo_io* io = dictionary_get(dispositivos_io, nombre_dispositivo);
    if(io == NULL){
        pthread_mutex_unlock(&mutex_dispositivos);
        return;
    }
    pthread_mutex_lock(&io->mutex_dispositivos);
    pthread_mutex_unlock(&mutex_dispositivos);
    t_instancia_io* instancia = NULL;
    

    
    for(int i = 0; i < list_size(io->sockets_io); i++){
        t_instancia_io* actual = list_get(io->sockets_io, i);
        if(actual->socket == socket_io){
            instancia = actual;
            break;
        }
    }
    pthread_mutex_unlock(&io->mutex_dispositivos);

    if(instancia != NULL){
        //log_error(logger, "216: pthread_mutex_lock(&io->mutex_dispositivos);");
        pthread_mutex_lock(&io->mutex_dispositivos);
        instancia->ocupado = false;
        instancia->pid_ocupado = -1;
        //log_error(logger, "220: pthread_mutex_unlock(&io->mutex_dispositivos);");
        pthread_mutex_unlock(&io->mutex_dispositivos);
    }

    if (!queue_is_empty(io->cola_bloqueados)) {
        t_instancia_io* libre = obtener_instancia_disponible(io);
        if(libre == NULL){
            log_error(logger, "No quedan instancias del dispositivo [%s]", nombre_dispositivo);
            log_debug(logger, "Este caso no se deberia dar");//no se contempla pasar procesos a exit
            //free(siguiente);
            list_destroy_and_destroy_elements(recibido, free);
            return;
        }


        t_pcb_io* siguiente = queue_pop(io->cola_bloqueados);
        // Mandar a IO
        t_paquete* paquete = crear_paquete();
        cambiar_opcode_paquete(paquete, SOLICITUD_IO);
        agregar_a_paquete(paquete, &(siguiente->pid), sizeof(int));
        agregar_a_paquete(paquete, &(siguiente->tiempo), sizeof(int));
        agregar_a_paquete(paquete, &cpu_id, sizeof(int));
        agregar_a_paquete(paquete, nombre_dispositivo, strlen(nombre_dispositivo) + 1);
        enviar_paquete(paquete, libre->socket, logger);
        borrar_paquete(paquete);

      //  log_error(logger, "247: pthread_mutex_lock(&io->mutex_dispositivos);");
        pthread_mutex_lock(&io->mutex_dispositivos);
        libre->ocupado = true;
        libre->pid_ocupado = siguiente->pid;
      //  log_error(logger, "251: pthread_mutex_unlock(&io->mutex_dispositivos);");
        pthread_mutex_unlock(&io->mutex_dispositivos);

        int* cpu_id_ptr_copia = malloc(sizeof(int));
        *cpu_id_ptr_copia = cpu_id;

        //log_warning(logger, "Pusheo cpu %d a cpus libres", cpu_id);
      //  log_error(logger, "258: pthread_mutex_lock(&mutex_cpus_libres);");
        pthread_mutex_lock(&mutex_cpus_libres);
        if(!cpu_esta_en_lista(*cpu_id_ptr_copia)){
            list_add(cpus_libres, cpu_id_ptr_copia);
        }else{
            free(cpu_id_ptr_copia);
        }
        log_debug(logger, "La cola de CPUs libres tiene un tamaño de %d", list_size(cpus_libres));
       // log_error(logger, "261: pthread_mutex_unlock(&mutex_cpus_libres);");
        pthread_mutex_unlock(&mutex_cpus_libres);
        sem_post(&cpus_disponibles);

        log_trace(logger, "Despierto proceso PID %d para usar dispositivo %s", siguiente->pid, nombre_dispositivo);

        free(siguiente);
    }

    
    if(pcb->temporal_blocked != NULL){
        temporal_destroy(pcb->temporal_blocked);
        pcb->temporal_blocked = NULL;
    }
   // log_error(logger, "277: pthread_mutex_unlock(&pcb->mutex_pcb);");
    pthread_mutex_unlock(&pcb->mutex_pcb);
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
       // log_error(logger,"300: pthread_mutex_lock(&mutex_blocked);");
        pthread_mutex_lock(&mutex_blocked);
        
        sacar_pcb_de_cola(cola_blocked, pid);
       // log_error(logger,"304: pthread_mutex_unlock(&mutex_blocked);");
        pthread_mutex_unlock(&mutex_blocked);

        //log_error(logger, "aca se hace un push a cola de ready de PID %d", pcb->pid);
      //  log_error(logger,"308: pthread_mutex_lock(&mutex_ready);");
        pthread_mutex_lock(&mutex_ready);
        queue_push(cola_ready, pcb);
      //  log_error(logger,"311: pthread_mutex_unlock(&mutex_ready);");
        pthread_mutex_unlock(&mutex_ready);

        sem_post(&sem_procesos_ready);
    } else{
        log_error(logger, "No se logró llevar a cabo el MEMORY DUMP");
        estado_anterior = pcb->estado_actual;
        cambiar_estado(pcb, EXIT);
        log_info(logger, "(%d) Pasa del estado %s al estado %s", pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));
        borrar_pcb(pcb);
    }
   // log_error(logger, "322: pthread_mutex_lock(&pcb->mutex_pcb);");
    pthread_mutex_lock(&pcb->mutex_pcb);
    if(pcb->temporal_blocked != NULL){
        temporal_destroy(pcb->temporal_blocked);
        pcb->temporal_blocked = NULL;
    }
   // log_error(logger, "328: pthread_mutex_unlock(&pcb->mutex_pcb);");
    pthread_mutex_unlock(&pcb->mutex_pcb);
    cerrar_conexion_memoria(socket_memoria);
    log_debug(logger, "Se recibio confirmacion de memory dump");
    return NULL;
}

void dump_memory(int socket_dispatch){
    int estado_anterior;

    t_list* recibido = recibir_paquete(socket_dispatch);
    int* pid_raw = list_get(recibido, 0);
    int* pc_raw = list_get(recibido, 1);
    int* cpu_id_raw = list_get(recibido, 2);

    int pid = *pid_raw;
    int pc = *pc_raw;
    int cpu_id = *cpu_id_raw;
    log_trace(logger, "## DUMP MEMORY - PID %d - PC %d - CPU_ID %d", pid, pc, cpu_id);
   // log_error(logger,"344: pthread_mutex_lock(&mutex_blocked);");
    pthread_mutex_lock(&mutex_blocked);
    
    t_pcb* pcb = obtener_pcb(pid);
    //pc++;
    pcb->pc = pc;

    actualizar_estimacion_rafaga(pcb);
    estado_anterior = pcb->estado_actual;
    cambiar_estado(pcb, BLOCKED);
    log_info(logger, "(%d) Pasa del estado %s al estado %s", pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));
    
    asignar_timer_blocked(pcb);
    queue_push(cola_blocked, pcb);
    pthread_mutex_unlock(&mutex_blocked);

    char* cpu_id_str = string_itoa(cpu_id);
   // log_error(logger, "361: pthread_mutex_lock(&mutex_interrupt);");
    pthread_mutex_lock(&mutex_interrupt);
    int* socket_interrupt_ptr = dictionary_get(tabla_interrupt, cpu_id_str);
   // log_error(logger, "364: pthread_mutex_unlock(&mutex_interrupt);");
    pthread_mutex_unlock(&mutex_interrupt);
    socket_interrupt = *socket_interrupt_ptr;
    free(cpu_id_str);

    log_debug(logger, "Mando una interrupcion desde dump memory");
    t_paquete* senial_bloqueante = crear_paquete();
    cambiar_opcode_paquete(senial_bloqueante, OC_INTERRUPT);
    enviar_paquete(senial_bloqueante, socket_interrupt, logger);
    borrar_paquete(senial_bloqueante);

    // t_paquete* confirmacion = crear_paquete();
    // cambiar_opcode_paquete(confirmacion, OK);
    // enviar_paquete(confirmacion, socket_dispatch, logger);
    // borrar_paquete(confirmacion);

    int* nuevo_cpu_id = malloc(sizeof(int));
    *nuevo_cpu_id = cpu_id;

    //log_warning(logger, "Pusheo cpu %d a cpus libres", cpu_id);
    pthread_mutex_lock(&mutex_cpus_libres);
    if(!cpu_esta_en_lista(*nuevo_cpu_id)){
        list_add(cpus_libres, nuevo_cpu_id);
    } else{
        //free(nuevo_cpu_id);
    }
    log_debug(logger, "La cola de CPUs libres tiene un tamaño de %d", list_size(cpus_libres));
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

    list_destroy_and_destroy_elements(recibido, free);
}



void iniciar_proceso(int socket_cpu){
    t_list* recibido = recibir_paquete(socket_cpu);
    //int* pid_anterior_raw = list_get(recibido, 0);
    int* tamanio_proceso_raw = list_get(recibido, 1);
    char* nombre_archivo = list_get(recibido, 2);

    //int pid_anterior = *pid_anterior_raw;
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

    pthread_mutex_lock(&mutex_ready);
    if (!queue_is_empty(cola_ready)) {
        sem_post(&sem_procesos_ready);
    }
    pthread_mutex_unlock(&mutex_ready);
    return;
}