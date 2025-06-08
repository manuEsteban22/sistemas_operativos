
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

    memset(memoria_usuario, 0, campos_config.tam_memoria);

    cantidad_marcos = campos_config.tam_memoria / campos_config.tam_pagina;

    bitmap_marcos = malloc(sizeof(bool) * cantidad_marcos);             //(reserva) crea array de bools para marcar cada marco
    memset(bitmap_marcos, false, sizeof(bool) * cantidad_marcos);      // los pone a todos en false


    log_info(logger,"Memoria de usuario inicializada con %d marcos de %db cada uno",cantidad_marcos, campos_config.tam_pagina);
}