#include <administracion_memoria.h>

t_dictionary* tablas_por_pid;
void* memoria_usuario = NULL;
int cantidad_marcos;
t_bitarray* bitmap_marcos = NULL;

void inicializar_memoria()
{
    memoria_usuario = malloc(campos_config.tam_memoria);
    if(memoria_usuario == NULL)
    {
        log_info(logger, "Error al inicializar memoria");
        exit(EXIT_FAILURE);
    }

    memset(memoria_usuario, 0, campos_config.tam_memoria);

    cantidad_marcos = campos_config.tam_memoria / campos_config.tam_pagina;


    int tam_en_bytes = (cantidad_marcos + 7)/8;

    char* buffer = malloc(tam_en_bytes);
    memset(buffer, 0, tam_en_bytes);

    bitmap_marcos = bitarray_create_with_mode(buffer, tam_en_bytes, LSB_FIRST);


    log_info(logger,"Memoria de usuario inicializada con %d marcos de %db cada uno",cantidad_marcos, campos_config.tam_pagina);
}


// t_tabla_paginas* crear_tabla(int nivel_actual)
// {

//     t_tabla_paginas* tabla = malloc(sizeof(t_tabla_paginas));
//     if (!tabla) {
//         log_error(logger, "Error al asignar memoria para tabla nivel %d", nivel_actual);
//         return NULL;
//     }

//     tabla->entradas = malloc(sizeof(t_entrada_tabla) * campos_config.entradas_por_tabla);
//     if (!tabla->entradas) {
//         free(tabla);
//         log_error(logger, "Error al asignar entradas para tabla nivel %d", nivel_actual);
//         return NULL;
//     }
//     memset(tabla->entradas, 0, sizeof(t_entrada_tabla) * campos_config.entradas_por_tabla);


//     for(int i = 0; i < campos_config.entradas_por_tabla; i++)
//     {
//         tabla->entradas[i].presencia = false;
//         tabla->entradas[i].marco = -1;
//         tabla->entradas[i].siguiente_tabla = NULL;
//         if (nivel_actual < campos_config.cantidad_niveles - 1) {
//             tabla->entradas[i].siguiente_tabla = crear_tabla(nivel_actual + 1);

//             if (!tabla->entradas[i].siguiente_tabla) {
//                 Liberar toda la tabla si falla
//                 liberar_tabla(tabla, nivel_actual);
//                 return NULL;
//             }
//         }
//     }
//     return tabla;
// } 

t_tabla_paginas* crear_tabla(int nivel_actual) {
    t_tabla_paginas* tabla = malloc(sizeof(t_tabla_paginas));
    tabla->entradas = list_create();
    if (!tabla->entradas) {
    log_error(logger, "No se pudo crear la lista de entradas");
    free(tabla);
    return NULL;
    }

    for (int i = 0; i < campos_config.entradas_por_tabla; i++) {
        t_entrada_tabla* entrada = malloc(sizeof(t_entrada_tabla));
        entrada->numero_entrada = i;
        entrada->presencia = false;
        entrada->marco = -1;
        entrada->siguiente_tabla = NULL;
        log_error(logger, "Creando tabla de páginas en nivel %d", nivel_actual);

        if (nivel_actual < campos_config.cantidad_niveles - 1) {
            entrada->siguiente_tabla = crear_tabla(nivel_actual + 1);
        }
        

        list_add(tabla->entradas, entrada);
    }

    return tabla;
}



void* creacion_estructuras_administrativas()
{

    semaforos_por_pid = dictionary_create();
    iniciados_por_pid = dictionary_create();
    pthread_mutex_init(&mutex_semaforos, NULL);

    paginas_en_swap = list_create();
    inicializar_memoria();
    tablas_por_pid = dictionary_create();
    lista_de_instrucciones_por_pid = dictionary_create();
    return NULL;
}

