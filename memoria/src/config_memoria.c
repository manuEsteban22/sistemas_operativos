#include <config_memoria.h>

t_config* iniciar_config();
t_log* iniciar_logger();

t_configuracion campos_config;

t_configuracion retornar_config(t_config * config){
    campos_config.puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    campos_config.tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
    campos_config.tam_pagina = config_get_int_value(config, "TAM_PAGINA");
    campos_config.entradas_por_tabla = config_get_int_value(config, "ENTRADAS_POR_TABLA");
    campos_config.cantidad_niveles = config_get_int_value(config, "CANTIDAD_NIVELES");
    campos_config.retardo_memoria = config_get_int_value(config, "RETARDO_MEMORIA");
    campos_config.path_swapfile = config_get_string_value(config, "PATH_SWAPFILE");
    campos_config.retardo_swap = config_get_int_value(config, "RETARDO_SWAP");
    campos_config.log_level = config_get_string_value(config, "LOG_LEVEL");
    campos_config.dump_path = config_get_string_value(config, "DUMP_PATH");
    campos_config.path_instrucciones = config_get_string_value(config, "PATH_INSTRUCCIONES");
    campos_config.cantidad_marcos_swap = config_get_int_value(config,"CANTIDAD_MARCOS_SWAP");
    return campos_config;
}

t_log* iniciar_logger(void){
    t_log_level nivel = log_level_from_string (campos_config.log_level);
    t_log* nuevo_logger;
    nuevo_logger = log_create("memoria.log","LogMem",true, nivel);
    log_trace(nuevo_logger, "Funciona logger memoria :)");
    return nuevo_logger;
}

t_config* iniciar_config(void){
    t_config* nuevo_config;
    nuevo_config = config_create("memoria.config");
    if(config_has_property(nuevo_config, "PUERTO_ESCUCHA"))
    {
        retornar_config(nuevo_config);
    }
    return nuevo_config;
}

// MUTEX



pthread_mutex_t mutex_memoria = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_diccionario_procesos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_diccionario_instrucciones = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_swap = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_bitmap = PTHREAD_MUTEX_INITIALIZER;