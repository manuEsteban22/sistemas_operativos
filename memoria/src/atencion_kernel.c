#include <atencion_kernel.h>
#include <math.h>

// busca pags libres, las asigna y arma tabla raiz

void* inicializar_proceso(int tam_proceso, int pid, char* nombre_archivo){
    if (tam_proceso <= 0 || !nombre_archivo) {
        log_error(logger, "Tamaño de proceso inválido: %d", tam_proceso);
        return NULL;
    }

    int pags_necesarias = (tam_proceso + campos_config.tam_pagina - 1) / campos_config.tam_pagina;
    log_trace(logger, "pags necesarias %d", pags_necesarias);

    t_proceso* proceso = malloc(sizeof(t_proceso));
    if (!proceso) return NULL;
    memset(proceso, 0, sizeof(t_proceso));
    proceso->pid = pid;
    proceso->tabla_raiz = crear_tabla(0);
    if (!proceso->tabla_raiz) {
        free(proceso);
        return NULL;
    }
    proceso->paginas_usadas = pags_necesarias;
    proceso->accesos_memoria = 0;
    proceso->page_faults = 0;
    char* pid_str = string_itoa(pid);
    memset(&proceso->metricas, 0, sizeof(t_metricas));
    log_trace(logger, "cargue el pid %s", pid_str);
    dictionary_put(tablas_por_pid, pid_str, proceso);
    free(pid_str);
    cargar_instrucciones(pid, nombre_archivo); 

    for (int pagina = 0; pagina < pags_necesarias; pagina++) 
    {
        int marco_libre = buscar_marco_libre();
        if (marco_libre == -1) 
        {
            log_error(logger, "No hay marcos libres para página %d de PID %d", pagina, pid);
            liberar_proceso(proceso);
            dictionary_remove_and_destroy(tablas_por_pid, pid_str, liberar_proceso);
            
            return NULL;
        }
        bitarray_set_bit(bitmap_marcos, marco_libre);
        t_entrada_tabla* entrada = buscar_entrada(proceso->tabla_raiz, pagina);
        if (!entrada) {
            log_error(logger, "No se pudo inicializar la entrada de la página %d para PID %d", pagina, pid);
            liberar_proceso(proceso);
            dictionary_remove_and_destroy(tablas_por_pid, pid_str, liberar_proceso);
            
            return NULL;
        }

        entrada->presencia = true;
        entrada->marco = marco_libre;
        log_trace(logger, "Página %d asignada a marco %d", pagina, marco_libre);
    }
    log_info(logger, "## PID: %d - Proceso Creado - Tamaño: %d", pid, tam_proceso);
    return proceso->tabla_raiz;
}

void suspender_proceso(int pid){
    char* pid_str = string_itoa(pid);
    t_proceso* proceso = dictionary_get(tablas_por_pid, pid_str);
    free(pid_str);
    if (!proceso){
        log_error(logger, "No se encontró el proceso %d", pid);
        return;
    }
    suspender_tabla(proceso->tabla_raiz, 0, pid, 0);

    proceso->metricas.cantidad_bajadas_a_swap++;
}

void des_suspender_proceso(int pid){
    t_list* paginas_proceso = list_create(); //filtramos las paginas de ese proceso
    for (int i = 0; i < list_size(paginas_en_swap); i++){
        t_pagina_swap* relacion = list_get(paginas_en_swap, i);
        if(relacion->pid == pid){
            list_add(paginas_proceso, relacion);
        }
    }
    if (list_size(paginas_proceso) > cantidad_marcos_libres()){
        log_error(logger, "No hay marcos libres suficientes para des-suspender el proceso %d", pid);
        list_destroy(paginas_proceso);
        return;
    }
    char* pid_str = string_itoa(pid);
    t_proceso* proceso = dictionary_get(tablas_por_pid, pid_str);
    t_tabla_paginas* tabla_raiz = proceso->tabla_raiz;
    for (int i = 0; i < list_size(paginas_proceso); i++) {
        t_pagina_swap* relacion = list_get(paginas_proceso, i);
        int marco_ram = buscar_marco_libre();
        if (marco_ram == -1) {
            log_error(logger, "Error inesperado: no hay marco libre para restaurar página");
            break;
        }
        void* buffer = malloc(campos_config.tam_pagina);
        leer_de_swap(buffer, relacion->marco_swap);
        memcpy(memoria_usuario + marco_ram * campos_config.tam_pagina, buffer, campos_config.tam_pagina);
        t_entrada_tabla* entrada = buscar_entrada(tabla_raiz, relacion->nro_pagina);
        entrada->presencia = true;
        entrada->marco = marco_ram;
        t_list* marcos_swap = list_create();
        int* marco_swap_ptr = malloc(sizeof(int));
        *marco_swap_ptr = relacion->marco_swap;
        list_add(marcos_swap, marco_swap_ptr);
        liberar_marcos(marcos_swap);
        list_remove_element(paginas_en_swap, relacion);
        free(relacion);
        free(buffer);
    }

    proceso->metricas.cantidad_subidas_a_memoria++;

    free(pid_str);
    list_destroy(paginas_proceso);
    log_info(logger, "Proceso %d des-suspendido correctamente", pid);
}

