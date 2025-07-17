#ifndef ADMINISTRACION_MEMORIA_H_
#define ADMINISTRACION_MEMORIA_H_
#include <memoria.h>
#include <utils/utils.h>
#include <commons/bitarray.h>
#include <atencion_kernel.h>

extern void* memoria_usuario;
extern t_bitarray* bitmap_marcos;
void inicializar_memoria();
extern t_dictionary* tablas_por_pid;

typedef struct t_tabla_paginas t_tabla_paginas;

typedef struct{

    bool presencia;
    uint32_t marco; 
    t_tabla_paginas* siguiente_tabla;

} t_entrada_tabla;

struct t_tabla_paginas{

    t_entrada_tabla* entradas;

};

t_tabla_paginas* crear_tabla(int nivel_actual);
void* creacion_estructuras_administrativas();

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

#endif