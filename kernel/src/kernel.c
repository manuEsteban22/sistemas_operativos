#include <utils/utils.c>
#include <utils/utils.h>
#include <conexion_kernel.h>
#include <planificador_largo_plazo.h>
#include <kernel.h>
#include <planificador_corto_plazo.h>

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
int tiempo_suspension;
pthread_t thread_io;
pthread_t thread_principal;
char *algoritmo_largo_plazo;
char *algoritmo_corto_plazo;
t_dictionary* dispositivos_io;
t_dictionary* tabla_pcbs;
t_dictionary* tabla_exec;
pthread_mutex_t mutex_exec = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cpus_libres = PTHREAD_MUTEX_INITIALIZER;
t_dictionary* tabla_dispatch;
t_dictionary* tabla_interrupt;
t_queue* cpus_libres;

int main(int argc, char* argv[]) {
    logger = iniciar_logger();
    config = iniciar_config();
    tabla_pcbs = dictionary_create();
    dispositivos_io = dictionary_create();
    tabla_exec = dictionary_create();
    tabla_dispatch = dictionary_create();
    tabla_interrupt = dictionary_create();
    cpus_libres = queue_create();

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
    
    //Inicio los servidores de dispatch, interrupt e I/O
    socket_io = iniciar_servidor(puerto_io, logger);
    socket_dispatch = iniciar_servidor(puerto_dispatch, logger);
    socket_interrupt = iniciar_servidor(puerto_interrupt, logger);

    int* ptr_io = malloc(sizeof(int));
    ptr_io = socket_io;
    int* ptr_dispatch = malloc(sizeof(int));
    ptr_dispatch = socket_dispatch;


    pthread_create(&thread_principal, NULL, hilo_main_cpu, NULL);
    pthread_detach(thread_principal);

    pthread_create(&thread_io, NULL, manejar_servidor_io, ptr_io);
    pthread_detach(thread_io);
    
    inicializar_planificador_lp(algoritmo_largo_plazo);
    inicializar_planificador_cp(algoritmo_corto_plazo);
    crear_proceso(256);

    pthread_t thread_planificador_lp;
    pthread_create(&thread_planificador_lp, NULL, (void*)planificador_largo_plazo, NULL);
    pthread_detach(thread_planificador_lp);

    pthread_t thread_planificador_cp;
    pthread_create(&thread_planificador_cp, NULL, (void*)planificador_corto_plazo_loop, ptr_dispatch);
    pthread_detach(thread_planificador_cp);

    pthread_t thread_planificador_mp;
    pthread_create(&thread_planificador_mp, NULL, (void*)planificador_mediano_plazo, NULL);
    pthread_detach(thread_planificador_mp);

    log_info(logger, "Kernel iniciado, esperando conexiones...");
    while(1){
        pause();
    }

    return 0;
}

t_log* iniciar_logger(void){
    t_log* nuevo_logger;
    nuevo_logger = log_create("kernel.log","LogKernel",true,LOG_LEVEL_TRACE);
    log_info(nuevo_logger, "funciona logger kernel :)");
    return nuevo_logger;
}

t_config* iniciar_config(void){
    t_config* nuevo_config = config_create("kernel.config");

    if (nuevo_config == NULL) {
        log_error(logger, "No se pudo abrir el archivo de configuración kernel.config");
        exit(EXIT_FAILURE);
    }
    
    if(config_has_property(nuevo_config, "IP_MEMORIA")){
        ip_memoria = config_get_string_value(nuevo_config, "IP_MEMORIA");
        puerto_memoria = config_get_string_value(nuevo_config, "PUERTO_MEMORIA");
        puerto_dispatch = config_get_string_value(nuevo_config, "PUERTO_ESCUCHA_DISPATCH");
        puerto_interrupt = config_get_string_value(nuevo_config, "PUERTO_ESCUCHA_INTERRUPT");
        puerto_io = config_get_string_value(nuevo_config, "PUERTO_ESCUCHA_IO");
        PROCESOS_MEMORIA = config_get_int_value(nuevo_config, "PROCESOS_MEMORIA");
        algoritmo_largo_plazo = config_get_string_value(nuevo_config, "ALGORITMO_INGRESO_A_READY");
        algoritmo_corto_plazo = config_get_string_value(nuevo_config, "ALGORITMO_CORTO_PLAZO");
        tiempo_suspension = config_get_int_value(nuevo_config, "TIEMPO_SUSPENSION");

        //log_info(logger, "no se pudo leer el archivo de config");
        log_info(logger, "la ip del server memoria es: %s", ip_memoria);
        log_info(logger, "el puerto del server memoria es: %s", puerto_memoria);
        log_info(logger, "el puerto del server dispatch es: %s", puerto_dispatch);
        log_info(logger, "el puerto del server interrupt es: %s", puerto_interrupt);
        log_info(logger, "el puerto del server io es: %s", puerto_io);
        log_info(logger, "algoritmo de planificación largo plazo: %s", algoritmo_largo_plazo);
        log_info(logger, "algoritmo de planificación corto plazo: %s", algoritmo_corto_plazo);
        return nuevo_config;
    }   else {
            log_error(logger, "Falta IP_MEMORIA en el config");
            config_destroy(nuevo_config);
            exit(EXIT_FAILURE);
        }
}

void terminar_programa(int conexion, t_log* logger, t_config* config){
    config_destroy(config);
    log_destroy(logger);
}