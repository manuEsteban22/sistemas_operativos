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
char *algoritmo_planificacion_lp;
char *algoritmo_planificacion_cp;
t_dictionary* dispositivos_io;
t_dictionary* tabla_pcbs;

int main(int argc, char* argv[]) {
    logger = iniciar_logger();
    config = iniciar_config();
    tabla_pcbs = dictionary_create();
    dispositivos_io = dictionary_create();
    /*
    if(argc < 3){
        log_error(logger, "faltaron argumentos en la ejecucion");
        return EXIT_FAILURE;
    }
    char* archivo_pseudocodigo = argv[1];
    int tamanio_proceso = atoi(argv[2]);

    inicializar_planificador_lp(algoritmo_planificacion);

    crear_proceso(tamanio_proceso);

    planificador_largo_plazo();
    */

    //Hago la conexion como cliente al servidor memoria
    socket_memoria = conectar_memoria(ip_memoria, puerto_memoria);
    
    //Inicio los servidores de dispatch, interrupt e I/O
    socket_io = iniciar_servidor(puerto_io, logger);
    socket_dispatch = iniciar_servidor(puerto_dispatch, logger);
    socket_interrupt = iniciar_servidor(puerto_interrupt, logger);

    //Creo argumentos para los hilos
    t_args_hilo* args_dispatch = malloc(sizeof(t_args_hilo));
    t_args_hilo* args_interrupt = malloc(sizeof(t_args_hilo));
    args_dispatch->socket = socket_dispatch;
    args_interrupt->socket = socket_interrupt;
    args_dispatch->nombre = "DISPATCH";
    args_interrupt->nombre = "INTERRUPT";

    pthread_create(&thread_dispatch, NULL, manejar_servidor_cpu, (void*)args_dispatch);
    pthread_detach(thread_dispatch);

    pthread_create(&thread_interrupt, NULL, manejar_servidor_cpu, (void*)args_interrupt);
    pthread_detach(thread_interrupt);

    pthread_create(&thread_io, NULL, manejar_servidor_io, socket_io);
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
        PROCESOS_MEMORIA = config_get_int_value(nuevo_config, "PROCESOS_MEMORIA");
        algoritmo_planificacion_lp = config_get_string_value(nuevo_config, "ALGORITMO_PLANIFICACION_LARGO_PLAZO");
        algoritmo_planificacion_cp = config_get_string_value(nuevo_config, "ALGORITMO_PLANIFICACION_CORTO_PLAZO");
        

        log_info(logger, "no se pudo leer el archivo de config");
        log_info(logger, "la ip del server memoria es: %s", ip_memoria);
        log_info(logger, "el puerto del server memoria es: %s", puerto_memoria);
        log_info(logger, "el puerto del server dispatch es: %s", puerto_dispatch);
        log_info(logger, "el puerto del server interrupt es: %s", puerto_interrupt);
        log_info(logger, "el puerto del server io es: %s", puerto_io);
        log_info(logger, "algoritmo de planificación largo plazo: %s", algoritmo_planificacion_lp);
        log_info(logger, "algoritmo de planificación corto plazo: %s", algoritmo_planificacion_cp);
        return nuevo_config;
    }
}

void terminar_programa(int conexion, t_log* logger, t_config* config){
    config_destroy(config);
    log_destroy(logger);
}