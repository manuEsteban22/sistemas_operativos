#include<tlb_cache.h>

t_list* tlb;
t_list* cache;


void escribir_pagina_en_memoria(int marco, void* contenido, int socket_memoria) {
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OC_PAG);
    
    agregar_a_paquete(paquete, &marco, sizeof(int));
    agregar_a_paquete(paquete, &contenido, string_length(contenido));
    enviar_paquete(paquete, socket_memoria, logger);
    borrar_paquete(paquete);
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

void actualizar_tlb(int pagina, int marco) {
    t_entrada_tlb* nueva_entrada = malloc(sizeof(t_entrada_tlb));
    nueva_entrada->pagina = pagina;
    nueva_entrada->marco = marco;

    if (list_size(tlb) >= entradas_tlb) {
        int indice_victima = 0;
        
        if (strcmp(reemplazo_tlb, "FIFO") == 0) {
            indice_victima = 0;
        } else if (strcmp(reemplazo_tlb, "LRU") == 0) {
            // completar LRU
        }
        
        t_entrada_tlb* victima = list_remove(tlb, indice_victima);
        free(victima);
    }
    
    list_add(tlb, nueva_entrada);
}

void limpiar_tlb() {
    list_destroy_and_destroy_elements(tlb, free);
    tlb = list_create();
}

bool esta_en_cache(int pagina, int* marco, t_pcb* pcb){
    for (int i = 0; i < list_size(cache); i++) {
        t_entrada_cache* entrada = list_get(cache, i);
        if (entrada->pagina == pagina) {
            *marco = entrada->marco;
            entrada->usado = true;
            log_info(logger, "PID: %d - Cache Hit - Pagina: %d", pcb->pid, pagina);
            return true;
        }
    }
    
    log_info(logger, "PID: %d - Cache Miss - Pagina: %d", pcb->pid, pagina);
    return false;
}

void actualizar_cache(int pagina, int marco, void* contenido, bool modificado, t_pcb* pcb, int socket_memoria) {
    t_entrada_cache* nueva_entrada = malloc(sizeof(t_entrada_cache));
    nueva_entrada->pagina = pagina;
    nueva_entrada->marco = marco;
    nueva_entrada->contenido = malloc(tam_pagina);
    memcpy(nueva_entrada->contenido, contenido, tam_pagina);
    nueva_entrada->modificado = modificado;
    nueva_entrada->usado = true;
    
    if (list_size(cache) >= entradas_cache) {
        int indice_victima = encontrar_victima_cache();
        
        t_entrada_cache* victima = list_get(cache, indice_victima);
        if (victima->modificado) {
            escribir_pagina_en_memoria(victima->marco, victima->contenido, socket_memoria);
            log_info(logger, "PID: %d - Memory Update - Página: %d - Frame: %d", pcb->pid, victima->pagina, victima->marco);
        }
        
        list_remove(cache, indice_victima);
        free(victima->contenido);
        free(victima);
    }
    
    list_add(cache, nueva_entrada);
    log_info(logger, "PID: %d - Cache Add - Pagina: %d", pcb->pid, pagina);
}

int encontrar_victima_cache() {
    int puntero_clock = 0;
    
    if(strcmp(reemplazo_cache, "CLOCK") == 0 || strcmp(reemplazo_cache, "CLOCK-M") == 0) {
        for(int i = 0; i < list_size(cache); i++) {
            t_entrada_cache* entrada = list_get(cache, puntero_clock);
            if(!entrada->usado) {
                int victima = puntero_clock;
                puntero_clock = (puntero_clock + 1) % entradas_cache;
                return victima;
            }
            entrada->usado = false;
            puntero_clock = (puntero_clock + 1) % entradas_cache;
        }
    }

    //default a fifo
    return 0;
}
int pedir_frame(t_pcb* pcb, int nro_pagina, int socket_memoria){
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OC_FRAME);


    agregar_a_paquete(paquete, nro_pagina, sizeof(int));
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    enviar_paquete(paquete, socket_memoria, logger);
    borrar_paquete(paquete);

    int opcode = recibir_operacion(socket_memoria);
    if(opcode != OC_FRAME){
        log_error(logger, "no se recibio el marco");
        return -1;
    }

    t_list* recibido = recibir_paquete(socket_memoria);
    int* marco = list_get(recibido, 0);

    list_destroy_and_destroy_elements(recibido, free);

    log_info(logger, "PID: %d - OBTENER MARCO - Página: %d - Marco: %d", pcb->pid, nro_pagina, marco);
    return marco;
}