void* finalizar_proceso(int pid){
    char* pid_str = string_itoa(pid);
    t_proceso* proceso = dictionary_get(tablas_por_pid, pid_str);
    if (!proceso){
        log_error(logger, "No se encontró el proceso %d", pid);
        free(pid_str);
        return NULL;
    }
    liberar_tabla(proceso->tabla_raiz, 0);
    for (int i = 0; i < list_size(paginas_en_swap); ){
        t_pagina_swap* relacion = list_get(paginas_en_swap, i);
        if (relacion->pid == pid){
            t_list* marcos_swap = list_create();
            int* marco_swap_ptr = malloc(sizeof(int));
            *marco_swap_ptr = relacion->marco_swap;
            list_add(marcos_swap, marco_swap_ptr);
            liberar_marcos(marcos_swap);
            list_remove(paginas_en_swap, i);
            free(relacion);
        } else {
            i++;
        }
    }
    dictionary_remove(tablas_por_pid, pid_str);
    free(pid_str);
    // Log de métricas
    log_info(logger, "## PID: %d - Proceso Destruido - Métricas - Acc.T.Pag: %d; Inst.Sol.: %d; SWAP: %d; Mem.Prin.: %d; Lec.Mem.: %d; Esc.Mem.: %d", pid, proceso->metricas.cantidad_accesos_tablas_de_paginas, proceso->metricas.cantidad_instrucciones_solicitadas, proceso->metricas.cantidad_bajadas_a_swap, proceso->metricas.cantidad_subidas_a_memoria, proceso->metricas.cantidad_lecturas_memoria, proceso->metricas.cantidad_escrituras_memoria);
    free(proceso); // liberar struct proceso
    return NULL;
}

int buscar_marco_libre() 
{
    int cantidad_marcos = campos_config.tam_memoria / campos_config.tam_pagina;
    for (int i = 0; i < cantidad_marcos; i++) {
        if (!bitarray_test_bit(bitmap_marcos, i)) {
            bitarray_set_bit(bitmap_marcos, i);
            return i;
        }
    }
    return -1;
}

// busca la entrada en cuestion

t_entrada_tabla* buscar_entrada(t_tabla_paginas* tabla_raiz, int nro_pagina) {
    if (!tabla_raiz) {
        log_error(logger, "Tabla raíz NULL para página %d", nro_pagina);
        return NULL;
    }

    // Verificar que entradas_por_tabla sea una potencia de 2
    int entradas_por_tabla = campos_config.entradas_por_tabla;

    t_tabla_paginas* actual = tabla_raiz;
    int bits_por_nivel = (int)log2(entradas_por_tabla);

    for (int nivel = 0; nivel < campos_config.cantidad_niveles - 1; nivel++) {
        int indice = (nro_pagina >> (bits_por_nivel * (campos_config.cantidad_niveles - nivel - 1))) & ((1 << bits_por_nivel) - 1);
        log_trace(logger, "Nivel %d, página %d, índice %d", nivel, nro_pagina, indice);

        if (!actual) {
            log_error(logger, "Tabla actual NULL en nivel %d para página %d", nivel, nro_pagina);
            return NULL;
        }
        if (indice >= campos_config.entradas_por_tabla) {
            log_error(logger, "Índice %d fuera de rango en nivel %d para página %d", indice, nivel, nro_pagina);
            return NULL;
        }
        if (!actual->entradas[indice].siguiente_tabla) {
            log_error(logger, "Siguiente tabla no existe en nivel %d, índice %d para página %d", nivel, indice, nro_pagina);
            return NULL;
        }
        actual = actual->entradas[indice].siguiente_tabla;
    }

    int indice_final = nro_pagina & ((1 << bits_por_nivel) - 1);
    if (indice_final >= campos_config.entradas_por_tabla) {
        log_error(logger, "Índice final %d inválido para página %d", indice_final, nro_pagina);
        return NULL;
    }

    log_trace(logger, "Entrada encontrada para página %d, índice final %d", nro_pagina, indice_final);
    return &actual->entradas[indice_final];
}

