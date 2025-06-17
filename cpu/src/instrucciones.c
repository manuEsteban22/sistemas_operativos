#include<instrucciones.h>

/*
cpu recibe de memoria como parte del handshake:
tamaño_pagina
entradas_por_tabla
cantidad_niveles
*/

t_id_instruccion convertir_a_opcode(char* identificador) {
//recibo un string con la instruccion y devuelvo a cual corresponde con su OPCODE
    if (strcmp(identificador, "NOOP") == 0) return NOOP;
    if (strcmp(identificador, "WRITE") == 0) return WRITE;
    if (strcmp(identificador, "READ") == 0) return READ;
    if (strcmp(identificador, "GOTO") == 0) return GOTO;
    if (strcmp(identificador, "IO") == 0) return IO;
    if (strcmp(identificador, "INIT_PROC") == 0) return INIT_PROC;
    if (strcmp(identificador, "DUMP_MEMORY") == 0) return DUMP_MEMORY;
    if (strcmp(identificador, "EXIT") == 0) return EXIT;

    log_error(logger, "la instruccion que llego no es valida");
    return EXIT;
}

t_instruccion* leerInstruccion(char* instruccion_raw){
//separo toda la linea de la instruccion en 3 variables que van dentro de un t_instruccion
    t_instruccion* instruccion = malloc(sizeof(t_instruccion));
    instruccion->param1 = NULL;
    instruccion->param2 = NULL;

    char **separado = string_split(instruccion_raw, " ");

    int cant_param = 0;
    while(separado[cant_param] != NULL) cant_param++;
    
    instruccion->identificador = convertir_a_opcode(separado[0]);
    
    if (cant_param > 1){
        instruccion->param1 = strdup(separado[1]);
    }
    if (cant_param > 2){
        instruccion->param2 = strdup(separado[2]);
    }

    string_array_destroy(separado);
    free(instruccion_raw);
    return instruccion;
}

t_instruccion* fetch(t_pcb* pcb, int socket_memoria){
    int pc = pcb->pc;
    int pid = pcb->pid;
    t_instruccion* proxima_instruccion;

    t_paquete* paquete_pid_pc = crear_paquete();
    agregar_a_paquete(paquete_pid_pc, &pid, sizeof(int));
    agregar_a_paquete(paquete_pid_pc, &pc, sizeof(int));
    enviar_paquete(paquete_pid_pc, socket_memoria, logger);
    borrar_paquete(paquete_pid_pc);

    if(recibir_operacion(socket_memoria) != PAQUETE){
        return NULL;
    }
    t_list* recibido = recibir_paquete(socket_memoria);
    if (recibido == NULL || list_size(recibido) == 0) {
        log_error(logger, "Error al recibir instrucciones de memoria");
        return NULL;
    }

    void* elemento = list_get(recibido, 0);
    if (elemento == NULL) {
        log_error(logger, "Elemento NULL recibido en list_get");
        list_destroy_and_destroy_elements(recibido, free);
        return NULL;
    }

    char* instruccion_sin_traducir = strdup((char*)elemento);
    log_info(logger, "Instrucción sin traducir: %s", instruccion_sin_traducir);
    proxima_instruccion = leerInstruccion(instruccion_sin_traducir);

    log_info(logger, "Instruccion: %d, param1: %s, param2: %s", proxima_instruccion->identificador, proxima_instruccion->param1, proxima_instruccion->param2);
    //borrar espacios y separar en una instruccion
    list_destroy_and_destroy_elements(recibido, free);

    //return proxima_instruccion;
    return proxima_instruccion;
}

bool requiere_traduccion(t_instruccion* instruccion){
    if(instruccion->identificador == WRITE || instruccion->identificador == READ){
        return true;
    }
    else{
        return false;
    }
}

