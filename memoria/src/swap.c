#include <swap.h>

t_log* logger;
bool* bitmap_marcos_swap;

void inicializar_swap(){

    FILE* archivo_swap = fopen(campos_config.path_swapfile, "r+b");
    if (!archivo_swap) 
    {
        archivo_swap = fopen(campos_config.path_swapfile, "w+b");
    }

    fseek(archivo_swap, 0, SEEK_END);
    long tamanio_archivo = ftell(archivo_swap);

    campos_config.cantidad_marcos_swap = config_get_int_value(config, "CANTIDAD_MARCOS_SWAP");

    int tamanio_esperado = campos_config.cantidad_marcos_swap * campos_config.tam_pagina;

    if (tamanio_archivo < tamanio_esperado){

        int faltantes = tamanio_esperado - tamanio_archivo;
        int tamanio_bloque = campos_config.tam_pagina;
        char* buffer = calloc(1, tamanio_bloque);
        int bloques_enteros = faltantes / tamanio_bloque;
        int resto_faltantes = faltantes % tamanio_bloque;

        for(int i = 0; i < bloques_enteros; i++) {
            fwrite(buffer, 1, tamanio_bloque, archivo_swap);
        }

        if(resto_faltantes > 0){
            fwrite(buffer, 1, resto_faltantes, archivo_swap);
        }

        free(buffer);
    }
    
    fclose(archivo_swap);

    bitmap_marcos_swap = malloc(sizeof(bool) * campos_config.cantidad_marcos_swap);
    if(!bitmap_marcos_swap){
        log_error(logger, "No se pudo reservar memoria para el bitmap de marcos de swap.");
        abort();
    }

    memset(bitmap_marcos_swap, false, sizeof(bool) * campos_config.cantidad_marcos_swap);

    log_info(logger, "Swap inicializado correctamente con %d marcos de %d bytes", campos_config.cantidad_marcos_swap, campos_config.tam_pagina);
}

// t_list* asignar_marcos_swap(int cantidad){
//     t_list* marcos_libres = list_create();

//     int cantidad_agregada = 0;
//     for(int i = 0; i < campos_config.cantidad_marcos_swap && cantidad_agregada < cantidad; i++){

//         if (!bitmap_marcos_swap[i]){
//             int* marco = malloc(sizeof(int));
//             *marco = i;
//             list_add(marcos_libres, marco);
//             bitmap_marcos_swap[i] = true;
//             cantidad_agregada++;
//         }
//     }

//     if(cantidad_agregada < cantidad){
//         liberar_marcos(marcos_libres);
//         printf("La cantidad de marcos disponibles es menor que la cantidad de marcos solicitada (%d).", cantidad);
//         return NULL;
//     }

//     return marcos_libres;
// }

void liberar_marcos(t_list* marcos){

    for (int i = 0; i < list_size(marcos); i++) {
        int* marco = list_get(marcos, i);
        bitmap_marcos_swap[*marco] = false;
    }

    list_destroy_and_destroy_elements(marcos, free);
}