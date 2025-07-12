#ifndef ADMINISTRACION_MEMORIA_H_
#define ADMINISTRACION_MEMORIA_H_
#include <memoria.h>
#include <utils/utils.h>


extern void* memoria_usuario;
extern bool* bitmap_marcos;
void inicializar_memoria();

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


#endif