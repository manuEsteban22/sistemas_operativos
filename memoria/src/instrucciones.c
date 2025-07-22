#include <instrucciones.h>
#include <math.h>
#include <time.h>
#define MAX_LINEA 256

//static t_list* lista_instrucciones = NULL;
t_dictionary* lista_de_instrucciones_por_pid = NULL;

void cargar_instrucciones(int pid, char* nombre_archivo) 
{
    char* Archivo = string_from_format("%s%s", campos_config.path_instrucciones, nombre_archivo);
    log_debug(logger, "El Archivo queda (%s)", Archivo);
    FILE* archivo = fopen( Archivo, "r");
    if (!archivo) 
    {
        perror("No se pudo abrir el archivo de instrucciones");
        exit(EXIT_FAILURE);
    }

    t_list* lista_instrucciones = list_create();

    char linea[MAX_LINEA];
    while (fgets(linea, sizeof(linea), archivo)) 
    {
        linea[strcspn(linea, "\n")] = 0; // eliminar \n
        list_add(lista_instrucciones, strdup(linea)); // guardar copia
    }

    fclose(archivo);
    dictionary_put(lista_de_instrucciones_por_pid, string_itoa(pid), lista_instrucciones);
    log_debug(logger, "Agregue una lista de instrucciones al diccionario PID %d", pid);
}

char* obtener_instruccion(int pc, char* pid) 
{
    if (pc >= list_size(dictionary_get(lista_de_instrucciones_por_pid, pid))) return NULL;
    return list_get(dictionary_get(lista_de_instrucciones_por_pid, pid), pc);
}


int cantidad_instrucciones(char* pid) {
    t_list* instrucciones = dictionary_get(lista_de_instrucciones_por_pid, pid);
    if (instrucciones == NULL) {
        log_error(logger, "La lista de instrucciones es NULL en cantidad_instrucciones para PID %s", pid);
        return 0;
    }
    return list_size(instrucciones);
}



int espacio_libre()
{
    const int tam_mem = campos_config.tam_memoria;//no sive de nada esto
    return tam_mem;
}

void liberar_memoria(char* pid) 
{
    log_warning(logger, "Liberar memoria Pid %s", pid);
    list_destroy_and_destroy_elements(dictionary_get(lista_de_instrucciones_por_pid, pid), free);
}


int mandar_instruccion(int socket_cliente) 
{

    t_list* lista_paquete = recibir_paquete(socket_cliente);
    int pid = *((int*)list_get(lista_paquete, 0));
    int pc = *((int*) list_get(lista_paquete, 1));
    char* pid_str = string_itoa(pid);
    bool iniciado = false;

    pthread_mutex_lock(&mutex_semaforos);
    sem_t* sem = dictionary_get(semaforos_por_pid, pid_str);
    iniciado = dictionary_get(iniciados_por_pid, pid_str);
    pthread_mutex_unlock(&mutex_semaforos);

    while (sem == NULL) {
        usleep(1000); 
        pthread_mutex_lock(&mutex_semaforos);
        sem = dictionary_get(semaforos_por_pid, pid_str);
        iniciado = dictionary_get(iniciados_por_pid, pid_str);
        pthread_mutex_unlock(&mutex_semaforos);
    }

    if (!iniciado){
        sem_wait(sem);

    }else{
        log_trace(logger, "PID %d ya fue iniciado", pid);
    }
    
    
    int cant = cantidad_instrucciones(pid_str);

    
    t_paquete* paquete = crear_paquete();

    if(pc >= cant)
    {
        log_error(logger, "El pc se paso de la cantidad - PC: %d - CANT: %d", pc, cant);
        cambiar_opcode_paquete(paquete, ERROR);
        enviar_paquete(paquete, socket_cliente, logger);
        borrar_paquete(paquete);
        liberar_memoria(pid_str);
        list_destroy_and_destroy_elements(lista_paquete, free);
        free(pid_str);
        return -1;
    }
    char* instruccion = obtener_instruccion(pc, pid_str);

    if (instruccion == NULL) {
    log_error(logger, "No se encontró la instrucción en la posición PC=%d", pc);
    liberar_memoria(pid_str);
    list_destroy_and_destroy_elements(lista_paquete, free);
    free(pid_str);
    return -1;
    }

    int tamanio = strlen(instruccion) + 1;
    log_info(logger, "## PID: %s - Obtener instrucción: %d - Instrucción: %s", pid_str, pc, instruccion);
    agregar_a_paquete(paquete, instruccion, tamanio);
    cambiar_opcode_paquete(paquete, PAQUETE);
    enviar_paquete(paquete, socket_cliente, logger);

    t_proceso* proceso = dictionary_get(tablas_por_pid, pid_str); // yrsteajteazjntguesijk sxr4
    log_error(logger, "alguna metrica es %d", proceso->metricas.cantidad_instrucciones_solicitadas);
    proceso->metricas.cantidad_instrucciones_solicitadas++;

    borrar_paquete(paquete);

    //log_info(logger, "Espacio libre: %d\n", espacio_libre());

    list_destroy_and_destroy_elements(lista_paquete, free); 
    return 0;
}

