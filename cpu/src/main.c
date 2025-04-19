#include <utils/utils.c>
t_log* iniciar_logger();
t_config* iniciar_config();
t_log* logger;
t_config* config;
char* ip;
char* puerto;

int main(int argc, char* argv[]) {
    int conexion;
    logger = iniciar_logger();
    config = iniciar_config();
    conexion = crear_conexion(ip, puerto);
    return 0;
}

t_log* iniciar_logger(void){
    t_log* nuevo_logger;
    nuevo_logger = log_create("cpu.log","LogCPU",true,LOG_LEVEL_INFO);
    log_info(nuevo_logger, "funciona logger cpu :)");
    return nuevo_logger;
}

t_config* iniciar_config(void){
    t_config* nuevo_config;
    nuevo_config = config_create("cpu.config");
    if(config_has_property(nuevo_config, "IP") &&
    config_has_property(nuevo_config, "PUERTO")){
        ip = config_get_string_value(nuevo_config, "IP");
        puerto = config_get_string_value(nuevo_config, "PUERTO");
    }
    else{log_info(logger, "no se pudo leer el archivo de config");}
    log_info(logger, "la ip es: %s", ip);
    log_info(logger, "el puerto: %s", puerto);
    return nuevo_config;
}
void terminar_programa(int conexion, t_log* logger, t_config* config){
    config_destroy(config);
    log_destroy(logger);
}