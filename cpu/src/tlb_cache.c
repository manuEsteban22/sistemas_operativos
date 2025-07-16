#include<tlb_cache.h>

t_list* tlb;
t_list* cache;
pthread_mutex_t mutex_tlb = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cache = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cache_clock = PTHREAD_MUTEX_INITIALIZER;
int contador_acceso = 0;
int puntero_clock;

void inicializar_tlb(){
    if(tlb != NULL){
        list_destroy(tlb);
    }
    tlb = list_create();
}

void inicializar_cache() {
    cache = list_create();
}

void escribir_pagina_en_memoria(int direccion_fisica, void* contenido, t_pcb* pcb) {
    log_debug(logger, "PID: %d, DIRECCION FISICA: %d", pcb->pid, direccion_fisica);
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OC_PAG_WRITE);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(int));
    agregar_a_paquete(paquete, contenido, tam_pagina);
    enviar_paquete(paquete, socket_memoria, logger);
    borrar_paquete(paquete);
}
char* leer_pagina_memoria(int direccion_fisica, t_pcb* pcb){
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OC_PAG_READ);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(int));
    enviar_paquete(paquete, socket_memoria, logger);
    borrar_paquete(paquete);

    if(recibir_operacion(socket_memoria) == OC_PAG_READ){
        t_list* recibido = recibir_paquete(socket_memoria);
        char* contenido = (char*)list_get(recibido, 0);

        char* pagina = malloc(tam_pagina);
        memcpy(pagina, contenido, tam_pagina);

        log_trace(logger, "Se leyo una pagina de memoria con contenido %s", contenido);
        list_destroy_and_destroy_elements(recibido, free);
        return pagina;
    }
}

void cache_miss(int nro_pagina, t_pcb* pcb){
    log_info(logger, "PID: %d - Cache Miss - Pagina: %d", pcb->pid, nro_pagina);
    int direccion_logica = nro_pagina * tam_pagina;
    int direccion_fisica = traducir_direccion(pcb, direccion_logica);
    int marco = direccion_fisica / tam_pagina;

    char* pagina_leida = leer_pagina_memoria(direccion_fisica, pcb);

    actualizar_cache(nro_pagina, marco, pagina_leida, false, pcb);
    free(pagina_leida);
}

