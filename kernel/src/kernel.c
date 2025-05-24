#include <utils/utils.c>
#include <utils/utils.h>
#include <conexion_kernel.h>
#include <planificador_largo_plazo.h>
#include <kernel.h>

t_log* iniciar_logger();
t_config* iniciar_config();
t_log* logger;
t_config* config;
int PROCESOS_MEMORIA;
char* ip_memoria;
char* puerto_memoria;
char* puerto_dispatch;
char* puerto_interrupt;
char* puerto_io;
int socket_io;
int socket_dispatch;
int socket_interrupt;
int socket_memoria;
pthread_t thread_io;
pthread_t thread_dispatch;
pthread_t thread_interrupt;
char *algoritmo_planificacion;

int main(int argc, char* argv[]) {
    logger = iniciar_logger();
    config = iniciar_config();
    inicializar_planificador_lp();
    
    
    //Hago la conexion como cliente al servidor memoria
    socket_memoria = crear_conexion(ip_memoria, puerto_memoria);
    
    //Inicio los servidores de dispatch, interrupt e I/O
    socket_io = iniciar_servidor(puerto_io);
    socket_dispatch = iniciar_servidor(puerto_dispatch);
    socket_interrupt = iniciar_servidor(puerto_interrupt);

    //Creo argumentos para los hilos
    t_args_hilo* args_dispatch = malloc(sizeof(t_args_hilo));
    t_args_hilo* args_interrupt = malloc(sizeof(t_args_hilo));
    t_args_hilo* args_io = malloc(sizeof(t_args_hilo));
    args_dispatch->socket = socket_dispatch;
    args_interrupt->socket = socket_interrupt;
    args_io->socket = socket_io;
    args_dispatch->nombre = "DISPATCH";
    args_interrupt->nombre = "INTERRUPT";
    args_io->nombre = "IO";

    pthread_create(&thread_dispatch, NULL, manejar_servidor_cpu, (void*)args_dispatch);
    pthread_detach(thread_dispatch);

    pthread_create(&thread_interrupt, NULL, manejar_servidor_cpu, (void*)args_interrupt);
    pthread_detach(thread_interrupt);

    pthread_create(&thread_io, NULL, manejar_servidor, (void*)args_io);
    pthread_detach(thread_io);

    

    log_info(logger, "Kernel iniciado, esperando conexiones...");
    while(1){
        pause();
    }

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
    if(config_has_property(nuevo_config, "IP_MEMORIA")){
        ip_memoria = config_get_string_value(nuevo_config, "IP_MEMORIA");
        puerto_memoria = config_get_string_value(nuevo_config, "PUERTO_MEMORIA");
        puerto_dispatch = config_get_string_value(nuevo_config, "PUERTO_ESCUCHA_DISPATCH");
        puerto_interrupt = config_get_string_value(nuevo_config, "PUERTO_ESCUCHA_INTERRUPT");
        puerto_io = config_get_string_value(nuevo_config, "PUERTO_ESCUCHA_IO");
        PROCESOS_MEMORIA = config_get_string_value(nuevo_config, "PROCESOS_MEMORIA");
        algoritmo_planificacion = config_get_string_value(nuevo_config, "ALGORITMO_PLANIFICACION");
        
        chequear_algoritmo_planificacion (algoritmo_planificacion);
        log_info(logger, "no se pudo leer el archivo de config");}
        log_info(logger, "la ip del server memoria es: %s", ip_memoria);
        log_info(logger, "el puerto del server memoria es: %s", puerto_memoria);
        log_info(logger, "el puerto del server dispatch es: %s", puerto_dispatch);
        log_info(logger, "el puerto del server interrupt es: %s", puerto_interrupt);
        log_info(logger, "el puerto del server io es: %s", puerto_io);
        log_info(logger, "algoritmo de planificación: %s", algoritmo_planificacion);
        return nuevo_config;
}


void terminar_programa(int conexion, t_log* logger, t_config* config){
    config_destroy(config);
    log_destroy(logger);
}