#include <atender_kernel.h>
extern t_log* logger;

// t_dictionary* tablas_por_pid;
// t_list* paginas_en_swap;

<<<<<<< HEAD
// busca pags libres, las asigna y arma tabla raiz
=======

// busca pags libres, las asigna y arma tabla raiz
>>>>>>> 4cc24c82fb4ffc20da474d804199a3fc0b47bf93

void* inicializar_proceso(int tam_proceso, int pid){

<<<<<<< HEAD


    int pags_necesarias = tam_proceso / campos_config.tam_pagina;
    t_tabla_paginas* tabla_raiz = crear_tabla(0);
    dictionary_put(tablas_por_pid, string_itoa(pid), tabla_raiz);
=======
//     int pags_necesarias = tam_proceso / campos_config.tam_pagina;
//     t_tabla_paginas* tabla_raiz = crear_tabla(0);
//     dictionary_put(tablas_por_pid, string_itoa(pid), tabla_raiz);
>>>>>>> 4cc24c82fb4ffc20da474d804199a3fc0b47bf93

<<<<<<< HEAD
// //         int marco_libre = buscar_marco_libre();
// //         if (marco_libre == -1){
// //             return NULL; //falta q haga algo
// //         }
=======
        int marco_libre = buscar_marco_libre();
        if (marco_libre == -1){
            return NULL; //falta q haga algo
        }
>>>>>>> 4f31cf5fbbdc96e542caa2f184dc10cee55d37d5

        int marco_libre = buscar_marco_libre();
        if (marco_libre == -1){
            return NULL; //falta q haga algo
        }

<<<<<<< HEAD
//         entrada->presencia = true;
//         entrada->marco = marco_libre;
//     }

//     return tabla_raiz;
=======
        entrada->presencia = true;
        entrada->marco = marco_libre;
    }

    return tabla_raiz;
}

void suspender_proceso(int pid){


    char* pid_str = string_itoa(pid);
    t_tabla_paginas* tabla_raiz = dictionary_get(tablas_por_pid, pid_str);
    free(pid_str);

    if (!tabla_raiz){
        log_error(logger, "No se encontró la tabla de páginas para el proceso %d", pid);
        return;
    }

    suspender_tabla(tabla_raiz, 0, pid, 0);
}

void des_suspender_proceso(int pid){

    t_list* paginas_proceso = list_create(); //filtramos las paginas de ese proceso
    for (int i = 0; i < list_size(paginas_en_swap); i++){
        t_pagina_swap* relacion = list_get(paginas_en_swap, i);
        if(relacion->pid == pid){
            list_add(paginas_proceso, relacion);
        }
    }

    if (list_size(paginas_proceso) > cantidad_marcos_libres()){ //error si no hay suficientes marcos libres
        log_error(logger, "No hay marcos libres suficientes para des-suspender el proceso %d", pid);
        list_destroy(paginas_proceso);
        return;
    }

    char* pid_str = string_itoa(pid);
    t_tabla_paginas* tabla_raiz = dictionary_get(tablas_por_pid, pid_str); //actualizar tabla de pags

    for (int i = 0; i < list_size(paginas_proceso); i++) { //restaurar las pags
        t_pagina_swap* relacion = list_get(paginas_proceso, i);
        int marco_ram = buscar_marco_libre();
        if (marco_ram == -1) {
            log_error(logger, "Error inesperado: no hay marco libre para restaurar página");
            break;
        }

        void* buffer = malloc(campos_config.tam_pagina);
        leer_de_swap(buffer, relacion->marco_swap);
        memcpy(memoria_usuario + marco_ram * campos_config.tam_pagina, buffer, campos_config.tam_pagina);


        t_entrada_tabla* entrada = buscar_entrada(tabla_raiz, relacion->nro_pagina);
        entrada->presencia = true;
        entrada->marco = marco_ram;

        t_list* marcos_swap = list_create(); //liberar marcos del swap
        int* marco_swap_ptr = malloc(sizeof(int));
        *marco_swap_ptr = relacion->marco_swap;
        list_add(marcos_swap, marco_swap_ptr);
        liberar_marcos(marcos_swap);

        list_remove_element(paginas_en_swap, relacion);
        free(relacion);
        free(buffer);
    }
    free(pid_str);

    list_destroy(paginas_proceso);
    log_info(logger, "Proceso %d des-suspendido correctamente", pid);
}

