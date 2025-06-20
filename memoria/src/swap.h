#ifndef SWAP_H_
#define SWAP_H_
#include <stdio.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <config_memoria.h>

void inicializar_swap();
t_list* asignar_marcos_swap(int cantidad);
void escribir_en_swap(void* buffer, int nro_marco);
void leer_de_swap(void* buffer, int nro_marco);
void liberar_marcos(t_list* marcos);

#endif