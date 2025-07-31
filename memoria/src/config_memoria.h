#ifndef CONFIG_MEMORIA_H_
#define CONFIG_MEMORIA_H_
#include <memoria.h>
#include <utils/utils.h>

typedef struct
{
    char* puerto_escucha;
    int tam_memoria;
    int tam_pagina;
    int entradas_por_tabla;
    int cantidad_niveles;
    int retardo_memoria;
    char* path_swapfile;
    int retardo_swap;
    char* log_level;
    char* dump_path;
    char* path_instrucciones;
    int cantidad_marcos_swap;
} t_configuracion;


t_configuracion retornar_config(t_config * config);

t_config* iniciar_config(void);
t_log* iniciar_logger();

extern t_configuracion campos_config;
extern t_log* logger;

extern pthread_mutex_t mutex_swap;
extern pthread_mutex_t mutex_memoria;
extern pthread_mutex_t mutex_diccionario_instrucciones;
extern pthread_mutex_t mutex_diccionario_procesos;
extern pthread_mutex_t mutex_bitmap;

#endif