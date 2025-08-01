#include<ciclo_instrucciones.h>


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
        if(instruccion->identificador == WRITE || instruccion->identificador == READ){
            int* dir = malloc(sizeof(int));
            *dir = atoi(separado[1]);
            instruccion->param1 = dir;
        }else{
        instruccion->param1 = strdup(separado[1]);
        }
    }
    if (cant_param > 2){
        if(instruccion->identificador == READ){
            int* tam = malloc(sizeof(int));
            *tam = atoi(separado[2]);
            instruccion->param2 = tam;
        }else{
        instruccion->param2 = strdup(separado[2]);
        }
    }

    string_array_destroy(separado);
    free(instruccion_raw);
    return instruccion;
}

t_instruccion* fetch(t_pcb* pcb){
    int pc = pcb->pc;
    int pid = pcb->pid;
    t_instruccion* proxima_instruccion;
    
    log_info(logger, "## PID: %d - FETCH - Program Counter: %d", pcb->pid, pcb->pc);
    t_paquete* paquete_pid_pc = crear_paquete();
    cambiar_opcode_paquete(paquete_pid_pc, FETCH);
    agregar_a_paquete(paquete_pid_pc, &pid, sizeof(int));
    agregar_a_paquete(paquete_pid_pc, &pc, sizeof(int));
    enviar_paquete(paquete_pid_pc, socket_memoria, logger);
    borrar_paquete(paquete_pid_pc);

    int opcode = recibir_operacion(socket_memoria);
    if(opcode != PAQUETE){
        log_error(logger, "## Error en el fetch, se recibio el opcode: %d", opcode);
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
    proxima_instruccion = leerInstruccion(instruccion_sin_traducir);
    list_destroy_and_destroy_elements(recibido, free);
    return proxima_instruccion;
}

bool requiere_traduccion(t_instruccion* instruccion){
    if(instruccion->identificador == WRITE || instruccion->identificador == READ){
        return true;
        log_debug(logger, "Detecté un write o read");
    }
    else{
        return false;
    }
}

int decode(t_instruccion* instruccion, t_pcb* pcb){
    if(!requiere_traduccion(instruccion)) return -1;

    if(instruccion->param1 == NULL){
        log_error(logger, "Error en el decode, parametro de direccion es NULL");
    }
    int direccion_logica = *(int*)instruccion->param1;
    log_trace(logger, "PID: %d - Dirección lógica a traducir: %d", pcb->pid, direccion_logica);

    int direccion_fisica = traducir_direccion(pcb, direccion_logica);

    log_trace(logger, "direccion logica: %d, direccion fisica %d", direccion_logica, direccion_fisica);
    return direccion_fisica;
}

void execute(t_instruccion* instruccion, t_pcb* pcb, int cpu_id){
    int direccion_fisica;
    int direccion_logica;
    char* datos;
    int pid = pcb->pid;
    switch (instruccion->identificador)
    {
    case NOOP:
        log_info(logger, "## PID: %d - Ejecutando: NOOP", pid);
        pcb->pc++;
        break;
    case WRITE:
        direccion_logica = *(int*)instruccion->param1;
        datos = (char*)instruccion->param2;
        log_trace(logger, "dir logica = %d, datos = %s", direccion_logica, datos);

        if(escribir_en_cache(direccion_logica, datos, pcb)){
            log_info(logger, "## PID: %d - Ejecutando: WRITE - %d %s", pid, direccion_logica, (char*)instruccion->param2);
            pcb->pc++;
            log_debug(logger, "Se ejecutó un Write de cache");
            break;
        }
        log_debug(logger, "Se ejecutó un Write sin cache");
        direccion_fisica = decode(instruccion, pcb);
        ejecutar_write(instruccion, direccion_fisica, pcb);    
        
        log_info(logger, "## PID: %d - Ejecutando: WRITE - %d %s", pid, direccion_logica, (char*)instruccion->param2);
        pcb->pc++;
        break;
    case READ:
        direccion_logica = *(int*)instruccion->param1;
        int tamanio = *(int*)instruccion->param2;
        if(entradas_cache <= 0){
            direccion_fisica = decode(instruccion, pcb);
            datos = ejecutar_read(instruccion, direccion_fisica, pcb);
        }else{
            datos = leer_de_cache(direccion_logica,tamanio,pcb);
        }
        
        log_info(logger, "## PID: %d - Ejecutando: READ - %d %d", pid, direccion_logica, tamanio);
        log_trace(logger, "Datos leidos: %s", datos);
        free(datos);
        pcb->pc++;
        break;
    case GOTO:
        log_info(logger, "## PID: %d - Ejecutando: GOTO - %d", pid, atoi(instruccion->param1));
        pcb->pc = atoi(instruccion->param1);
        break;
    case IO:
        log_info(logger, "## PID: %d - Ejecutando: IO - %s %d", pid, (char*)instruccion->param1, atoi(instruccion->param2));
        //pcb->pc++;
        ejecutar_io(instruccion, pcb, cpu_id);
        pcb->pc++;
        break;
    case INIT_PROC:
        init_proc(instruccion, pcb);
        log_info(logger, "## PID: %d - Ejecutando: INIT_PROC - %s %d", pid, (char*)instruccion->param1, atoi(instruccion->param2));
        pcb->pc++;
        break;
    case DUMP_MEMORY:
        log_info(logger, "## PID: %d - Ejecutando: DUMP_MEMORY", pid);
        dump_memory(pcb, cpu_id);
        pcb->pc++;
        break;
    case EXIT:
        log_info(logger, "## PID: %d - Ejecutando: EXIT", pid);
        exit_syscall(pcb);
        break;
    default:
        log_error(logger, "instruccion desconocida");
        pcb->pc++;
        break;
    }
    free(instruccion->param1);
    free(instruccion->param2);
    return;
}

bool check_interrupt(t_pcb* pcb){
    int opcode;
    int recibido = recv(socket_kernel_interrupt, &opcode, sizeof(int), MSG_DONTWAIT);

    if(recibido > 0 && opcode == OC_INTERRUPT){
        log_info(logger, "## Llega interrupción al puerto Interrupt");
        t_paquete* paquete = crear_paquete();
        cambiar_opcode_paquete(paquete, CPU_INTERRUPT);
        agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
        agregar_a_paquete(paquete, &(pcb->pc), sizeof(int));
        enviar_paquete(paquete, socket_kernel_dispatch, logger);
        borrar_paquete(paquete);
        return true;
    }
    return false;
}

void check_interrupt_syscall(t_pcb* pcb){
    int opcode = recibir_operacion(socket_kernel_interrupt);
    if(opcode == OC_INTERRUPT){
        log_info(logger, "## Llega interrupción al puerto Interrupt");
        t_paquete* paquete = crear_paquete();
        cambiar_opcode_paquete(paquete, CPU_INTERRUPT);
        agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
        agregar_a_paquete(paquete, &(pcb->pc), sizeof(int));
        enviar_paquete(paquete, socket_kernel_dispatch, logger);
        borrar_paquete(paquete);
        recibir_operacion(socket_kernel_interrupt);//limpia basura en el buffer
        return;
    }else{log_error(logger, "en vez de llegar interrupcion, llego %d", opcode);}
}


void iniciar_ciclo_de_instrucciones(t_pcb* pcb, int cpu_id){

    //hacer un bucle que llame a cada parte del ciclo hasta que el fetch devuelva un exit
    bool proceso_en_running = true;
    bool hay_interrupcion = false;
    t_instruccion* prox;

    while(proceso_en_running){
        prox = fetch(pcb);
        if(prox == NULL){
            log_error(logger, "Error en el fetch en pc=%d", pcb->pc);
            proceso_en_running = false;
            return;
        }
        int instruccion = prox->identificador;
        if(instruccion == EXIT){
            proceso_en_running = false;
            gestionar_desalojo(pcb);
            log_trace(logger, "lei un exit");
        }
        execute(prox, pcb, cpu_id);

        if(instruccion == IO || instruccion == DUMP_MEMORY){
            check_interrupt_syscall(pcb);
            gestionar_desalojo(pcb);
            proceso_en_running = false;
        }

        if(proceso_en_running){
            hay_interrupcion = check_interrupt(pcb);
            if(hay_interrupcion){
                gestionar_desalojo(pcb);
                //enviar_desalojo_a_kernel(pcb);
                proceso_en_running = false;
            }
        }
        free(prox);
    }
    free(pcb);
    log_trace(logger, "Termino un ciclo cpu");
}