void* finalizar_proceso(int pid){

<<<<<<< HEAD
    char* pid_str = string_itoa(pid); //liberamos marcos de ram y marcamos las entradas como no presentes
    t_tabla_paginas* tabla_raiz = dictionary_get(tablas_por_pid, pid_str);

    if (!tabla_raiz){
        log_error(logger, "No se encontró la tabla de páginas para el proceso %d", pid);
        free(pid_str);
        return;
    }

    liberar_tabla(tabla_raiz, 0); 

    for (int i = 0; i < list_size(paginas_en_swap); ){ //liberamos marcos de swap y elminamos las relaciones
        t_pagina_swap* relacion = list_get(paginas_en_swap, i);
        if (relacion->pid == pid){
            t_list* marcos_swap = list_create();
            int* marco_swap_ptr = malloc(sizeof(int));
            *marco_swap_ptr = relacion->marco_swap;
            list_add(marcos_swap, marco_swap_ptr);
            liberar_marcos(marcos_swap;)

            list_remove(paginas_en_swap, i);
            free(relacion);
        } else {
            i++; // se incrementa solo si no se elimino
        }
    }

    dictionary_remove(tablas_por_pid, pid_str); //elimina tabla del diccionario
    free(pid_str);

    log_info(logger, "Proceso %d finalizado. ")
}
=======
>>>>>>> 4f31cf5fbbdc96e542caa2f184dc10cee55d37d5
// }
>>>>>>> 4cc24c82fb4ffc20da474d804199a3fc0b47bf93

// void suspender_proceso(int pid){


//     char* pid_str = string_itoa(pid);
//     t_tabla_paginas* tabla_raiz = dictionary_get(tablas_por_pid, pid_str);
//     free(pid_str);

//     if (!tabla_raiz){
//         log_error(logger, "No se encontró la tabla de páginas para el proceso %d", pid);
//         return;
//     }

//     suspender_tabla(tabla_raiz, 0, pid, 0);
// }

// void des_suspender_proceso(int pid){

//     t_list* paginas_proceso = list_create(); //filtramos las paginas de ese proceso
//     for (int i = 0; i < list_size(paginas_en_swap); i++){
//         t_pagina_swap* relacion = list_get(paginas_en_swap, i);
//         if(relacion->pid == pid){
//             list_add(paginas_proceso, relacion);
//         }
//     }

//     if (list_size(paginas_proceso) > cantidad_marcos_libres()){ //error si no hay suficientes marcos libres
//         log_error(logger, "No hay marcos libres suficientes para des-suspender el proceso %d", pid);
//         list_destroy(paginas_proceso);
//         return;
//     }

//     char* pid_str = string_itoa(pid);
//     t_tabla_paginas* tabla_raiz = dictionary_get(tablas_por_pid, pid_str); //actualizar tabla de pags

//     for (int i = 0; i < list_size(paginas_proceso); i++) { //restaurar las pags
//         t_pagina_swap* relacion = list_get(paginas_proceso, i);
//         int marco_ram = buscar_marco_libre();
//         if (marco_ram == -1) {
//             log_error(logger, "Error inesperado: no hay marco libre para restaurar página");
//             break;
//         }

//         void* buffer = malloc(campos_config.tam_pagina);
//         leer_de_swap(buffer, relacion->marco_swap);
//         memcpy(memoria_usuario + marco_ram * campos_config.tam_pagina, buffer, campos_config.tam_pagina);


//         t_entrada_tabla* entrada = buscar_entrada(tabla_raiz, relacion->nro_pagina);
//         entrada->presencia = true;
//         entrada->marco = marco_ram;

//         t_list* marcos_swap = list_create(); //liberar marcos del swap
//         int* marco_swap_ptr = malloc(sizeof(int));
//         *marco_swap_ptr = relacion->marco_swap;
//         list_add(marcos_swap, marco_swap_ptr);
//         liberar_marcos(marcos_swap);

//         list_remove_element(paginas_en_swap, relacion);
//         free(relacion);
//         free(buffer);
//     }
//     free(pid_str);

        t_list* marcos_swap = list_create(); //liberar marcos del swap
        int* marco_swap_ptr = malloc(sizeof(int));
        *marco_swap_ptr = relacion->marco_swap;
        list_add(marcos_swap, marco_swap_ptr);
        liberar_marcos(marcos_swap);

        list_remove_element(paginas_en_swap, relacion);
        free(relacion);
        free(buffer);
    }
    free(pid_str);

    list_destroy(paginas_proceso);
    log_info(logger, "Proceso %d des-suspendido correctamente", pid);
}