int mandar_frame(int socket_cliente){//recibo nro_pagina y pid y le mando el frame
    t_list* lista_paquete = recibir_paquete(socket_cliente);

    int pid = *((int*) list_get(lista_paquete, 0));
    int nro_pagina = *((int*) list_get(lista_paquete, 1));

    int marco = obtener_marco(pid, nro_pagina);
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OC_FRAME);
    agregar_a_paquete(paquete,&marco,sizeof(int));
    enviar_paquete(paquete, socket_cliente, logger);
    borrar_paquete(paquete);
    
    return 0;
}

int ejecutar_read(int socket_cliente){
    t_list* recibido = recibir_paquete(socket_cliente);
    int direccion_fisica = *((int*)list_get(recibido, 0));
    int tamanio = *((int*)list_get(recibido, 1));
    int pid = *((int*)list_get(recibido, 2));
    char* pid_str = string_itoa(pid);

    void* leido = malloc(tamanio);
    memcpy(leido, memoria_usuario + direccion_fisica, tamanio);

    log_info(logger, "## PID: %d - Lectura - Dir. Física: %d - Tamaño: %d",pid, direccion_fisica, tamanio);
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OC_READ);
    agregar_a_paquete(paquete, leido, tamanio);
    enviar_paquete(paquete, socket_cliente, logger);

    t_proceso* proceso = dictionary_get(tablas_por_pid, pid_str); //cut6dseyrasjiytr46azjtr4ea6
    proceso->metricas.cantidad_lecturas_memoria++;

    free(pid_str);

    borrar_paquete(paquete);
    free(leido);
    list_destroy_and_destroy_elements(recibido, free);
    return 0;
}

int ejecutar_write(int socket_cliente){
    t_list* recibido = recibir_paquete(socket_cliente);
    int direccion_fisica = *((int*)list_get(recibido, 0));
    char* datos = list_get(recibido, 1);
    int pid = *((int*)list_get(recibido, 2));
    int tamanio = strlen(datos) + 1;
    char* pid_str = string_itoa(pid);

    memcpy(memoria_usuario + direccion_fisica, datos, tamanio);

    t_proceso* proceso = dictionary_get(tablas_por_pid, pid_str); // aaaaaaaaaaaaaaaaaaaaaaaaaakhfysxky
    proceso->metricas.cantidad_escrituras_memoria++;

    free(pid_str);

    log_info(logger, "## PID: %d - Escritura - Dir. Física: %d - Tamaño: %d",pid, direccion_fisica, tamanio);
    
    list_destroy_and_destroy_elements(recibido, free);
    return 0;
}

int leer_pagina_completa(int socket_cliente){
    t_list* recibido = recibir_paquete(socket_cliente);
    int pid = *((int*)list_get(recibido, 0));
    int direccion_fisica = *((int*)list_get(recibido, 1));
    log_debug(logger, "PID %d - Dir Fisica %d", pid, direccion_fisica);
    char* pid_str = string_itoa(pid);

    char* buffer = malloc(campos_config.tam_pagina);
    memcpy(buffer, memoria_usuario + direccion_fisica, campos_config.tam_pagina);
    log_debug(logger, "Contenido de la pagina : %s", buffer);

    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OC_PAG_READ);
    agregar_a_paquete(paquete, buffer, campos_config.tam_pagina);
    enviar_paquete(paquete, socket_cliente, logger);

    t_proceso* proceso = dictionary_get(tablas_por_pid, pid_str); //cut6dseyrasjiytr46azjtr4ea6
    proceso->metricas.cantidad_lecturas_memoria++;

    free(pid_str);
    borrar_paquete(paquete);
    free(buffer);
    list_destroy_and_destroy_elements(recibido, free);
    return 0;
}

