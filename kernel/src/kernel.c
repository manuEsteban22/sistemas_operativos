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
double alfa;
double estimacion_inicial;
char* log_level;
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
pthread_t thread_io_main;
pthread_t thread_principal;
char *algoritmo_largo_plazo;
char *algoritmo_corto_plazo;
t_dictionary* dispositivos_io;
t_dictionary* tabla_sockets_io;
t_dictionary* tabla_pcbs;
t_dictionary* tabla_exec;
pthread_mutex_t mutex_exec = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cpus_libres = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_tabla_pcbs = PTHREAD_MUTEX_INITIALIZER;
t_dictionary* tabla_dispatch;
t_dictionary* tabla_interrupt;
t_queue* cpus_libres;

int main(int argc, char* argv[]) {
    config = iniciar_config();
    logger = iniciar_logger();
    tabla_pcbs = dictionary_create();
    dispositivos_io = dictionary_create();
    tabla_exec = dictionary_create();
    tabla_dispatch = dictionary_create();
    tabla_interrupt = dictionary_create();
    cpus_libres = queue_create();

    
    if(argc < 3){
        log_error(logger, "Faltaron argumentos en la ejecucion");
        return EXIT_FAILURE;
    }
    char* archivo_pseudocodigo = argv[1];
    int tamanio_proceso = atoi(argv[2]);
    
    
    //Inicio los servidores de dispatch, interrupt e I/O
    socket_io = iniciar_servidor(puerto_io, logger);
    socket_dispatch = iniciar_servidor(puerto_dispatch, logger);
    socket_interrupt = iniciar_servidor(puerto_interrupt, logger);

    iniciar_estructuras();

    // int* ptr_io = malloc(sizeof(int));
    // ptr_io = socket_io;
    int* ptr_dispatch = malloc(sizeof(int));
    *ptr_dispatch = socket_dispatch;


    pthread_create(&thread_principal, NULL, hilo_main_cpu, NULL);
    pthread_detach(thread_principal);

    pthread_create(&thread_io_main, NULL, hilo_main_io, NULL);
    pthread_detach(thread_io_main);
    
    inicializar_planificador_lp();
    inicializar_planificador_mp();
    proceso_arranque(tamanio_proceso, archivo_pseudocodigo);
    sem_post(&sem_plp);

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
    pause();

    return 0;
}

t_log* iniciar_logger(void){
    t_log* nuevo_logger;
    t_log_level level = log_level_from_string(log_level);
    nuevo_logger = log_create("kernel.log","LogKernel",true,level);
    return nuevo_logger;
}

void iniciar_estructuras(){
    sem_init(&cpus_disponibles, 0, 0);
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
        algoritmo_largo_plazo = config_get_string_value(nuevo_config, "ALGORITMO_INGRESO_A_READY");
        algoritmo_corto_plazo = config_get_string_value(nuevo_config, "ALGORITMO_CORTO_PLAZO");
        tiempo_suspension = config_get_int_value(nuevo_config, "TIEMPO_SUSPENSION");
        alfa = config_get_double_value(nuevo_config, "ALFA");
        estimacion_inicial = config_get_double_value(nuevo_config, "ESTIMACION_INICIAL");
        log_level = config_get_string_value(nuevo_config, "LOG_LEVEL");

        return nuevo_config;
    }   else {
            config_destroy(nuevo_config);
            exit(EXIT_FAILURE);
        }
}

void proceso_arranque(int tamanio_proceso, char* nombre_archivo){

    int pid = crear_proceso(tamanio_proceso);

    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OC_INIT);
    agregar_a_paquete(paquete, &pid, sizeof(int));
    agregar_a_paquete(paquete, &tamanio_proceso, sizeof(int));
    agregar_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo)+1);
    socket_memoria = operacion_con_memoria();
    enviar_paquete(paquete, socket_memoria, logger);
    cerrar_conexion_memoria(socket_memoria);
    borrar_paquete(paquete);
    log_trace(logger, "Proceso inicial (%s), tamanio [%d]", nombre_archivo, tamanio_proceso);

}

void terminar_programa(int conexion, t_log* logger, t_config* config){
    config_destroy(config);
    log_destroy(logger);
}