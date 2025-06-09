#ifndef ADMINISTRACION_MEMORIA_H_
#define ADMINISTRACION_MEMORIA_H_
#include <memoria.h>
#include <utils/utils.h>


void inicializar_memoria();

typedef struct {
    int marco;
    bool presente;
    bool modificado;
    int uso;
} entrada_pagina;

typedef struct {
    int pid;
    entrada_pagina** tabla_nivel_1;
    // u otras estructuras según el esquema de paginación
} proceso_memoria;

#endif