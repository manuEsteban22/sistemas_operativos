#include <atencion_kernel.h>
#include <math.h>

t_metricas inicializar_metricas()
{
    t_metricas metricas;
    metricas.cantidad_accesos_tablas_de_paginas = 0;
    metricas.cantidad_instrucciones_solicitadas = 0;
    metricas.cantidad_bajadas_a_swap = 0;
    metricas.cantidad_subidas_a_memoria = 0;
    metricas.cantidad_lecturas_memoria = 0;
    metricas.cantidad_escrituras_memoria = 0;
    return metricas;
}

void* inicializar_proceso(int tam_proceso, int pid, char* nombre_archivo) {
    int pags_necesarias = (tam_proceso + campos_config.tam_pagina - 1) / campos_config.tam_pagina;  //para redonder  (ej: 8 : 5 = 1,6  (1 entero) necesitamos 2, entonces (8+4) : 5 = 2,3 (2 entero))

    log_trace(logger, "pags necesarias %d", pags_necesarias);

    t_proceso* proceso = calloc(1, sizeof(t_proceso));
    pthread_mutex_init(&proceso->mutex_tabla, NULL);
    proceso->pid = pid;
    proceso->paginas_usadas = pags_necesarias;

    proceso->tabla_raiz = crear_tabla(0);

    char* pid_str = string_itoa(pid);
    

    cargar_instrucciones(pid, nombre_archivo); //mete el set de instrucciones a una lista que mete a un diccionario de listas de sets

    for (int pagina = 0; pagina < pags_necesarias; pagina++) 
    {
        int marco_libre = buscar_marco_libre();

        
        pthread_mutex_lock(&proceso->mutex_tabla);
        t_entrada_tabla* entrada = buscar_entrada(proceso->tabla_raiz, pagina, 0);  // va buscando ordenadamente entre las entradas, como recorriendo un arbol binario
        

        if (!entrada->presencia) 
        {
            log_trace(logger, "Página %d no está en memoria principal", pagina);
        }

        pthread_mutex_lock(&mutex_bitmap);
        bitarray_set_bit(bitmap_marcos, marco_libre);
        pthread_mutex_unlock(&mutex_bitmap);
        entrada->presencia = true;
        entrada->marco = marco_libre;
        pthread_mutex_unlock(&proceso->mutex_tabla);

        log_trace(logger, "Página %d asignada a marco %d", pagina, marco_libre);
    }

    log_info(logger, "## PID: %d - Proceso Creado - Tamaño: %d", pid, tam_proceso);
    
    proceso->metricas = inicializar_metricas();  // para que no esten en null
    
    pthread_mutex_lock(&mutex_diccionario_procesos);
    dictionary_put(tablas_por_pid, pid_str, proceso);
    pthread_mutex_unlock(&mutex_diccionario_procesos);

    free(pid_str);
    return proceso->tabla_raiz;
}

void suspender_proceso(int pid)
{
    char* pid_str = string_itoa(pid);

    pthread_mutex_lock(&mutex_diccionario_procesos);
    t_proceso* proceso = dictionary_get(tablas_por_pid, pid_str);
    pthread_mutex_unlock(&mutex_diccionario_procesos);

    free(pid_str);
    if (!proceso)
    {
        log_error(logger, "No se encontró el proceso %d", pid);
        return;
    }
    suspender_tabla(proceso->tabla_raiz, 0, pid, 0); //suspende tabla que suspende pags y las escribe en swap

    proceso->metricas.cantidad_bajadas_a_swap++;
}

