#include <syscalls.h>


void llamar_a_io(int socket_dispatch) {
    int estado_anterior;
    t_list* campos = recibir_paquete(socket_dispatch);

    char* pid_raw = list_get(campos, 0);
    char* pc_raw = list_get(campos, 1);
    int size_disp = *((int*)list_get(campos, 2));
    char* dispositivo = list_get(campos, 3);
    char* tiempo_raw = list_get(campos, 4);
    char* cpuid_raw = list_get(campos, 5);

    int pid = *(int*)pid_raw;
    int pc = *(int*)pc_raw;
    int tiempo = *(int*)tiempo_raw;
    int cpu_id = *(int*)cpuid_raw;

    log_info(logger, "recibi syscall IO - PID %d - PC %d - Dispositivo [%s] - Tiempo %d", pid, pc, dispositivo, tiempo);

    t_dispositivo_io* io = dictionary_get(dispositivos_io, dispositivo);

    if(io == NULL) {
        log_error(logger, "dispositivo IO [%s] no existe. Enviando proceso a EXIT", dispositivo);
        t_pcb* pcb = obtener_pcb(pid);
        estado_anterior = pcb->estado_actual;
        cambiar_estado(pcb, EXIT);
        log_info(logger, "(%d) Pasa del estado %s al estado %s",pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));

        borrar_pcb(pcb);
        list_destroy_and_destroy_elements(campos, free);
        return;
    }

    pthread_mutex_lock(&mutex_blocked);
    t_pcb* pcb = obtener_pcb(pid);
    pcb->pc = pc;

    temporal_stop(pcb->temporal_estado);
    actualizar_estimacion_rafaga(pcb);
    estado_anterior = pcb->estado_actual;
    cambiar_estado(pcb, BLOCKED);
    log_info(logger, "(%d) Pasa del estado %s al estado %s",pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));

    asignar_timer_blocked(pcb);
    queue_push(cola_blocked, pcb);
    pthread_mutex_unlock(&mutex_blocked);
    char* cpu_id_str = string_itoa(cpu_id);
    int* socket_interrupt_ptr = dictionary_get(tabla_interrupt, cpu_id_str);
    socket_interrupt = *socket_interrupt_ptr;
    free(cpu_id_str);

    t_paquete* senial_bloqueante = crear_paquete();
    cambiar_opcode_paquete(senial_bloqueante, OC_INTERRUPT);
    enviar_paquete(senial_bloqueante, socket_interrupt, logger);
    borrar_paquete(senial_bloqueante);
    
    t_paquete* confirmacion = crear_paquete();
    cambiar_opcode_paquete(confirmacion, OK);
    enviar_paquete(confirmacion, socket_dispatch, logger);
    borrar_paquete(confirmacion);

    if(io->ocupado) {
        log_info(logger, "Dispositivo ocupado, mando PID: %d a cola bloqueados", pid);
        t_pcb_io* bloqueado = malloc(sizeof(t_pcb_io));
        bloqueado->pid = pid;
        bloqueado->tiempo = tiempo;
        queue_push(io->cola_bloqueados, bloqueado);
    } else {
        io->ocupado = true;
        log_error(logger, "size disp = %d", size_disp);
        t_paquete* paquete = crear_paquete();
        cambiar_opcode_paquete(paquete, SOLICITUD_IO);
        agregar_a_paquete(paquete, &pid, sizeof(int));
        agregar_a_paquete(paquete, &tiempo, sizeof(int));
        agregar_a_paquete(paquete, &cpu_id, sizeof(int));
        agregar_a_paquete(paquete, dispositivo, size_disp);
        enviar_paquete(paquete, io->socket_io, logger);
        borrar_paquete(paquete);
        log_trace(logger, "Dispositivo PID %d enviado a IO", pid);
    }

    int* cpu_id_ptr = malloc(sizeof(int));
    *cpu_id_ptr = cpu_id;
    log_warning(logger, "2- cpu aniadida %d", *cpu_id_ptr);
    pthread_mutex_lock(&mutex_cpus_libres);
    queue_push(cpus_libres, cpu_id_ptr);
    pthread_mutex_unlock(&mutex_cpus_libres);
    sem_post(&cpus_disponibles);

    list_destroy_and_destroy_elements(campos, free);
}