void* finalizar_proceso(int pid){

<<<<<<< HEAD
    char* pid_str = string_itoa(pid); //liberamos marcos de ram y marcamos las entradas como no presentes
    t_tabla_paginas* tabla_raiz = dictionary_get(tablas_por_pid, pid_str);

    if (!tabla_raiz){
        log_error(logger, "No se encontró la tabla de páginas para el proceso %d", pid);
        free(pid_str);
        return;
    }

    liberar_tabla(tabla_raiz, 0); 

    for (int i = 0; i < list_size(paginas_en_swap); ){ //liberamos marcos de swap y elminamos las relaciones
        t_pagina_swap* relacion = list_get(paginas_en_swap, i);
        if (relacion->pid == pid){
            t_list* marcos_swap = list_create();
            int* marco_swap_ptr = malloc(sizeof(int));
            *marco_swap_ptr = relacion->marco_swap;
            list_add(marcos_swap, marco_swap_ptr);
            liberar_marcos(marcos_swap;)

            list_remove(paginas_en_swap, i);
            free(relacion);
        } else {
            i++; // se incrementa solo si no se elimino
        }
    }

    dictionary_remove(tablas_por_pid, pid_str); //elimina tabla del diccionario
    free(pid_str);

// // }

// // int buscar_marco_libre() 
// // {
// //     for (int i = 0; i < cantidad_marcos_totales; i++) {
// //         if (!bitarray_test_bit(bitmap_marcos, i)) {
// //             bitarray_set_bit(bitmap_marcos, i);
// //             return i;
// //         }
// //     }
// //     return -1;
// // }

// // // busca la entrada en cuestion

// // t_entrada_tabla* buscar_entrada(t_tabla_paginas* tabla_raiz, int nro_pagina){

// //     t_tabla_paginas* actual = tabla_raiz;
// //     int bits_por_nivel = log2(campos_config.entradas_por_tabla);

//     for(int nivel = 0; nivel < campos_config.cantidad_niveles -1; nivel++){
//         int indice = (nro_pagina >> (bits_por_nivel * (campos_config.cantidad_niveles - nivel - 1))) & ((1 << bits_por_nivel) - 1);
//         actual = actual->entradas[indice].siguiente_tabla;
//     }

//     int indice_final = nro_pagina & ((1 << bits_por_nivel) - 1);
//     return &actual->entradas[indice_final];
// }

// void suspender_tabla(t_tabla_paginas* tabla, int nivel, int pid, int pagina_base){

//     for (int i = 0; i < campos_config.entradas_por_tabla; i++){
//         t_entrada_tabla* entrada = &tabla->entradas[i];
//         int salto = (int)pow(campos_config.entradas_por_tabla, campos_config.cantidad_niveles - nivel - 1);
//         int nro_pagina = pagina_base + i * salto;

//         if (nivel < campos_config.cantidad_niveles - 1){
//             if (entrada->siguiente_tabla)
//                 suspender_tabla(entrada->siguiente_tabla, nivel + 1, pid, nro_pagina);
            
//         } else {
//             if (entrada->presencia){
//                 suspender_pagina(pid, nro_pagina, entrada->marco);
//                 bitarray_clean_bit(bitmap_marcos, entrada->marco);
//                 entrada->presencia = false;
//             }
//         }
//     }
// }

// void suspender_pagina(int pid, int nro_pagina, int marco){

//     t_list* marcos_swap = asignar_marcos_swap(1); //pide marco libre

//     if (!marcos_swap){
//         log_error(logger, "No hay marcos libres en swap para suspender la pagina %d del proceso %d", nro_pagina, pid);
//         return;
//     }

//     int marco_swap = *(int*)list_get(marcos_swap, 0);

//     void* buffer = malloc(campos_config.tam_pagina); //copia el contenido de las paginas
//     memcpy(buffer, memoria_usuario + marco * campos_config.tam_pagina, campos_config.tam_pagina);

//     escribir_en_swap(buffer, marco_swap); //lo escribe en el swap

//     t_pagina_swap* relacion = malloc(sizeof(t_pagina_swap)); //guarda la relacion para cuando tenga que restaurarla
//     relacion->pid = pid;
//     relacion->nro_pagina = nro_pagina;
//     relacion->marco_swap = marco_swap;
//     list_add(paginas_en_swap, relacion);

//     free(buffer); //libera
//     liberar_marcos(marcos_swap);
// }