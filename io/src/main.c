#include <utils/utils.c>
t_log* iniciar_logger();
t_config* iniciar_config();
t_log* logger;
t_config* config;
char* ip_kernel;
char* puerto_kernel;

int main(int argc, char* argv[]) {
    int conexion_kernel;
    logger = iniciar_logger();
    config = iniciar_config();
    conexion_kernel = crear_conexion(ip_kernel, puerto_kernel);
    return 0;
}

t_log* iniciar_logger(void){
    t_log* nuevo_logger;
    nuevo_logger = log_create("io.log","LogIO",true,LOG_LEVEL_INFO);
    log_info(nuevo_logger, "funciona logger IO :)");
    return nuevo_logger;
}

t_config* iniciar_config(void){
    t_config* nuevo_config;
    nuevo_config = config_create("io.config");
    if(config_has_property(nuevo_config, "IP_KERNEL") &&
    config_has_property(nuevo_config, "PUERTO_KERNEL")){
        ip_kernel = config_get_string_value(nuevo_config, "IP_KERNEL");
        puerto_kernel = config_get_string_value(nuevo_config, "PUERTO_KERNEL");
    }
    else{log_info(logger, "no se pudo leer el archivo de config");}
    log_info(logger, "la ip del kernel es: %s", ip_kernel);
    log_info(logger, "el puerto del kernel es: %s", puerto_kernel);
    return nuevo_config;
}