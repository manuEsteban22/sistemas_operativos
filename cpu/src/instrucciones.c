#include <instrucciones.h>

void ejecutar_write(t_instruccion* instruccion, int direccion_fisica, t_pcb* pcb){
    if(instruccion == NULL || pcb == NULL || instruccion->param2 == NULL){
        log_error(logger,"## Error en la instruccion o el pcb");
        return;
    }

    char* datos = (char*)instruccion->param2;
    int size_datos = strlen(datos) + 1;


    t_paquete* paquete = crear_paquete();
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(int));
    agregar_a_paquete(paquete, datos, size_datos);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    cambiar_opcode_paquete(paquete, OC_WRITE);
    enviar_paquete(paquete, socket_memoria, logger);
    borrar_paquete(paquete);

    log_info(logger, "PID: %d - Accion: ESCRIBIR - Direccion fisica: %d - Valor: %s", pcb->pid, direccion_fisica, datos);
}

char* ejecutar_read(t_instruccion* instruccion, int direccion_fisica, t_pcb* pcb){
    int tamanio = (*(int*)instruccion->param2);
    t_paquete* paquete = crear_paquete();
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(int));
    agregar_a_paquete(paquete, &tamanio, sizeof(int));
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    cambiar_opcode_paquete(paquete, OC_READ);
    enviar_paquete(paquete, socket_memoria, logger);
    borrar_paquete(paquete);

    if(recibir_operacion(socket_memoria) != OC_READ){
        log_error(logger, "No se recibió la lectura");
    }
    t_list* contenido = recibir_paquete(socket_memoria);
    char* datos_crudos = list_get(contenido, 0);
    char* datos_leidos = strndup(datos_crudos, tamanio);

    log_info(logger, "PID: %d - Accion: LEER - Direccion fisica: %d - Valor: %s", pcb->pid, direccion_fisica, datos_leidos);

    list_destroy_and_destroy_elements(contenido, free);

    return datos_leidos;
}

//le dice a kernel que envie el dispositivo a io, formato [pid][tam_disp][disp][tiempo]
void ejecutar_io(t_instruccion* instruccion, t_pcb* pcb, int cpu_id){
    char* dispositivo = (char*)instruccion->param1;
    int tiempo = atoi(instruccion->param2);

    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, SYSCALL_IO);

    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    agregar_a_paquete(paquete, &(pcb->pc), sizeof(int));

    int size_dispositivo = strlen(dispositivo) + 1;
    agregar_a_paquete(paquete, &size_dispositivo, sizeof(int));
    agregar_a_paquete(paquete, dispositivo, size_dispositivo);
    agregar_a_paquete(paquete, &tiempo, sizeof(int));
    agregar_a_paquete(paquete, &cpu_id, sizeof(int));
    
    enviar_paquete(paquete, socket_kernel_dispatch, logger);
    borrar_paquete(paquete);

    // int respuesta = recibir_operacion(socket_kernel_dispatch);
    // if(respuesta == OK){
    //     log_trace(logger, "Recibi confirmacion de IO de kernel");
    //     return;
    // }
    // else{
    //     log_error(logger, "No se recibio respuesta del IO de kernel, %d", respuesta);
    //     recv(socket_kernel_dispatch, &respuesta, sizeof(int), MSG_WAITALL);
    //     log_trace(logger, "Opcode %d", respuesta);
    // }
}

void init_proc(t_instruccion* instruccion, t_pcb* pcb){
    char* archivo_instrucciones = (char*)instruccion->param1;
    int tamanio = atoi(instruccion->param2);

    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, SYSCALL_INIT);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    agregar_a_paquete(paquete, &tamanio, sizeof(int));
    agregar_a_paquete(paquete, archivo_instrucciones, strlen(archivo_instrucciones) + 1);
    enviar_paquete(paquete, socket_kernel_dispatch, logger);
    borrar_paquete(paquete);
    return;
}

void dump_memory(t_pcb* pcb, int cpu_id){
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, SYSCALL_DUMP_MEMORY);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    agregar_a_paquete(paquete, &(pcb->pc), sizeof(int));
    agregar_a_paquete(paquete, &cpu_id, sizeof(int));
    enviar_paquete(paquete, socket_kernel_dispatch, logger);
    borrar_paquete(paquete);

    int respuesta = recibir_operacion(socket_kernel_dispatch);
    if(respuesta == OK){
        log_trace(logger, "Recibi confirmacion de DUMP de kernel");
        return;
    }
    else{
        log_error(logger, "No se recibio respuesta del DUMP de kernel");
    }
}

void exit_syscall(t_pcb* pcb){
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, SYSCALL_EXIT);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    enviar_paquete(paquete, socket_kernel_dispatch, logger);
    borrar_paquete(paquete);
}