int escribir_pagina_completa(int socket_cliente){
    t_list* recibido = recibir_paquete(socket_cliente);
    int pid = *((int*)list_get(recibido, 0));
    int direccion_fisica = *((int*)list_get(recibido, 1));
    char* datos = list_get(recibido, 2);
    char* pid_str = string_itoa(pid);

    log_debug(logger, "PID: %d Direccion fisica: %d", pid, direccion_fisica);
    log_debug(logger, "Los datos a escribir en la pagina son %s", datos);
    memcpy(memoria_usuario + direccion_fisica, datos, campos_config.tam_pagina);

    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OK);
    enviar_paquete(paquete, socket_cliente, logger);

    t_proceso* proceso = dictionary_get(tablas_por_pid, pid_str); // aaaaaaaaaaaaaaaaaaaaaaaaaakhfysxky
    proceso->metricas.cantidad_escrituras_memoria++;

    free(pid_str);
    borrar_paquete(paquete);
    list_destroy_and_destroy_elements(recibido, free);
    return 0;
}

void dumpear_memoria(int pid){
    char* timestamp = obtener_timestamp();
    char* path = string_from_format("%s%d-%s.dmp", campos_config.dump_path, pid, timestamp);
    
    FILE* archivo = fopen(path, "w");
    if (!archivo) {
        log_error(logger, "No se pudo crear el dump para el proceso %d", pid);
        free(timestamp);
        free(path);
        return;
    }

    char* pid_str = string_itoa(pid);
    t_proceso* proceso = dictionary_get(tablas_por_pid, pid_str);
    t_proceso* copia = malloc(sizeof(t_proceso));
    copia = proceso;
    free(pid_str);

    if(!copia){
        log_error(logger, "No se encontró el proceso %d", pid);
        fclose(archivo);
        free(timestamp);
        free(path);
        return;
    }

    int marcos_totales = campos_config.tam_memoria / campos_config.tam_pagina;

    for (int pagina = 0; pagina < copia->paginas_usadas; pagina++) {
        t_entrada_tabla* entrada = buscar_entrada(copia->tabla_raiz, pagina, 0);
        if(!entrada){
            log_error(logger, "Entrada NULL para pagina %d", pagina);
            
        }

        if (!entrada->presencia) {
            log_trace(logger, "Página %d no está en memoria principal", pagina);
            
        }

        if (entrada->marco < 0 || entrada->marco >= marcos_totales) {
            log_error(logger, "Marco inválido para página %d: %d", pagina, entrada->marco);
            
        }

        void* origen = memoria_usuario + ((size_t)entrada->marco * campos_config.tam_pagina);
        log_trace(logger, "Dump página %d: marco %d @ %p", pagina, entrada->marco, origen);
        fwrite(origen, 1, campos_config.tam_pagina, archivo);
    }

    fclose(archivo);
    free(path);
    free(timestamp);
    log_info(logger, "Dump de memoria del proceso %d generado", pid);
}

char* obtener_timestamp() {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char* buffer = malloc(20);
    strftime(buffer, 20, "%Y%m%d%H%M%S", tm_info);
    return buffer;
}

int obtener_marco(int pid, int nro_pagina) {
    char* pid_str = string_itoa(pid);
    t_proceso* proceso = dictionary_get(tablas_por_pid, pid_str);
    free(pid_str);

    if (!proceso) {
        log_error(logger, "No se encontró el proceso %d", pid);
        return -1;
    }

    t_tabla_paginas* actual = proceso->tabla_raiz;
    int bits_por_nivel = log2(campos_config.entradas_por_tabla);
    int marco = -1;

    for (int nivel = 0; nivel < campos_config.cantidad_niveles; nivel++) {
        proceso->metricas.cantidad_accesos_tablas_de_paginas++;
        usleep(campos_config.retardo_memoria * 1000);

        int indice = (nro_pagina >> (bits_por_nivel * (campos_config.cantidad_niveles - nivel - 1))) & ((1 << bits_por_nivel) - 1);
        t_entrada_tabla* entrada = list_get(actual->entradas, indice);

        if (!entrada) {
            log_error(logger, "No se encontró la entrada %d en el nivel %d", indice, nivel);
            return -1;
        }

        if (nivel < campos_config.cantidad_niveles - 1) {
            actual = entrada->siguiente_tabla;
        } else {
            marco = entrada->marco;
        }
    }

    log_trace(logger, "Marco = %d", marco);
    return marco;
}