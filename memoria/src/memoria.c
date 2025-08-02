#include <memoria.h>

t_log* logger;
t_config* config;

int socket_servidor;
char* puerto;


int main(int argc, char* argv[]){
    config = iniciar_config();
    logger = iniciar_logger();
    creacion_estructuras_administrativas();

    socket_servidor = iniciar_servidor(campos_config.puerto_escucha, logger);
    lanzar_servidor(socket_servidor);
    pause();
    return 0;
}

//cerrar logger, config, etc
void terminar_programa(t_log* logger){
    log_destroy(logger);
    config_destroy(config);
} 