#include <utils/utils.c>
#include <conexion_cpu.h>
t_log* iniciar_logger();
t_config* iniciar_config();
t_log* logger;
t_config* config;
char* ip_kernel;
char* puerto_kernel_dispatch;
char* puerto_kernel_interrupt;
char* ip_memoria;
char* puerto_memoria;

int main(int argc, char* argv[]) {
    int socket_kernel_dispatch, socket_kernel_interrupt, socket_memoria;
    if(argc < 2){
        //log_error(logger, "faltaron argumentos en la ejecucion");
        return EXIT_FAILURE;
    }
    int cpu_id = atoi(argv[1]);
    logger = iniciar_logger(cpu_id);
    config = iniciar_config();
    socket_kernel_dispatch = conectar_kernel(ip_kernel, puerto_kernel_dispatch, "DISPATCH", cpu_id);
    socket_kernel_interrupt = conectar_kernel(ip_kernel, puerto_kernel_interrupt, "INTERRUPT", cpu_id);

    socket_memoria = crear_conexion(ip_memoria, puerto_memoria);
    return 0;
}

t_log* iniciar_logger(int id){
    t_log* nuevo_logger;
    char* log_file = string_from_format("cpu%d.log", id);
    nuevo_logger = log_create(log_file,"LogCPU",true,LOG_LEVEL_INFO);
    log_info(nuevo_logger, "funciona logger cpu :)");
    free(log_file);
    return nuevo_logger;
}

t_config* iniciar_config(void){
    t_config* nuevo_config;
    nuevo_config = config_create("cpu.config");
    if(config_has_property(nuevo_config, "IP_KERNEL") &&
    config_has_property(nuevo_config, "PUERTO_KERNEL_DISPATCH") &&
    config_has_property(nuevo_config, "PUERTO_KERNEL_INTERRUPT") &&
    config_has_property(nuevo_config, "IP_MEMORIA") &&
    config_has_property(nuevo_config, "PUERTO_MEMORIA")){
        ip_kernel = config_get_string_value(nuevo_config, "IP_KERNEL");
        puerto_kernel_dispatch = config_get_string_value(nuevo_config, "PUERTO_KERNEL_DISPATCH");
        puerto_kernel_interrupt = config_get_string_value(nuevo_config, "PUERTO_KERNEL_INTERRUPT");
        ip_memoria = config_get_string_value(nuevo_config,"IP_MEMORIA");
        puerto_memoria = config_get_string_value(nuevo_config, "PUERTO_MEMORIA");
    }
    else{log_info(logger, "no se pudo leer el archivo de config");}
    log_info(logger, "la ip del kernel es: %s", ip_kernel);
    log_info(logger, "el puerto del kernel de dispatch es: %s", puerto_kernel_dispatch);
    log_info(logger, "el puerto del kernel de interrupt es: %s", puerto_kernel_interrupt);
    return nuevo_config;
}

void terminar_programa(int conexion, t_log* logger, t_config* config){
    config_destroy(config);
    log_destroy(logger);
}