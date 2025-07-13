#ifndef ATENDER_KERNEL_H_
#define ATENDER_KERNEL_H_
#include <config_memoria.h>
#include <memoria.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <math.h>


// Estructuras de métricas y proceso
extern t_dictionary* tablas_por_pid;

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

// Funciones públicas
int cantidad_marcos_libres();
void* inicializar_proceso(int tam_proceso, int pid);
void suspender_proceso(int pid);
void des_suspender_proceso(int pid);
void* finalizar_proceso(int pid);
int buscar_marco_libre();
t_entrada_tabla* buscar_entrada(t_tabla_paginas* tabla_raiz, int nro_pagina);
void suspender_tabla(t_tabla_paginas* tabla, int nivel, int pid, int pagina_base);
void suspender_pagina(int pid, int nro_pagina, int marco);

#endif