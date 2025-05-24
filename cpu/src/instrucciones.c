#include<instrucciones.h>

/*cpu recibe de memoria como parte del handshake:
tamaño_pagina
entradas_por_tabla
cantidad_niveles
*/

t_instruccion* fetch(t_pcb* pcb, int socket_memoria){
    int pc = pcb->pc;
    int pid = pcb->pid;
    
    send(socket_memoria, &pid, sizeof(int), 0);
    send(socket_memoria, &pc, sizeof(int), 0);

    t_instruccion* proxima_instruccion;// = recibir_instruccion(socket_memoria);
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
    int direccion_logica = (int*)instruccion->param1;
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
    cambiar_opcode_paquete(paquete, WRITE);
    enviar_paquete(paquete, socket_memoria);
    borrar_paquete(paquete);

    log_info(logger, "PID: %d - Accion: ESCRIBIR - Direccion fisica: %d - Valor: %s", pid, direccion_fisica, datos);
}

char* ejecutar_read(t_instruccion* instruccion, int socket_memoria, int direccion_fisica, int pid){
    int tamanio = ((int*)instruccion->param2);

    t_paquete* paquete = crear_paquete();
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(int));
    agregar_a_paquete(paquete, &tamanio, sizeof(int));
    cambiar_opcode_paquete(paquete, READ);
    enviar_paquete(paquete, socket_memoria);
    borrar_paquete(paquete);

    t_list* contenido = recibir_paquete(socket_memoria);
    char* datos = list_get(contenido, 0);
    log_info(logger, "PID: %d - Accion: LEER - Direccion fisica: %d - Valor: %s", pid, direccion_fisica, datos);

    char* copia_datos = strdup(datos);

    list_destroy_and_destroy_elements(contenido, free);

    return copia_datos;
}

void execute(t_instruccion* instruccion, int socket_memoria, t_pcb* pcb){
    int direccion_fisica;
    int pid = pcb->pid;
    int pc = pcb->pc;
    switch (instruccion->identificador)
    {
    case NOOP:
        //no hace nada
        break;
    case WRITE:
        direccion_fisica = decode(instruccion);
        ejecutar_write(instruccion, socket_memoria, direccion_fisica, pid);
        break;
    case READ:
        direccion_fisica = decode(instruccion);
        ejecutar_read(instruccion, socket_memoria, direccion_fisica, pid);
    case GOTO:
        
        break;
    default:
        break;
    }
}

void prueba_write(int socket_memoria){
    t_instruccion* instruccion = malloc(sizeof(t_instruccion));
    instruccion->identificador = WRITE;
    int* direccion_logica = malloc(sizeof(int));
    *direccion_logica = 128;
    instruccion->param1 = direccion_logica;
    char* datos = strdup("HolaMundo");
    instruccion->param2 = datos;
    int pid_prueba = 4;
    execute(instruccion, socket_memoria, pid_prueba);
    free(direccion_logica);
    free(datos);
    free(instruccion);
}

void iniciar_ciclo_de_instrucciones(){

}