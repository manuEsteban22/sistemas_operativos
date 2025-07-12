#ifndef SWAP_H_
#define SWAP_H_
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <commons/collections/list.h>
#include <string.h>
#include <config_memoria.h>

void inicializar_swap();
t_list* asignar_marcos_swap(int cantidad);
void escribir_en_swap(void* buffer, int nro_marco);
void leer_de_swap(void* buffer, int nro_marco);
void liberar_marcos(t_list* marcos);

typedef struct {
    int pid;
    int nro_pagina;
    int marco_swap;
} t_pagina_swap;

t_list* paginas_en_swap;

#endif