void manejar_finaliza_io(int socket_io){
    t_list* recibido = recibir_paquete(socket_io);
    int* pid = list_get(recibido, 0);
    char* nombre_dispositivo = list_get(recibido, 1);
    int cpu_id = *((int*)list_get(recibido, 2));

    log_trace(logger, "Recibi finalizacion de io - pid %d - dispositivo %s - cpuid %d", *pid, nombre_dispositivo, cpu_id);
    t_pcb* pcb = obtener_pcb(*pid);
    int estado_anterior;

    if (pcb == NULL){
        log_error(logger, "FINALIZA_IO: No se encontró el PCB del PID %d", *pid);
        list_destroy_and_destroy_elements(recibido, free);
        return;
    }

    if (pcb->estado_actual == SUSP_BLOCKED){

        pthread_mutex_lock(&mutex_susp_blocked);
        sacar_pcb_de_cola(cola_susp_blocked, pcb->pid);
        pthread_mutex_unlock(&mutex_susp_blocked);

        estado_anterior = pcb->estado_actual;
        cambiar_estado(pcb, SUSP_READY);
        log_info(logger, "(%d) Pasa del estado %s al estado %s",pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));


        pthread_mutex_lock(&mutex_susp_ready);
        queue_push(cola_susp_ready, pcb);
        pthread_mutex_unlock(&mutex_susp_ready);

    } else if (pcb->estado_actual == BLOCKED){
        estado_anterior = pcb->estado_actual;
        cambiar_estado(pcb, READY);
        log_info(logger, "(%d) Pasa del estado %s al estado %s",pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));

        queue_push(cola_ready, pcb);
        log_info(logger, "%d Finalizo IO y pasa a READY", pcb->pid);
        sem_post(&sem_procesos_ready);

    }

    t_dispositivo_io* io = dictionary_get(dispositivos_io, nombre_dispositivo);
    io->ocupado = false;

    if (!queue_is_empty(io->cola_bloqueados)) {
        t_pcb_io* siguiente = queue_pop(io->cola_bloqueados);

        // Mandar a IO
        t_paquete* paquete = crear_paquete();
        cambiar_opcode_paquete(paquete, SOLICITUD_IO);
        agregar_a_paquete(paquete, &(siguiente->pid), sizeof(int));
        agregar_a_paquete(paquete, &(siguiente->tiempo), sizeof(int));
        agregar_a_paquete(paquete, &cpu_id, sizeof(int));
        agregar_a_paquete(paquete, nombre_dispositivo, strlen(nombre_dispositivo) + 1);
        enviar_paquete(paquete, io->socket_io, logger);
        borrar_paquete(paquete);

        io->ocupado = true;

        int* cpu_id_ptr = malloc(sizeof(int));
        *cpu_id_ptr = cpu_id;

        log_warning(logger, "1- cpu aniadida %d", *cpu_id_ptr);
        pthread_mutex_lock(&mutex_cpus_libres);
        queue_push(cpus_libres, cpu_id_ptr);
        pthread_mutex_unlock(&mutex_cpus_libres);
        sem_post(&cpus_disponibles);

        log_trace(logger, "Despierto proceso PID %d para usar dispositivo %s", siguiente->pid, nombre_dispositivo);

        //free(cpu_id_ptr);
        free(siguiente);
    }

    list_destroy_and_destroy_elements(recibido, free);

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
    pcb->pc = pc;

    actualizar_estimacion_rafaga(pcb);
    estado_anterior = pcb->estado_actual;
    cambiar_estado(pcb, BLOCKED);
    log_info(logger, "(%d) Pasa del estado %s al estado %s", pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));
    
    asignar_timer_blocked(pcb);
    queue_push(cola_blocked, pcb);
    pthread_mutex_unlock(&mutex_blocked);

    char* cpu_id_str = string_itoa(cpu_id);
    int* socket_interrupt_ptr = dictionary_get(tabla_interrupt, cpu_id_str);
    socket_interrupt = *socket_interrupt_ptr;
    free(cpu_id_str);

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

    log_warning(logger, "3- cpu aniadida %d", *nuevo_cpu_id);
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

    cerrar_conexion_memoria(socket_memoria);

    // pasar todo lo de abajo a una funcion recibir confirmacion dumpeo
    //

    if (recibir_operacion(socket_memoria) == MEMORY_DUMP){
        estado_anterior = pcb->estado_actual;
        cambiar_estado(pcb, READY);
        log_info(logger, "(%d) Pasa del estado %s al estado %s", pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));
        
    } else{
        log_error(logger, "No se logró llevar a cabo el MEMORY DUMP");
        estado_anterior = pcb->estado_actual;
        cambiar_estado(pcb, EXIT);
        log_info(logger, "(%d) Pasa del estado %s al estado %s", pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));
        borrar_pcb(pcb);
    }

    list_destroy_and_destroy_elements(recibido, free);
}

void iniciar_proceso(int socket_cpu){
    t_list* recibido = recibir_paquete(socket_cpu);
    int pid_anterior = *((int*)list_get(recibido, 0));
    int tamanio_proceso = *((int*)list_get(recibido, 1));
    char* nombre_archivo = list_get(recibido, 2);

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
    int pid = *((int*)list_get(recibido, 0));

    t_pcb* pcb = obtener_pcb(pid);
    actualizar_estimacion_rafaga(pcb);
    finalizar_proceso(pcb);
}