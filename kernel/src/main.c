#include <utils/utils.c>
#include <utils/utils.h>
t_log* iniciar_logger();
t_config* iniciar_config();
t_log* logger;
t_config* config;
char* ip_memoria;
char* puerto_memoria;
char* puerto_dispatch;
char* puerto_interrupt;
char* puerto_io;
int socket_io;
int socket_dispatch;
int socket_interrupt;
pthread_t thread_io;
pthread_t thread_dispatch;
pthread_t thread_interrupt;

int main(int argc, char* argv[]) {
    int conexion_memoria;
    logger = iniciar_logger();
    config = iniciar_config();
    //hago la conexion como cliente al servidor memoria
    conexion_memoria = crear_conexion(ip_memoria, puerto_memoria);
    //inicio ambos servidores para recibir señales de dispatch e interrupt del CPU
    socket_io = iniciar_servidor(puerto_io);
    socket_dispatch = iniciar_servidor(puerto_dispatch);
    socket_interrupt = iniciar_servidor(puerto_interrupt);
    //creo hilos para poder manejar los accept en ambas instancias de servidor
    pthread_create(&thread_dispatch, NULL, (void*)esperar_cliente, socket_dispatch);
    pthread_create(&thread_interrupt, NULL, (void*)esperar_cliente, socket_interrupt);
    pthread_create(&thread_io, NULL, (void*)esperar_cliente, socket_io);
    //desconecta los hilos cuando terminan
    pthread_join(thread_dispatch, NULL);
    pthread_join(thread_interrupt, NULL);
    pthread_join(thread_io, NULL);
    //esperar_clientes_multiplexado(socket_dispatch)
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
    config_has_property(nuevo_config, "PUERTO_ESCUCHA_INTERRUPT") &&
    config_has_property(nuevo_config, "PUERTO_ESCUCHA_IO")){
        ip_memoria = config_get_string_value(nuevo_config, "IP_MEMORIA");
        puerto_memoria = config_get_string_value(nuevo_config, "PUERTO_MEMORIA");
        puerto_dispatch = config_get_string_value(nuevo_config, "PUERTO_ESCUCHA_DISPATCH");
        puerto_interrupt = config_get_string_value(nuevo_config, "PUERTO_ESCUCHA_INTERRUPT");
        puerto_io = config_get_string_value(nuevo_config, "PUERTO_ESCUCHA_IO");
    }
    else{log_info(logger, "no se pudo leer el archivo de config");}
    log_info(logger, "la ip del server memoria es: %s", ip_memoria);
    log_info(logger, "el puerto del server memoria es: %s", puerto_memoria);
    log_info(logger, "el puerto del server dispatch es: %s", puerto_dispatch);
    log_info(logger, "el puerto del server interrupt es: %s", puerto_interrupt);
    log_info(logger, "el puerto del server io es: %s", puerto_io);
    return nuevo_config;
}

void terminar_programa(int conexion, t_log* logger, t_config* config){
    config_destroy(config);
    log_destroy(logger);
}