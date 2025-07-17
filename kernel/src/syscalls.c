#include <syscalls.h>

void llamar_a_io(int socket_dispatch) {
    int estado_anterior;
    t_list* campos = recibir_paquete(socket_dispatch);

    char* pid_raw = list_get(campos, 0);
    char* pc_raw = list_get(campos, 1);
    //char* size_disp_raw = list_get(campos, 2);
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
        t_paquete* paquete = crear_paquete();
        cambiar_opcode_paquete(paquete, SOLICITUD_IO);
        agregar_a_paquete(paquete, &pid, sizeof(int));
        agregar_a_paquete(paquete, &tiempo, sizeof(int));
        enviar_paquete(paquete, io->socket_io, logger);
        borrar_paquete(paquete);
        log_trace(logger, "Dispositivo PID %d enviado a IO", pid);
    }

    int* cpu_id_ptr = malloc(sizeof(int));
    *cpu_id_ptr = cpu_id;
    pthread_mutex_lock(&mutex_cpus_libres);
    queue_push(cpus_libres, cpu_id_ptr);
    pthread_mutex_unlock(&mutex_cpus_libres);
    sem_post(&cpus_disponibles);

    list_destroy_and_destroy_elements(campos, free);
}

void dump_memory(int socket_cpu){
    int estado_anterior;

    t_list* recibido = recibir_paquete(socket_dispatch);
    int* pid_ptr = list_get(recibido, 0);
    int* pc_ptr = list_get(recibido, 1);
    int* cpu_id_ptr = list_get(recibido, 2);

    int pid = *pid_ptr;
    int pc = *pc_ptr;
    int cpu_id = *cpu_id_ptr;

    log_trace(logger, "## DUMP MEMORY - PID %d", pid);

    pthread_mutex_lock(&mutex_blocked);
    t_pcb* pcb = obtener_pcb(pid);
    pcb->pc = pc;

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

void ejecutar_exit(int socket_cpu){
    t_list* recibido = recibir_paquete(socket_cpu);
    int pid = *((int*)list_get(recibido, 0));

    t_pcb* pcb = obtener_pcb(pid);
    finalizar_proceso(pcb);
}