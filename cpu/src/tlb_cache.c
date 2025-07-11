#include<tlb_cache.h>

t_list* tlb;
t_list* cache;
pthread_mutex_t mutex_tlb = PTHREAD_MUTEX_INITIALIZER;
int contador_acceso = 0;


void inicializar_tlb(){
    if(tlb != NULL){
        list_destroy(tlb);
    }
    tlb = list_create();
    //list_add(tlb, 1);
}

void escribir_pagina_en_memoria(int marco, void* contenido) {
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OC_PAG);
    
    agregar_a_paquete(paquete, &marco, sizeof(int));
    agregar_a_paquete(paquete, &contenido, string_length(contenido));
    enviar_paquete(paquete, socket_memoria, logger);
    borrar_paquete(paquete);
}


int esta_en_tlb(int pagina){
    log_trace(logger,"Buscando pagina %d en TLB", pagina);
    if(tlb == NULL){
        log_error(logger, "Error: TLB no inicializada");
        return -1;
    }
    log_trace(logger, "anda1");
    pthread_mutex_lock(&mutex_tlb);
    int marco = -1;
    log_trace(logger, "anda2");
    for(int i=0;i < list_size(tlb);i++){
        log_trace(logger, "anda3");
        t_entrada_tlb* entrada = list_get(tlb, i);
        log_trace(logger, "anda4");
        if(entrada->pagina == pagina){
            marco = entrada->marco;
            
            entrada->ultimo_acceso = ++contador_acceso;
            log_trace(logger, "anda5");
        }
    }
    pthread_mutex_unlock(&mutex_tlb);
    return marco;
}

void actualizar_tlb(int pagina, int marco) {
    t_entrada_tlb* nueva_entrada = malloc(sizeof(t_entrada_tlb));
    nueva_entrada->pagina = pagina;
    nueva_entrada->marco = marco;
    nueva_entrada->ultimo_acceso = ++contador_acceso;

    if (list_size(tlb) >= entradas_tlb) {
        t_entrada_tlb* victima = NULL;
        if (strcmp(reemplazo_tlb, "FIFO") == 0) {
            victima = list_get(tlb, 0);
        } else if (strcmp(reemplazo_tlb, "LRU") == 0) {
            int mas_antiguo = -1;
            for(int i; i<list_size(tlb);i++){
                t_entrada_tlb* entrada = list_get(tlb,i);
                if(entrada->ultimo_acceso < mas_antiguo){
                    mas_antiguo = entrada->ultimo_acceso;
                    victima = entrada;
                }
            }
        }
        list_remove_element(tlb, victima);
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

void actualizar_cache(int pagina, int marco, void* contenido, bool modificado, t_pcb* pcb) {
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
            escribir_pagina_en_memoria(victima->marco, victima->contenido);
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
int pedir_frame(t_pcb* pcb, int nro_pagina){
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OC_FRAME);
    agregar_a_paquete(paquete, &nro_pagina, sizeof(int));
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    enviar_paquete(paquete, socket_memoria, logger);
    borrar_paquete(paquete);
    log_trace(logger, "## Se envio el paquete para preguntar frame a memoria");

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