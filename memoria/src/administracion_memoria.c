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

    // bitmap_marcos = malloc(sizeof(bool) * cantidad_marcos);            
    // memset(bitmap_marcos, false, sizeof(bool) * cantidad_marcos); 

    int tam_en_bytes = (cantidad_marcos + 7)/8;

    char* buffer = malloc(tam_en_bytes);
    memset(buffer, 0, tam_en_bytes);

    bitmap_marcos = bitarray_create_with_mode(buffer, tam_en_bytes, LSB_FIRST);


    log_info(logger,"Memoria de usuario inicializada con %d marcos de %db cada uno",cantidad_marcos, campos_config.tam_pagina);
}


t_tabla_paginas* crear_tabla(int nivel_actual)
{

    t_tabla_paginas* tabla = malloc(sizeof(t_tabla_paginas));
    tabla->entradas = malloc(sizeof(t_entrada_tabla) * campos_config.entradas_por_tabla);


    for(int i = 0; i < campos_config.entradas_por_tabla; i++)
    {
        tabla->entradas[i].presencia = false;
        tabla->entradas[i].marco = 0;
        if (nivel_actual < campos_config.cantidad_niveles) {
            tabla->entradas[i].siguiente_tabla = crear_tabla(nivel_actual + 1);
        } else {
            tabla->entradas[i].siguiente_tabla = NULL; // Último nivel
        }
    }
    return tabla;
} 

void* creacion_estructuras_administrativas()
{
   inicializar_memoria();
   tablas_por_pid = dictionary_create();
    lista_de_instrucciones_por_pid = dictionary_create();
   //crear_tabla(0);
   return NULL;
}

