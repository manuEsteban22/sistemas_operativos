#include <atender_kernel.h>
extern t_log* logger;

t_dictionary* tablas_por_pid;
tablas_por_pid = dictionary_create();

// busca pags libres, las asigna y arma tabla raiz

void* inicializar_proceso(int tam_proceso, int pid){

    int pags_necesarias = tam_proceso / campos_config.tam_pagina;
    t_tabla_paginas* tabla_raiz = crear_tabla(0);
    dictionary_put(tablas_por_pid, string_itoa(pid), tabla_raiz);

    for(int pagina = 0; pagina < cantidad_paginas; pagina++){

        int marco_libre = buscar_marco_libre();
        if (marco_libre == -1){
            return NULL; //falta q haga algo
        }

        bitmap_marcos(marco_libre) = true;

        t_entrada_tabla* entrada = buscar_entrada(tabla_raiz, pagina);

        entrada->presencia = true;
        entrada->marco = marco_libre;
    }

    return tabla_raiz;
}

void* suspender_proceso(){



}

void* des_suspender_proceso()
{

}

void* finalizar_proceso()
{

}

int buscar_marco_libre() 
{
    for (int i = 0; i < cantidad_marcos_totales; i++) {
        if (!bitarray_test_bit(bitmap_marcos, i)) {
            bitarray_set_bit(bitmap_marcos, i);
            return i;
        }
    }
    return -1;
}

// busca la entrada en cuestion

t_entrada_tabla* buscar_entrada(t_tabla_paginas* tabla_raiz, int nro_pagina){

    t_tabla_paginas* actual = tabla_raiz;
    int bits_por_nivel = log2(campos_config.entradas_por_tabla);

    for(int nivel = 0; nivel < campos_config.cantidad_niveles -1; nivel++){
        int indice = (nro_pagina >> (bits_pot_nivel * (campos_config.cantidad_niveles - nivel - 1))) & ((1 << bits_por_nivel) - 1);
        actual = actual->entradas[indice].siguiente_tabla;
    }

    int indice_final = nro_pagina & ((1 << bits_por_nivel) - 1);
    return &actual->entradas[indice_final];
}