#include <syscalls.h>

void llamar_a_io(int socket_cpu) {
    t_list* campos = recibir_paquete(socket_cpu);

    char* pid_raw = list_get(campos, 0);
    char* pc_raw = list_get(campos, 1);
    //char* size_disp_raw = list_get(campos, 2);
    char* dispositivo = list_get(campos, 3);
    char* tiempo_raw = list_get(campos, 4);

    int pid = *(int*)pid_raw;
    int pc = *(int*)pc_raw;
    int tiempo = *(int*)tiempo_raw;

    log_info(logger, "recibi syscall IO - PID %d - PC %d - Dispositivo [%s] - Tiempo %d", pid, pc, dispositivo, tiempo);

    t_dispositivo_io* io = dictionary_get(dispositivos_io, dispositivo);

    if(io == NULL) {
        log_error(logger, "dispositivo IO [%s] no existe. Enviando proceso a EXIT", dispositivo);
        t_pcb* pcb = obtener_pcb(pid);
        cambiar_estado(pcb, EXIT);
        borrar_pcb(pcb);
        list_destroy_and_destroy_elements(campos, free);
        return;
    }

    pthread_mutex_lock(&mutex_blocked);
    t_pcb* pcb = obtener_pcb(pid);
    pcb->pc = pc;
    cambiar_estado(pcb, BLOCKED);
    asignar_timer_blocked(pcb);
    queue_push(cola_blocked, pcb);
    pthread_mutex_unlock(&mutex_blocked);


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

    list_destroy_and_destroy_elements(campos, free);
}

void dump_memory(int socket_cpu){
    t_list* recibido = list_create();
    int pid = list_get(recibido, 0);

    t_pcb* pcb = obtener_pcb(pid);
    cambiar_estado(pcb, BLOCKED);

    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, SOLICITUD_DUMP_MEMORY);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    socket_memoria = operacion_con_memoria();
    enviar_paquete(paquete, socket_memoria, logger);
    borrar_paquete(paquete);

    if(recibir_operacion(socket_memoria) == MEMORY_DUMP){
        cambiar_estado(pcb, READY);
    }else{
        log_error(logger, "No se logró llevar a cabo el MEMORY DUMP");
        cambiar_estado(pcb, EXIT);
    }
    cerrar_conexion_memoria(socket_memoria);
}

void iniciar_proceso(int socket_cpu){
    printf("iniciar");
}