void des_suspender_proceso(int pid) 
{
    // Filtrar páginas de ese proceso
    t_list* paginas_proceso = list_create();
    for (int i = 0; i < list_size(paginas_en_swap); i++) 
    {
        t_pagina_swap* relacion = list_get(paginas_en_swap, i);
        if (relacion->pid == pid) 
        {
            list_add(paginas_proceso, relacion);
        }
    }

    // Verificar si hay marcos libres en RAM
    if (list_size(paginas_proceso) > cantidad_marcos_libres()) 
    {
        log_error(logger, "No hay marcos libres suficientes para des-suspender el proceso %d", pid);
        list_destroy(paginas_proceso);
        return;
    }

    char* pid_str = string_itoa(pid);

    pthread_mutex_lock(&mutex_diccionario_procesos);
    t_proceso* proceso = dictionary_get(tablas_por_pid, pid_str);
    pthread_mutex_unlock(&mutex_diccionario_procesos);

    for (int i = 0; i < list_size(paginas_proceso); i++) 
    {
        t_pagina_swap* relacion = list_get(paginas_proceso, i);
        int marco_ram = buscar_marco_libre();
        if (marco_ram == -1) 
        {
            log_error(logger, "Error inesperado: no hay marco libre para restaurar página");
            break;
        }

        void* buffer = malloc(campos_config.tam_pagina);

        pthread_mutex_lock(&mutex_swap);
        leer_de_swap(buffer, relacion->marco_swap);
        pthread_mutex_unlock(&mutex_swap);

        pthread_mutex_lock(&mutex_memoria);
        memcpy(memoria_usuario + marco_ram * campos_config.tam_pagina, buffer, campos_config.tam_pagina);
        pthread_mutex_unlock(&mutex_memoria);
 
        pthread_mutex_lock(&proceso->mutex_tabla);
        t_entrada_tabla* entrada = buscar_entrada(proceso->tabla_raiz, relacion->nro_pagina, 0);
        entrada->presencia = true;
        entrada->marco = marco_ram;
        pthread_mutex_unlock(&proceso->mutex_tabla);

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

void* finalizar_proceso(int pid) 
{
    char* pid_str = string_itoa(pid);
    pthread_mutex_lock(&mutex_diccionario_procesos);
    t_proceso* proceso = dictionary_get(tablas_por_pid, pid_str);
    pthread_mutex_unlock(&mutex_diccionario_procesos);

    if (!proceso) 
    {
        log_error(logger, "No se encontró el proceso %d", pid);
        free(pid_str);
        return NULL;
    }

    if (proceso->tabla_raiz) 
    {
        liberar_tabla(proceso->tabla_raiz, 0);
        proceso->tabla_raiz = NULL;
    }

    for (int i = 0; i < list_size(paginas_en_swap); ) 
    {
        t_pagina_swap* relacion = list_get(paginas_en_swap, i);
        if (relacion->pid == pid) 
        {
            int* marco_swap_ptr = malloc(sizeof(int));
            *marco_swap_ptr = relacion->marco_swap;

            t_list* marcos_swap = list_create();
            list_add(marcos_swap, marco_swap_ptr);
            liberar_marcos(marcos_swap);

            list_remove(paginas_en_swap, i);
            free(relacion);
        } else {
            i++;
        }
    }

    pthread_mutex_lock(&mutex_diccionario_procesos);
    dictionary_remove(tablas_por_pid, pid_str);
    pthread_mutex_unlock(&mutex_diccionario_procesos);

    // Log de métricas
    log_info(logger, "## PID: %d - Proceso Destruido - Métricas - Acc.T.Pag: %d; Inst.Sol.: %d; SWAP: %d; Mem.Prin.: %d; Lec.Mem.: %d; Esc.Mem.: %d",
        pid,
        proceso->metricas.cantidad_accesos_tablas_de_paginas,
        proceso->metricas.cantidad_instrucciones_solicitadas,
        proceso->metricas.cantidad_bajadas_a_swap,
        proceso->metricas.cantidad_subidas_a_memoria,
        proceso->metricas.cantidad_lecturas_memoria,
        proceso->metricas.cantidad_escrituras_memoria
    );

    free(proceso);
    free(pid_str);
    return NULL;
}

int buscar_marco_libre() 
{
    int cantidad_marcos = campos_config.tam_memoria / campos_config.tam_pagina;
    
    for (int i = 0; i < cantidad_marcos; i++) 
    {
        pthread_mutex_lock(&mutex_bitmap);
        if (!bitarray_test_bit(bitmap_marcos, i)) 
        {
            bitarray_set_bit(bitmap_marcos, i);
            pthread_mutex_unlock(&mutex_bitmap);
            return i;
        }
        pthread_mutex_unlock(&mutex_bitmap);
    }
    return -1;
}

// busca la entrada en cuestion


t_entrada_tabla* buscar_entrada(t_tabla_paginas* tabla, int pagina, int nivel_actual) 
{
    int bits_por_nivel = log2(campos_config.entradas_por_tabla);
    int entrada_actual = (pagina >> (bits_por_nivel * (campos_config.cantidad_niveles - nivel_actual - 1))) & ((1 << bits_por_nivel) - 1);

    t_entrada_tabla* entrada = list_get(tabla->entradas, entrada_actual);

    if (!entrada) 
    {
        log_error(logger, "Entrada inválida en nivel %d (entrada %d)", nivel_actual, entrada_actual);
        return NULL;
    }
    
    if (nivel_actual == campos_config.cantidad_niveles - 1) 
    {
        
        if(!entrada->presencia)
        {
            log_debug(logger, "false");
        }
        else if(entrada->presencia)
        {
            log_debug(logger, "true");
        }else
        {
            log_debug(logger, "rari");
        }
        return entrada;
    }

    if (!entrada->siguiente_tabla) 
    {
        log_error(logger, "Entrada sin siguiente tabla en nivel %d (entrada %d)", nivel_actual, entrada_actual);
        return NULL;
    }

    return buscar_entrada(entrada->siguiente_tabla, pagina, nivel_actual + 1);
}

void suspender_tabla(t_tabla_paginas* tabla, int nivel, int pid, int pagina_base) 
{
    int cant_entradas = list_size(tabla->entradas);

    for (int i = 0; i < cant_entradas; i++) 
    {
        t_entrada_tabla* entrada = list_get(tabla->entradas, i);
        int salto = (int)pow(campos_config.entradas_por_tabla, campos_config.cantidad_niveles - nivel - 1);
        int nro_pagina = pagina_base + i * salto;

        if (nivel < campos_config.cantidad_niveles - 1) 
        {
            if (entrada->siguiente_tabla)
            {
                suspender_tabla(entrada->siguiente_tabla, nivel + 1, pid, nro_pagina);
            }
        } else {
            if (entrada->presencia) 
            {
                suspender_pagina(pid, nro_pagina, entrada->marco);
                
                pthread_mutex_lock(&mutex_bitmap);
                bitarray_clean_bit(bitmap_marcos, entrada->marco);
                pthread_mutex_unlock(&mutex_bitmap);

                entrada->presencia = false;
            }
        }
    }
}

void suspender_pagina(int pid, int nro_pagina, int marco) 
{
    t_list* marcos_swap = asignar_marcos_swap(1);
    if (!marcos_swap) 
    {
        log_error(logger, "No hay marcos libres en swap para suspender la pagina %d del proceso %d", nro_pagina, pid);
        return;
    }

    int marco_swap = *(int*)list_get(marcos_swap, 0);

    void* buffer = malloc(campos_config.tam_pagina);

    memcpy(buffer, memoria_usuario + marco * campos_config.tam_pagina, campos_config.tam_pagina);

    escribir_en_swap(buffer, marco_swap);

    t_pagina_swap* relacion = malloc(sizeof(t_pagina_swap));
    relacion->pid = pid;
    relacion->nro_pagina = nro_pagina;
    relacion->marco_swap = marco_swap;
    list_add(paginas_en_swap, relacion);

    free(buffer);
    list_destroy_and_destroy_elements(marcos_swap, free);
}

int cantidad_marcos_libres()
{
    int marcos_libres = 0;
    int cantidad_marcos = campos_config.tam_memoria / campos_config.tam_pagina;
    
    for (int i = 0; i < cantidad_marcos; i++) {
        pthread_mutex_lock(&mutex_bitmap);
        if (!bitarray_test_bit(bitmap_marcos, i)) {
            marcos_libres ++;
        }
        pthread_mutex_unlock(&mutex_bitmap);
    }
    return marcos_libres;
}

bool entra_el_proceso(int tamanio)
{
    int marcos_que_ocupa = tamanio / campos_config.tam_pagina;
    int marcos_libres = cantidad_marcos_libres();
    if(marcos_que_ocupa <= marcos_libres){
        return true;
    }
    else{
        return false;
    }
}


void liberar_tabla(t_tabla_paginas* tabla, int nivel_actual)
{
    for (int i = 0; i < list_size(tabla->entradas); i++) {
        t_entrada_tabla* entrada = list_get(tabla->entradas, i);
        if (entrada->siguiente_tabla && nivel_actual < campos_config.cantidad_niveles - 1) {
            liberar_tabla(entrada->siguiente_tabla, nivel_actual + 1);
        }
        free(entrada);
    }
    list_destroy(tabla->entradas);
    free(tabla);
}

void liberar_proceso(void* proceso_void) 
{
    t_proceso* proceso = (t_proceso*)proceso_void;
    if (!proceso) 
    {
        log_error(logger, "Intento de liberar un proceso NULL");
        return;
    }

    if (proceso->tabla_raiz) 
    {
        liberar_tabla(proceso->tabla_raiz, 0);
        proceso->tabla_raiz = NULL;
    }

    char* pid_str = string_itoa(proceso->pid);

    pthread_mutex_lock(&mutex_diccionario_instrucciones);
    t_list* instrucciones = dictionary_remove(lista_de_instrucciones_por_pid, pid_str);
    pthread_mutex_unlock(&mutex_diccionario_instrucciones);

    if (instrucciones)
    {
        list_destroy_and_destroy_elements(instrucciones, free);
    }

    pthread_mutex_lock(&mutex_diccionario_procesos);
    dictionary_remove(tablas_por_pid, pid_str);  
    pthread_mutex_unlock(&mutex_diccionario_procesos);

    free(pid_str);

    log_trace(logger, "Liberando proceso PID %d", proceso->pid);
    free(proceso);
}