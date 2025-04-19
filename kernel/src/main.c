#include <utils/utils.c>
t_log* iniciar_logger();
t_config* iniciar_config();
t_log* logger;
t_config* config;
char* ip_memoria;
char* puerto_memoria;
char* puerto_dispatch;
char* puerto_interrupt;
int socket_dispatch;
int socket_interrupt;

int main(int argc, char* argv[]) {
    int conexion_memoria;
    logger = iniciar_logger();
    config = iniciar_config();
    conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);
    socket_dispatch = iniciar_servidor(puerto_dispatch);
    socket_interrupt = iniciar_servidor(puerto_interrupt);
    //esperar_cliente
    return 0;
}

t_log* iniciar_logger(void){
    t_log* nuevo_logger;
    nuevo_logger = log_create("kernel.log","LogKernel",true,LOG_LEVEL_INFO);
    log_info(nuevo_logger, "funciona logger kernel :)");
    return nuevo_logger;
}

t_config* iniciar_config(void){
    t_config* nuevo_config;
    nuevo_config = config_create("kernel.config");
    if(config_has_property(nuevo_config, "IP_MEMORIA") &&
    config_has_property(nuevo_config, "PUERTO_MEMORIA") && 
    config_has_property(nuevo_config, "PUERTO_ESCUCHA_DISPATCH") &&
    config_has_property(nuevo_config, "PUERTO_ESCUCHA_INTERRUPT")){
        ip_memoria = config_get_string_value(nuevo_config, "IP_MEMORIA");
        puerto_memoria = config_get_string_value(nuevo_config, "PUERTO_MEMORIA");
        puerto_dispatch = config_get_string_value(nuevo_config, "PUERTO_ESCUCHA_DISPATCH");
        puerto_interrupt = config_get_string_value(nuevo_config, "PUERTO_ESCUCHA_INTERRUPT");
    }
    else{log_info(logger, "no se pudo leer el archivo de config");}
    log_info(logger, "la ip del server memoria es: %s", ip_memoria);
    log_info(logger, "el puerto del server memoria es: %s", puerto_memoria);
    log_info(logger, "el puerto del server dispatch es: %s", puerto_dispatch);
     log_info(logger, "el puerto del server interrupt es: %s", puerto_interrupt);
    return nuevo_config;
}

void terminar_programa(int conexion, t_log* logger, t_config* config){
    config_destroy(config);
    log_destroy(logger);
}