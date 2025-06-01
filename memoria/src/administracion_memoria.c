
#include <administracion_memoria.h>


void* memoria_usuario = NULL;
int cantidad_marcos;

bool* bitmap_marcos = NULL;

void inicializar_memoria()
{
    memoria_usuario = malloc(campos_config.tam_memoria);
    if(memoria_usuario == NULL)
    {
        log_info(logger, "Error al inicializar memoria");
        exit(EXIT_FAILURE);
    }

    cantidad_marcos = campos_config.tam_memoria / campos_config.tam_pagina;

    log_info(logger,"%d",cantidad_marcos);
}