void suspender_tabla(t_tabla_paginas* tabla, int nivel, int pid, int pagina_base){

    for (int i = 0; i < campos_config.entradas_por_tabla; i++){
        t_entrada_tabla* entrada = &tabla->entradas[i];
        int salto = (int)pow(campos_config.entradas_por_tabla, campos_config.cantidad_niveles - nivel - 1);
        int nro_pagina = pagina_base + i * salto;

        if (nivel < campos_config.cantidad_niveles - 1){
            if (entrada->siguiente_tabla)
                suspender_tabla(entrada->siguiente_tabla, nivel + 1, pid, nro_pagina);
            
        } else {
            if (entrada->presencia){
                suspender_pagina(pid, nro_pagina, entrada->marco);
                bitarray_clean_bit(bitmap_marcos, entrada->marco);
                entrada->presencia = false;
            }
        }
    }
}

void suspender_pagina(int pid, int nro_pagina, int marco){

    t_list* marcos_swap = asignar_marcos_swap(1); //pide marco libre

    if (!marcos_swap){
        log_error(logger, "No hay marcos libres en swap para suspender la pagina %d del proceso %d", nro_pagina, pid);
        return;
    }

    int marco_swap = *(int*)list_get(marcos_swap, 0);

    void* buffer = malloc(campos_config.tam_pagina); //copia el contenido de las paginas
    memcpy(buffer, memoria_usuario + marco * campos_config.tam_pagina, campos_config.tam_pagina);

    escribir_en_swap(buffer, marco_swap); //lo escribe en el swap

    t_pagina_swap* relacion = malloc(sizeof(t_pagina_swap)); //guarda la relacion para cuando tenga que restaurarla
    relacion->pid = pid;
    relacion->nro_pagina = nro_pagina;
    relacion->marco_swap = marco_swap;
    list_add(paginas_en_swap, relacion);

    free(buffer); //libera
    liberar_marcos(marcos_swap);
}

int cantidad_marcos_libres()
{
    int marcos_libres = 0;
    int cantidad_marcos = campos_config.tam_memoria / campos_config.tam_pagina;
    
    for (int i = 0; i < cantidad_marcos; i++) {
        if (!bitarray_test_bit(bitmap_marcos, i)) {
            marcos_libres ++;
        }
    }
    return marcos_libres;
}

bool entra_el_proceso(int tamanio){
    int marcos_que_ocupa = tamanio / campos_config.tam_pagina;
    int marcos_libres = cantidad_marcos_libres();
    if(marcos_que_ocupa <= marcos_libres){
        return true;
    }
    else{
        return false;
    }
}

void liberar_tabla(t_tabla_paginas* tabla, int nivel) {
    if (!tabla) {
        return;
    }

    for (int i = 0; i < campos_config.entradas_por_tabla; i++) {
        t_entrada_tabla* entrada = &tabla->entradas[i];
        if (nivel < campos_config.cantidad_niveles - 1) {
            if (entrada->siguiente_tabla) {
                liberar_tabla(entrada->siguiente_tabla, nivel + 1);
                entrada->siguiente_tabla = NULL;
            }
        } else if (entrada->presencia) {
            bitarray_clean_bit(bitmap_marcos, entrada->marco);
            entrada->presencia = false;
            entrada->marco = -1;
        }
    }

    free(tabla->entradas);
    tabla->entradas = NULL;
    log_trace(logger, "Liberando tabla de nivel %d", nivel);
    free(tabla);
}

void liberar_proceso(void* proceso_void) {
    t_proceso* proceso = (t_proceso*)proceso_void;
    if (!proceso) {
        log_error(logger, "Intento de liberar un proceso NULL");
        return;
    }

    if (proceso->tabla_raiz) {
        liberar_tabla(proceso->tabla_raiz, 0);
        proceso->tabla_raiz = NULL;
    }

    char* pid_str = string_itoa(proceso->pid);
    t_list* instrucciones = dictionary_remove(lista_de_instrucciones_por_pid, pid_str);
    if (instrucciones) {
        list_destroy_and_destroy_elements(instrucciones, free);
    }
    free(pid_str);

    log_trace(logger, "Liberando proceso PID %d", proceso->pid);
    free(proceso);
}