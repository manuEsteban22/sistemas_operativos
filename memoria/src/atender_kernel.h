#ifndef ATENDER_KERNEL_H_
#define ATENDER_KERNEL_H_
#include<config_memoria.h>
#include<memoria.h>

void* inicializar_proceso(int tam_proceso, int pid);

typedef struct{
    int cantidad_accesos_tablas_de_paginas;
    int cantidad_instrucciones_solicitadas;
    int cantidad_bajadas_a_swap;
    int cantidad_subidas_a_memoria;
    int cantidad_lecturas_memoria;
    int cantidad_escrituras_memoria;
} t_metricas;

typedef struct{
    int pid;
    t_tabla_paginas tabla_raiz;
    int paginas_usadas;
    int accesos_memoria;
    int page_faults;
    t_metricas metricas;
} t_proceso;

#endif