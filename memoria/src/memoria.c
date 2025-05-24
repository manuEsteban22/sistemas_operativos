#include <utils/utils.c>
#include <instrucciones.h>
#include <memoria.h>
t_log* iniciar_logger();
t_config* iniciar_config();
t_log* logger;
t_config* config;
int socket_servidor;
char* puerto;
t_configuracion campos_config;

int main(int argc, char* argv[]) {
    logger = iniciar_logger();
    config = iniciar_config();
    socket_servidor = iniciar_servidor(campos_config.puerto_escucha);
    //esperar_clientes_multiplexado(socket_servidor);
    //esperar_cliente(socket_servidor);
    mandar_instrucciones();
    return 0;

}

t_log* iniciar_logger(void)
{
    t_log* nuevo_logger;
    nuevo_logger = log_create("memoria.log","LogMem",true,LOG_LEVEL_INFO);
    log_info(nuevo_logger, "funciona logger memoria :)");
    return nuevo_logger;
}

t_configuracion retornar_config(t_config * config)
{
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
    return campos_config;
}

t_config* iniciar_config(void)
{
    t_config* nuevo_config;
    nuevo_config = config_create("memoria.config");
    if(config_has_property(nuevo_config, "PUERTO_ESCUCHA"))
    {
        retornar_config(nuevo_config);
    }
    else{log_info(logger, "no se pudo leer el archivo de config");}
    log_info(logger, "el puerto: %s", campos_config.puerto_escucha);
    return nuevo_config;
}
//cerrar logger, config, etc
void terminar_programa(t_log* logger)
{
    log_destroy(logger);
    config_destroy(config);
} 