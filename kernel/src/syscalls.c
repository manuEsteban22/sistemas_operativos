#include <syscalls.h>


void llamar_a_io(socket_cpu){
    int pid;
    int size_dispositivo;
    char* dispositivo;
    int tiempo;
    recv(socket_cpu, &pid, sizeof(int), MSG_WAITALL);
    recv(socket_cpu, &size_dispositivo, sizeof(int), MSG_WAITALL);
    dispositivo = malloc(size_dispositivo);
    recv(socket_cpu, dispositivo, size_dispositivo, MSG_WAITALL);
    recv(socket_cpu, &tiempo, sizeof(int), MSG_WAITALL);

    log_info(logger, "recibi syscall IO - PID %d - Dispositivo %s - Tiempo %d", pid, dispositivo, tiempo);

    t_dispositivo_io* io = dictionary_get(dispositivos_io, dispositivo);

    if(io == NULL) {
        log_error(logger, "dispositivo IO [%s] no existe. Enviando proceso a EXIT", dispositivo);
        t_pcb* pcb = obtener_pcb(pid);
        cambiar_estado(pcb, EXIT);
        borrar_pcb(pcb);
        free(dispositivo);
        return;
    }

    t_pcb* pcb = obtener_pcb(pid);
    cambiar_estado(pcb, BLOCKED);

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
        enviar_paquete(paquete, io->socket_io);
        borrar_paquete(paquete);
        log_info(logger, "Dispositivo PID %d enviado a IO", pid);
    }

    free(dispositivo);

}