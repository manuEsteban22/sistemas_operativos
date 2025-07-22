#ifndef ATENCION_KERNEL_H_
#define ATENCION_KERNEL_H_

#include "administracion_memoria.h"
#include <commons/collections/list.h>
#include <math.h>

// Funciones públicas
int cantidad_marcos_libres();
void liberar_tabla(t_tabla_paginas* tabla, int nivel_actual);
void* inicializar_proceso(int tam_proceso, int pid, char* nombre_archivo);
void suspender_proceso(int pid);
void des_suspender_proceso(int pid);
void* finalizar_proceso(int pid);
int buscar_marco_libre();
bool entrada_valida(t_entrada_tabla* entrada);
t_entrada_tabla* buscar_entrada(t_tabla_paginas* tabla, int pagina, int nivel_actual);
void suspender_tabla(t_tabla_paginas* tabla, int nivel, int pid, int pagina_base);
void suspender_pagina(int pid, int nro_pagina, int marco);
void liberar_proceso(void* proceso_void);

#endif