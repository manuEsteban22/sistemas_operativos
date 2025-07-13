#include <instrucciones.h>
#include <math.h>
#include <atender_kernel.h> // Para t_proceso y el diccionario
#include <time.h>
#define MAX_LINEA 256

static t_list* lista_instrucciones = NULL;

void cargar_instrucciones(int pid) 
{
    char* Archivo = string_from_format("%sinstrucciones_pid%i.txt", campos_config.path_instrucciones, pid);
    FILE* archivo = fopen( Archivo, "r");
    if (!archivo) 
    {
        perror("No se pudo abrir el archivo de instrucciones");
        exit(EXIT_FAILURE);
    }

    lista_instrucciones = list_create();

    char linea[MAX_LINEA];
    while (fgets(linea, sizeof(linea), archivo)) 
    {
        linea[strcspn(linea, "\n")] = 0; // eliminar \n
        list_add(lista_instrucciones, strdup(linea)); // guardar copia
    }

    fclose(archivo);
}

char* obtener_instruccion(int pc) 
{
    if (pc >= list_size(lista_instrucciones)) return NULL;
    return list_get(lista_instrucciones, pc);
}

int cantidad_instrucciones() 
{
    return list_size(lista_instrucciones);
}

int espacio_libre()
{
    const int tam_mem = campos_config.tam_memoria;
    return tam_mem;
}

void liberar_memoria() 
{
    list_destroy_and_destroy_elements(lista_instrucciones, free);
}


int mandar_instruccion(int socket_cliente) 
{

    t_list* lista_paquete = recibir_paquete(socket_cliente);
    int pid = *((int*) list_get(lista_paquete, 0));
    int pc = *((int*) list_get(lista_paquete, 1));

    cargar_instrucciones(pid);

    int cant = cantidad_instrucciones();

    
    t_paquete* paquete = crear_paquete();

    if(pc >= cant)
    {
        log_error(logger, "el pc se paso de la cantidad");
        cambiar_opcode_paquete(paquete, ERROR);
        enviar_paquete(paquete, socket_cliente, logger);
        borrar_paquete(paquete);
        liberar_memoria();
        list_destroy_and_destroy_elements(lista_paquete, free);
        return -1;
    }
    
    char* instruccion = obtener_instruccion(pc);

    if (instruccion == NULL) {
    log_error(logger, "No se encontró la instrucción en la posición PC=%d", pc);
    liberar_memoria();
    list_destroy_and_destroy_elements(lista_paquete, free);
    return -1;
    }

    int tamanio = strlen(instruccion) + 1;
    log_trace(logger, "PID: %d - Obtener instrucción: %d - Instrucción: %s", pid, pc, instruccion);
    agregar_a_paquete(paquete, instruccion, tamanio);
    cambiar_opcode_paquete(paquete, PAQUETE);
    enviar_paquete(paquete, socket_cliente, logger);
    borrar_paquete(paquete);

    printf("Espacio libre: %d\n", espacio_libre());

    liberar_memoria();
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

    void* leido = malloc(tamanio);
    memcpy(leido, memoria_usuario + direccion_fisica, tamanio);

    log_trace(logger, "anda antes de mandar lo leido");
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OC_READ);
    agregar_a_paquete(paquete, leido, tamanio);
    enviar_paquete(paquete, socket_cliente, logger);
    borrar_paquete(paquete);
    free(leido);
    list_destroy_and_destroy_elements(recibido, free);
    return 0;
}

int ejecutar_write(int socket_cliente){
    t_list* recibido = recibir_paquete(socket_cliente);
    int direccion_fisica = *((int*)list_get(recibido, 0));
    char* datos = list_get(recibido, 1);
    int tamanio = strlen(datos) + 1;

    memcpy(memoria_usuario + direccion_fisica, datos, tamanio);

    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OK);
    enviar_paquete(paquete, socket_cliente, logger);
    borrar_paquete(paquete);
    list_destroy_and_destroy_elements(recibido, free);
    return 0;
}

int leer_pagina_completa(int socket_cliente){
    t_list* recibido = recibir_paquete(socket_cliente);
    int direccion_fisica = *((int*)list_get(recibido, 0));

    void* buffer = malloc(campos_config.tam_pagina);
    memcpy(buffer, memoria_usuario + direccion_fisica, campos_config.tam_pagina);

    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OC_READ);
    agregar_a_paquete(paquete, buffer, campos_config.tam_pagina);
    enviar_paquete(paquete, socket_cliente, logger);
    borrar_paquete(paquete);
    free(buffer);
    list_destroy_and_destroy_elements(recibido, free);
    return 0;
}

int escribir_pagina_completa(int socket_cliente){
    t_list* recibido = recibir_paquete(socket_cliente);
    int direccion_fisica = *((int*)list_get(recibido, 0));
    void* datos = list_get(recibido, 1);

    memcpy(memoria_usuario + direccion_fisica, datos, campos_config.tam_pagina);

    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OK);
    enviar_paquete(paquete, socket_cliente, logger);
    borrar_paquete(paquete);
    list_destroy_and_destroy_elements(recibido, free);
    return 0;
}

void dumpear_memoria(int pid){
    char* timestamp = obtener_timestamp(); // Implementá una función para timestamp
    char* path = string_from_format("%s%d-%s.dmp", campos_config.dump_path, pid, timestamp);
    FILE* archivo = fopen(path, "wb");
    if (!archivo) {
        log_error(logger, "No se pudo crear el dump para el proceso %d", pid);
        free(path);
        free(timestamp);
        return;
    }
    char* pid_str = string_itoa(pid);
    t_proceso* proceso = dictionary_get(tablas_por_pid, pid_str);
    free(pid_str);
    for (int i = 0; i < proceso->paginas_usadas; i++) {
        t_entrada_tabla* entrada = buscar_entrada(proceso->tabla_raiz, i);
        if (entrada->presencia) {
            void* origen = memoria_usuario + entrada->marco * campos_config.tam_pagina;
            fwrite(origen, 1, campos_config.tam_pagina, archivo);
        }
    }
    fclose(archivo);
    free(path);
    free(timestamp);
    log_info(logger, "Dump de memoria del proceso %d generado", pid);
}

// Implementación simple de timestamp para el dump
char* obtener_timestamp() {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char* buffer = malloc(20);
    strftime(buffer, 20, "%Y%m%d%H%M%S", tm_info);
    return buffer;
}

// Devuelve el número de marco correspondiente a una página lógica, contando accesos y retardo por nivel
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
        usleep(campos_config.retardo_memoria * 1000); // retardo en microsegundos
        int indice = (nro_pagina >> (bits_por_nivel * (campos_config.cantidad_niveles - nivel - 1))) & ((1 << bits_por_nivel) - 1);
        if (nivel < campos_config.cantidad_niveles - 1) {
            actual = actual->entradas[indice].siguiente_tabla;
        } else {
            marco = actual->entradas[indice].marco;
        }
    }
    return marco;
}