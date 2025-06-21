#include <instrucciones.h>

void ejecutar_write(t_instruccion* instruccion, int socket_memoria, int direccion_fisica, int pid){
    char* datos = (char*)instruccion->param2;
    int size_datos = strlen(datos);

    t_paquete* paquete = crear_paquete();
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(int));
    agregar_a_paquete(paquete, datos, size_datos);
    cambiar_opcode_paquete(paquete, OC_WRITE);
    enviar_paquete(paquete, socket_memoria, logger);
    borrar_paquete(paquete);

    log_info(logger, "PID: %d - Accion: ESCRIBIR - Direccion fisica: %d - Valor: %s", pid, direccion_fisica, datos);
}

char* ejecutar_read(t_instruccion* instruccion, int socket_memoria, int direccion_fisica, int pid){
    int tamanio = (*(int*)instruccion->param2);

    t_paquete* paquete = crear_paquete();
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(int));
    agregar_a_paquete(paquete, &tamanio, sizeof(int));
    cambiar_opcode_paquete(paquete, OC_READ);
    enviar_paquete(paquete, socket_memoria, logger);
    borrar_paquete(paquete);

    t_list* contenido = recibir_paquete(socket_memoria);
    char* datos = list_get(contenido, 0);
    log_info(logger, "PID: %d - Accion: LEER - Direccion fisica: %d - Valor: %s", pid, direccion_fisica, datos);

    char* copia_datos = strdup(datos);

    list_destroy_and_destroy_elements(contenido, free);

    return copia_datos;
}

//le dice a kernel que envie el dispositivo a io, formato [pid][tam_disp][disp][tiempo]
void ejecutar_io(t_instruccion* instruccion, t_pcb* pcb, int socket_kernel_dispatch){
    char* dispositivo = (char*)instruccion->param1;
    int tiempo = atoi(instruccion->param2);

    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, SYSCALL_IO);

    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    agregar_a_paquete(paquete, &(pcb->pc) + 1, sizeof(int));

    int size_dispositivo = strlen(dispositivo) + 1;
    agregar_a_paquete(paquete, &size_dispositivo, sizeof(int));
    agregar_a_paquete(paquete, dispositivo, size_dispositivo);
    agregar_a_paquete(paquete, &tiempo, sizeof(int));
    
    enviar_paquete(paquete, socket_kernel_dispatch, logger);
    borrar_paquete(paquete);
    log_trace(logger, "envié instruccion io en pid: %d", pcb->pid);
    return;
}

void init_proc(t_instruccion* instruccion, t_pcb* pcb, int socket_kernel_dispatch){
    char* archivo_instrucciones = (char*)instruccion->param1;
    int tamanio = (int)instruccion->param2;

    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, SYSCALL_INIT);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    agregar_a_paquete(paquete, &tamanio, sizeof(int));
    agregar_a_paquete(paquete, archivo_instrucciones, strlen(archivo_instrucciones) + 1);
    enviar_paquete(paquete, socket_kernel_dispatch, logger);
    borrar_paquete(paquete);
    return;
}

void dump_memory(t_pcb* pcb, int socket_kernel_dispatch){
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, SYSCALL_DUMP_MEMORY);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    enviar_paquete(paquete, socket_kernel_dispatch, logger);
    borrar_paquete(paquete);
}

void exit_syscall(t_pcb* pcb, int socket_kernel_dispatch){
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, SYSCALL_EXIT);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    enviar_paquete(paquete, socket_kernel_dispatch, logger);
    borrar_paquete(paquete);
}