#include <memoria.h>
t_log* iniciar_logger();
t_log* logger;
t_config* config;

int socket_servidor;
char* puerto;


int main(int argc, char* argv[]){
    config = iniciar_config();
    logger = iniciar_logger();
    socket_servidor = iniciar_servidor(campos_config.puerto_escucha, logger);
    lanzar_servidor(socket_servidor);
    //creacion_estructuras_administrativas();
    paginas_en_swap = list_create();
    while(1){pause();};
    return 0;
}

t_log* iniciar_logger(void){
    t_log_level nivel = log_level_from_string (campos_config.log_level);
    t_log* nuevo_logger;
    nuevo_logger = log_create("memoria.log","LogMem",true, nivel);
    log_trace(nuevo_logger, "Funciona logger memoria :)");
    return nuevo_logger;
}


//cerrar logger, config, etc
void terminar_programa(t_log* logger){
    log_destroy(logger);
    config_destroy(config);
} 