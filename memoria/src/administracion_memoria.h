#ifndef ADMINISTRACION_MEMORIA_H_
#define ADMINISTRACION_MEMORIA_H_

#include <commons/bitarray.h>
#include <commons/collections/dictionary.h>
#include <stdbool.h>
#include <stdint.h>

// Forward declaration
typedef struct t_tabla_paginas t_tabla_paginas;

extern void* memoria_usuario;
extern t_bitarray* bitmap_marcos;
extern t_dictionary* tablas_por_pid;

void inicializar_memoria();
t_tabla_paginas* crear_tabla(int nivel_actual);

// Estructuras de métricas y proceso
typedef struct {
    int cantidad_accesos_tablas_de_paginas;
    int cantidad_instrucciones_solicitadas;
    int cantidad_bajadas_a_swap;
    int cantidad_subidas_a_memoria;
    int cantidad_lecturas_memoria;
    int cantidad_escrituras_memoria;
} t_metricas;

typedef struct {
    int pid;
    t_tabla_paginas* tabla_raiz;
    int paginas_usadas;
    int accesos_memoria;
    int page_faults;
    t_metricas metricas;
} t_proceso;

// Definición de t_entrada_tabla y t_tabla_paginas
typedef struct {
    bool presencia;
    uint32_t marco;
    t_tabla_paginas* siguiente_tabla;
} t_entrada_tabla;

struct t_tabla_paginas {
    t_entrada_tabla* entradas;
};

#endif