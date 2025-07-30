#include <swap.h>
#include <memoria.h>

//bool* bitmap_marcos_swap;
t_bitarray* bitmap_marcos_swap = NULL;
t_list* paginas_en_swap = NULL;

void inicializar_swap(){

    FILE* archivo_swap = fopen(campos_config.path_swapfile, "r+b");
    if (!archivo_swap) 
    {
        archivo_swap = fopen(campos_config.path_swapfile, "w+b");
    }

    fseek(archivo_swap, 0, SEEK_END);
    long tamanio_archivo = ftell(archivo_swap);

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

    int tam_en_bytes = (cantidad_marcos_swap + 7)/8;

    char* buffer = malloc(tam_en_bytes);
    
    memset(buffer, 0, tam_en_bytes);

    bitmap_marcos_swap = bitarray_create_with_mode(buffer, tam_en_bytes, LSB_FIRST);

    log_info(logger, "Swap inicializado correctamente con %d marcos de %d bytes", campos_config.cantidad_marcos_swap, campos_config.tam_pagina);
}

t_list* asignar_marcos_swap(int cantidad){
    t_list* marcos_libres = list_create();

    int cantidad_agregada = 0;
    for(int i = 0; i < campos_config.cantidad_marcos_swap && cantidad_agregada < cantidad; i++){

        if (!bitmap_marcos_swap[i]){
            int* marco = malloc(sizeof(int));
            *marco = i;
            list_add(marcos_libres, marco);
            bitmap_marcos_swap[i] = true;
            cantidad_agregada++;
        }
    }

    if(cantidad_agregada < cantidad){
        liberar_marcos(marcos_libres);
        printf("La cantidad de marcos disponibles es menor que la cantidad de marcos solicitada (%d).", cantidad);
        return NULL;
    }

    if (cantidad_agregada == cantidad){
        log_info(logger, "Se asignaron %d marcos.", cantidad);
    }

    return marcos_libres;
}

void escribir_en_swap(void* buffer, int nro_marco){

    FILE* archivo_swap = fopen(campos_config.path_swapfile, "r+b");
    if (!archivo_swap){
        log_error(logger, "No se pudo abrir el archivo del swap.");
        abort();
    }

    int posicion = nro_marco * campos_config.tam_pagina;
    fseek(archivo_swap, posicion, SEEK_SET);
    fwrite(buffer, 1, campos_config.tam_pagina, archivo_swap);

 

    fclose(archivo_swap);
    log_info(logger, "Se escribió una página en el marco %d del swap.", nro_marco);
}

void leer_de_swap(void* buffer, int nro_marco){

    FILE* archivo_swap = fopen(campos_config.path_swapfile, "rb");
    if (!archivo_swap){
        log_error(logger, "No se pudo abrir el archivo del swap.");
        abort();
    }

    int posicion = nro_marco * campos_config.tam_pagina;
    fseek(archivo_swap, posicion, SEEK_SET);
    fread(buffer, 1, campos_config.tam_pagina, archivo_swap);
    fclose(archivo_swap);
}

void liberar_marcos(t_list* marcos){

    for (int i = 0; i < list_size(marcos); i++) {
        int* marco = list_get(marcos, i);
        bitmap_marcos_swap[*marco] = false;
    }

    list_destroy_and_destroy_elements(marcos, free);
}