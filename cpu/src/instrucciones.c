#include <instrucciones.h>


void escribir_en_cache(int direccion_fisica, char* datos, t_pcb* pcb) {
    int nro_pagina = direccion_fisica / tam_pagina;
    int desplazamiento = direccion_fisica % tam_pagina;
    int marco;
    
    if(esta_en_cache(nro_pagina, &marco, pcb)) {
        for(int i = 0; i < list_size(cache); i++) {
            t_entrada_cache* entrada = list_get(cache, i);
            if(entrada->pagina == nro_pagina) {
                memcpy(entrada->contenido + desplazamiento, datos, strlen(datos));
                entrada->modificado = true;
                log_info(logger, "PID: %d - Cache Update - Pagina: %d", pcb->pid, nro_pagina);
                return;
            }
        }
    }
}

char* leer_de_cache(int direccion_fisica, int tamanio, t_pcb* pcb) {
    int nro_pagina = direccion_fisica / tam_pagina;
    int desplazamiento = direccion_fisica % tam_pagina;
    void* contenido_pagina = NULL;
    int marco;
    
    if(esta_en_cache(nro_pagina, &marco, pcb)) {
        t_entrada_cache* entrada = NULL;
        for(int i = 0; i < list_size(cache); i++) {
            entrada = list_get(cache, i);
            if(entrada->pagina == nro_pagina) {
                break;
            }
        }
        
        if(entrada != NULL) {
            char* datos = malloc(tamanio + 1);
            memcpy(datos, entrada->contenido + desplazamiento, tamanio);
            //datos[tamanio] = '\0';
            return datos;
        }
    }
    return NULL;
}

void ejecutar_write(t_instruccion* instruccion, int direccion_fisica, t_pcb* pcb){
    if(instruccion == NULL || pcb == NULL || instruccion->param2 == NULL){
        log_error(logger,"## Error en la instruccion o el pcb");
        return;
    }

    char* datos = (char*)instruccion->param2;
    int size_datos = strlen(datos);

    //escribir_en_cache(direccion_fisica, datos, pcb);

    t_paquete* paquete = crear_paquete();
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(int));
    agregar_a_paquete(paquete, datos, size_datos);
    cambiar_opcode_paquete(paquete, OC_WRITE);
    enviar_paquete(paquete, socket_memoria, logger);
    borrar_paquete(paquete);

    log_info(logger, "PID: %d - Accion: ESCRIBIR - Direccion fisica: %d - Valor: %s", pcb->pid, direccion_fisica, datos);

    int confirmacion = recibir_operacion(socket_memoria);
    if(confirmacion == OK){
        log_trace(logger, "todo joya");
    }
}

char* ejecutar_read(t_instruccion* instruccion, int direccion_fisica, t_pcb* pcb){
    int tamanio = (*(int*)instruccion->param2);

    char* lectura_cache = leer_de_cache(direccion_fisica, tamanio, pcb);
    if(lectura_cache != NULL){
        log_info(logger, "pid%d, los datos: %s estaban en cache", pcb->pid, lectura_cache);
        return lectura_cache;
    }

    t_paquete* paquete = crear_paquete();
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(int));
    agregar_a_paquete(paquete, &tamanio, sizeof(int));
    cambiar_opcode_paquete(paquete, OC_READ);
    enviar_paquete(paquete, socket_memoria, logger);
    borrar_paquete(paquete);

    t_list* contenido = recibir_paquete(socket_memoria);
    char* datos = list_get(contenido, 0);

    int nro_pagina = direccion_fisica / tam_pagina;
    int marco = traducir_direccion(pcb, direccion_fisica) / tam_pagina;
    actualizar_cache(nro_pagina,marco,datos,false,pcb);

    log_info(logger, "PID: %d - Accion: LEER - Direccion fisica: %d - Valor: %s", pcb->pid, direccion_fisica, datos);

    char* copia_datos = strdup(datos);

    list_destroy_and_destroy_elements(contenido, free);

    return copia_datos;
}

//le dice a kernel que envie el dispositivo a io, formato [pid][tam_disp][disp][tiempo]
void ejecutar_io(t_instruccion* instruccion, t_pcb* pcb){
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
    
    enviar_paquete(paquete, socket_kernel_dispatch, logger);
    borrar_paquete(paquete);
    log_trace(logger, "envié instruccion io en pid: %d", pcb->pid);
    return;
}

void init_proc(t_instruccion* instruccion, t_pcb* pcb){
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

void dump_memory(t_pcb* pcb){
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, SYSCALL_DUMP_MEMORY);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    enviar_paquete(paquete, socket_kernel_dispatch, logger);
    borrar_paquete(paquete);
}

void exit_syscall(t_pcb* pcb){
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, SYSCALL_EXIT);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    enviar_paquete(paquete, socket_kernel_dispatch, logger);
    borrar_paquete(paquete);
}