int decode(t_instruccion* instruccion/*, t_pcb* pcb, int socket_memoria*/){
    if(!requiere_traduccion(instruccion)) return -1;
    int direccion_logica = (*(int*)instruccion->param1);
    int direccion_fisica = direccion_logica;
    return direccion_fisica;
    /*
    int nro_pagina = direccion_logica / tamanio_pagina;
    int desplazamiento = direccion_logica % tamanio_pagina;
    int marco;

    if(entradas_tlb > 0 && esta_en_tlb(nro_pagina, &marco)){
        log_info(logger,"PID: %d - TLB HIT - Pagina: %d", pcb->pid, nro_pagina);
        return 
    } else{
        log_info(logger,"PID: %d - TLB MISS - Pagina: %d", pcb->pid, nro_pagina);
        pedir_frame(pcb, nro_pagina, marco, socket_memoria);
    }

    int direccion_fisica = marco * tamanio_pagina + desplazamiento;
    log_info(logger, "Dirección fisica %d", direccion_fisica);*/
}

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
    int tiempo = *(int*)instruccion->param2;
    
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, SYSCALL_IO);

    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));

    int size_dispositivo = strlen(dispositivo) + 1;
    agregar_a_paquete(paquete, &size_dispositivo, sizeof(int));
    agregar_a_paquete(paquete, dispositivo, size_dispositivo);
    agregar_a_paquete(paquete, &tiempo, sizeof(int));
    enviar_paquete(paquete, socket_kernel_dispatch, logger);
    borrar_paquete(paquete);
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

void execute(t_instruccion* instruccion, int socket_memoria, int socket_kernel_dispatch, t_pcb* pcb){
    int direccion_fisica;
    int pid = pcb->pid;
    int pc = pcb->pc;
    switch (instruccion->identificador)
    {
    case NOOP:
        //no hace nada
        log_info(logger, "ejecute un noop");
        break;
    case WRITE:
        direccion_fisica = decode(instruccion);
        ejecutar_write(instruccion, socket_memoria, direccion_fisica, pid);
        break;
    case READ:
        direccion_fisica = decode(instruccion);
        ejecutar_read(instruccion, socket_memoria, direccion_fisica, pid);
        break;
    case GOTO:
        log_info(logger, "ejecute un GOTO");
        pc = (int)instruccion->param1;
        break;
    case IO:
        ejecutar_io(instruccion, pcb, socket_kernel_dispatch);
        break;
    case INIT_PROC:
        init_proc(instruccion, pcb, socket_kernel_dispatch);
        break;
    case DUMP_MEMORY:
        dump_memory(pcb, socket_kernel_dispatch);
        break;
    case EXIT:
        exit_syscall(pcb, socket_kernel_dispatch);
        break;
    default:
        break;
    }
    pcb->pc = pc;
    return;
}

void prueba_write(int socket_memoria, int socket_kernel_dispatch){
    t_instruccion* instruccion = malloc(sizeof(t_instruccion));
    instruccion->identificador = OC_WRITE;
    int* direccion_logica = malloc(sizeof(int));
    *direccion_logica = 128;
    instruccion->param1 = direccion_logica;
    char* datos = strdup("prueba");
    instruccion->param2 = datos;
    t_pcb* pcb_prueba = malloc(sizeof(t_pcb));
    pcb_prueba->pid = 4;
    execute(instruccion, socket_memoria, socket_kernel_dispatch, pcb_prueba);
    free(direccion_logica);
    free(datos);
    free(instruccion);
}

void prueba(int socket_memoria, int socket_kernel_dispatch){
    t_instruccion* prox;
    t_pcb* pcb_prueba = malloc(sizeof(t_pcb));
    pcb_prueba->pc = 0;
    pcb_prueba->pid = 1;
    prox = fetch(pcb_prueba, socket_memoria);
    execute(prox, socket_memoria, socket_kernel_dispatch, pcb_prueba);
    free(pcb_prueba);
    free(prox);
}

void iniciar_ciclo_de_instrucciones(int socket_memoria, int socket_kernel_dispatch){
    //hacer un bucle que llame a cada parte del ciclo hasta que el fetch devuelva un exit
    bool leyo_exit = false;
    t_instruccion* prox;
    t_pcb* pcb_prueba = malloc(sizeof(t_pcb));
    //valores de prueba, despues hay que recibirlos por dispatch del kernel
    pcb_prueba->pc = 0;
    pcb_prueba->pid = 1;

    while(!leyo_exit){
        prox = fetch(pcb_prueba, socket_memoria);
        if(prox->identificador == EXIT){
            leyo_exit = true;
            log_info(logger, "lei un exit");
        }
        execute(prox, socket_memoria, socket_kernel_dispatch, pcb_prueba);
        pcb_prueba->pc++;
    }
    free(pcb_prueba);
    free(prox);
}