#include<instrucciones.h>

/*cpu recibe de memoria como parte del handshake:
tamaño_pagina
entradas_por_tabla
cantidad_niveles
*/
int tamanio_pagina = 64;
int entradas_por_tabla = 4;
int cantidad_niveles = 3;
t_list* tlb;

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

bool esta_en_tlb(int pagina, int* marco){
    for(int i=0;i < list_size(tlb);i++){
        t_entrada_tlb* entrada = list_get(tlb, i);
        if(entrada->pagina == pagina){
            *marco = entrada->marco;
            return true;
        }
    }
    return false;
}

void limpiar_tlb() {
    list_destroy_and_destroy_elements(tlb, free);
    tlb = list_create();
}

void decode(t_instruccion* instruccion, t_pcb* pcb){
    if(!requiere_traduccion(instruccion)) return;

    int direccion_logica = instruccion->param1;
    int nro_pagina = direccion_logica / tamanio_pagina;
    int desplazamiento = direccion_logica % tamanio_pagina;
    int marco;

    if(entradas_tlb > 0 && esta_en_tlb(nro_pagina, &marco)){
        log_info(logger,"PID: %d - TLB HIT - Pagina: %s", pcb->pid, nro_pagina);
        return 
    } else{
        log_info(logger,"PID: %d - TLB MISS - Pagina: %s", pcb->pid, nro_pagina);
    }

    int direccion_fisica = marco * tamanio_pagina + desplazamiento;
    log_info(logger, "Dirección fisica %d", direccion_fisica);
}