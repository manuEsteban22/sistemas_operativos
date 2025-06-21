#include<tlb_cache.h>

t_list* tlb;
t_list* cache;



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
            
            log_info(logger, "PID: %d - Cache Hit - Pagina: %d", pcb->pid, pagina);
            return true;
        }
    }
    
    log_info(logger, "PID: %d - Cache Miss - Pagina: %d", pcb->pid, pagina);
    return false;
}





int pedir_frame(t_pcb* pcb, int nro_pagina, int socket_memoria){
    t_paquete* paquete = crear_paquete();
    t_paquete_frame* contenido;
    contenido->pagina = nro_pagina;
    contenido->pid = pcb->pid;
    agregar_a_paquete(paquete, contenido, sizeof(t_paquete_frame));
}