int esta_en_tlb(int pagina){
    log_trace(logger,"Buscando pagina %d en TLB", pagina);
    if(tlb == NULL){
        log_error(logger, "Error: TLB no inicializada");
        return -1;
    }
    pthread_mutex_lock(&mutex_tlb);
    int marco = -1;
    for(int i=0;i < list_size(tlb);i++){
        t_entrada_tlb* entrada = list_get(tlb, i);
        if(entrada->pagina == pagina){
            marco = entrada->marco;
            
            entrada->ultimo_acceso = ++contador_acceso;
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
            for(int i = 0; i<list_size(tlb);i++){
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
    pthread_mutex_lock(&mutex_cache);
    for (int i = 0; i < list_size(cache); i++) {
        t_entrada_cache* entrada = list_get(cache, i);
        if (entrada->pagina == pagina) {
            *marco = entrada->marco;
            entrada->usado = true;
            pthread_mutex_unlock(&mutex_cache);
            return true;
        }
    }
    pthread_mutex_unlock(&mutex_cache);
    return false;
}

void* leer_de_cache(int direccion_logica, int tamanio, t_pcb* pcb){
    if(entradas_cache <= 0){
        log_error(logger, "La cache no esta activada");
        return NULL;
    }

    int nro_pagina = direccion_logica / tam_pagina;
    int desplazamiento = direccion_logica % tam_pagina;
    int marco;
    char* datos = NULL;

    bool hit = esta_en_cache(nro_pagina, &marco, pcb);

    log_debug(logger, "debug_1.2, nro_pagina %d", nro_pagina);
    if(!hit){
        // log_info(logger, "PID: %d - Cache Miss - Pagina: %d", pcb->pid, nro_pagina);
        
        // log_debug(logger, "La pagina no estaba en cache");
        // datos = leer_pagina_memoria(nro_pagina, pcb);
        // int direccion_fisica = traducir_direccion(pcb, direccion_logica);
        // int marco = direccion_fisica / tam_pagina;

        // actualizar_cache(nro_pagina, marco, datos, false, pcb);
        // free(datos);
        cache_miss(nro_pagina, pcb);

        usleep(retardo_cache * 1000);
        esta_en_cache(nro_pagina, &marco, pcb);
        // return leer_de_cache(direccion_logica, tamanio, pcb);
    }else{
        log_info(logger, "PID: %d - Cache Hit - Pagina: %d", pcb->pid, nro_pagina);
    }
    pthread_mutex_lock(&mutex_cache);

    t_entrada_cache* entrada = NULL;
    for(int i = 0; i < list_size(cache); i++) {
        t_entrada_cache* temp = list_get(cache, i);
        if(temp->pagina == nro_pagina) {
            entrada = temp;
            break;
        }
    }

    if(entrada == NULL) {
        pthread_mutex_unlock(&mutex_cache);
        return NULL;
    }
    
    if(desplazamiento + tamanio > tam_pagina) {
        log_error(logger, "Se intento leer fuera de la pagina");
        pthread_mutex_unlock(&mutex_cache);
        return NULL;
    }
    datos = malloc(tamanio + 1);
    memcpy(datos, entrada->contenido + desplazamiento, tamanio);

    entrada->usado = true;
    pthread_mutex_unlock(&mutex_cache);
    usleep(retardo_cache * 1000);

    return datos;
}

bool escribir_en_cache(int direccion_logica, char* datos, t_pcb* pcb){
    if(entradas_cache <= 0){
        return false;
    }
    int nro_pagina = direccion_logica / tam_pagina;
    int desplazamiento = direccion_logica % tam_pagina;
    int marco;
    
    if(esta_en_cache(nro_pagina, &marco, pcb)){
        pthread_mutex_lock(&mutex_cache);

        t_entrada_cache* entrada = NULL;
        for(int i = 0; i < list_size(cache); i++){
            t_entrada_cache* temp = list_get(cache, i);
            if(temp->pagina == nro_pagina){
                entrada = temp;
                break;
            }
        }
        if(entrada != NULL){
            memcpy(entrada->contenido + desplazamiento, datos, strlen(datos) + 1);
            entrada->modificado = true;
            entrada->usado = true;
        }
        pthread_mutex_unlock(&mutex_cache);
        usleep(retardo_cache * 1000);
        return true;
    } else{

        // cache_miss(nro_pagina, pcb);
        // usleep(retardo_cache * 1000);
        // return true;

        int direccion_fisica = traducir_direccion(pcb, direccion_logica);
        int marco_local = direccion_fisica / tam_pagina;
        char* pagina_completa = leer_pagina_memoria(nro_pagina, pcb);
        
        memcpy(pagina_completa + desplazamiento, datos, strlen(datos) + 1);
        log_debug(logger, "debug escribir en cache, marco: %d", marco_local);
        actualizar_cache(nro_pagina, marco_local, pagina_completa, true, pcb);
        free(pagina_completa);
        usleep(retardo_cache * 1000);
        return true;
    }
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
        int indice_victima = encontrar_victima_cache(pcb);
        
        t_entrada_cache* victima = list_get(cache, indice_victima);
        if (victima->modificado) {
        log_info(logger, "PID: %d - Memory Update - Página: %d - Frame: %d", pcb->pid, victima->pagina, victima->marco);
        }
        
        list_remove(cache, indice_victima);
        free(victima->contenido);
        free(victima);
    }
    
    list_add(cache, nueva_entrada);
    log_info(logger, "PID: %d - Cache Add - Pagina: %d", pcb->pid, pagina);
    return;
}

int encontrar_victima_cache(t_pcb* pcb) {
    pthread_mutex_lock(&mutex_cache_clock);
    int vueltas = 0;

    while (vueltas < 2) {
        for (int i = 0; i < list_size(cache); i++) {
            t_entrada_cache* entrada = list_get(cache, puntero_clock);
            
            log_trace(logger, "Evaluando entrada %d: usado=%d, modificado=%d", puntero_clock, entrada->usado, entrada->modificado);

            if (strcmp(reemplazo_cache, "CLOCK-M") == 0) {
                if (vueltas == 0 && !entrada->usado && !entrada->modificado) {
                    int victima = puntero_clock;
                    puntero_clock = (puntero_clock + 1) % entradas_cache;
                    pthread_mutex_unlock(&mutex_cache_clock);
                    return victima;
                }
                if (vueltas == 1 && !entrada->usado && entrada->modificado) {
                    // Escribir a memoria antes de reemplazar
                    escribir_pagina_en_memoria((entrada->marco * tam_pagina), entrada->contenido, pcb);
                    int victima = puntero_clock;
                    puntero_clock = (puntero_clock + 1) % entradas_cache;
                    pthread_mutex_unlock(&mutex_cache_clock);
                    return victima;
                }
            } else if (strcmp(reemplazo_cache, "CLOCK") == 0) {
                if (!entrada->usado) {
                    int victima = puntero_clock;
                    puntero_clock = (puntero_clock + 1) % entradas_cache;
                    pthread_mutex_unlock(&mutex_cache_clock);
                    return victima;
                }
            }

            entrada->usado = false;
            puntero_clock = (puntero_clock + 1) % entradas_cache;
        }
        vueltas++;
    }

    log_error(logger, "No se encontró víctima, usando la entrada 0 por defecto.");
    pthread_mutex_unlock(&mutex_cache_clock);
    return 0;
}
int pedir_frame(t_pcb* pcb, int nro_pagina){
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OC_FRAME);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    agregar_a_paquete(paquete, &nro_pagina, sizeof(int));
    enviar_paquete(paquete, socket_memoria, logger);
    borrar_paquete(paquete);
    log_trace(logger, "## Se envio el paquete para preguntar frame a memoria");

    int opcode = recibir_operacion(socket_memoria);
    if(opcode != OC_FRAME){
        log_error(logger, "no se recibio el marco");
        return -1;
    }

    t_list* recibido = recibir_paquete(socket_memoria);
    int marco = *((int*)list_get(recibido, 0));

    list_destroy_and_destroy_elements(recibido, free);

    log_info(logger, "PID: %d - OBTENER MARCO - Página: %d - Marco: %d", pcb->pid, nro_pagina, marco);
    return marco;
}