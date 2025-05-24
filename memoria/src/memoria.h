#ifndef MEMORIA_H_
#define MEMORIA_H_

extern t_log* logger;

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
} t_configuracion;

extern t_configuracion campos_config;
#endif