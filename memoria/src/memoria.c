#include <utils/utils.c>
#include <instrucciones.h>
t_log* iniciar_logger();
t_config* iniciar_config();
t_log* logger;
t_config* config;
int socket_servidor;
char* puerto;

int main(int argc, char* argv[]) {
    logger = iniciar_logger();
    config = iniciar_config();
    socket_servidor = iniciar_servidor(puerto);
    //esperar_clientes_multiplexado(socket_servidor);
    //esperar_cliente(socket_servidor);
    mandar_instrucciones();
    return 0;

}

t_log* iniciar_logger(void){
    t_log* nuevo_logger;
    nuevo_logger = log_create("memoria.log","LogMem",true,LOG_LEVEL_INFO);
    log_info(nuevo_logger, "funciona logger memoria :)");
    return nuevo_logger;
}

t_config* iniciar_config(void){
    t_config* nuevo_config;
    nuevo_config = config_create("memoria.config");
    if(config_has_property(nuevo_config, "PUERTO_ESCUCHA")){
        puerto = config_get_string_value(nuevo_config, "PUERTO_ESCUCHA");
    }
    else{log_info(logger, "no se pudo leer el archivo de config");}
    log_info(logger, "el puerto: %s", puerto);
    return nuevo_config;
}
//cerrar logger, config, etc
void terminar_programa(t_log* logger){
    log_destroy(logger);
    config_destroy(config);
} 