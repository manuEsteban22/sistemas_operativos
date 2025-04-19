#include <utils/utils.c>
t_log* iniciar_logger();
t_log* logger;
int socket_servidor;

int main(int argc, char* argv[]) {
    logger = iniciar_logger();
    socket_servidor = iniciar_servidor();
    esperar_cliente(socket_servidor);
    return 0;
}

t_log* iniciar_logger(void){
    t_log* nuevo_logger;
    nuevo_logger = log_create("memoria.log","LogMem",true,LOG_LEVEL_INFO);
    log_info(nuevo_logger, "funciona logger memoria :)");
    return nuevo_logger;
}
//cerrar logger, config, etc
void terminar_programa(t_log* logger){
    log_destroy(logger);
}