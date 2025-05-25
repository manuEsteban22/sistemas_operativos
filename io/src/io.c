#include <utils/utils.c>
#include <conexion_io.h>

t_log* iniciar_logger();
t_config* iniciar_config();
t_log* logger;
t_config* config;
char* ip_kernel;
char* puerto_kernel;

int main(int argc, char* argv[]) {
    int socket_kernel;
    logger = iniciar_logger();

    if (argc < 2){
        log_error(logger, "Falta el nombre del dispositivo IO");
        return EXIT_FAILURE;
    }

    int nombre_dispositivo = atoi (argv[1]);
    log_info(logger, "Nombre del dispositivo IO: %d", nombre_dispositivo);
    config = iniciar_config();

    socket_kernel = conectar_kernel(ip_kernel, puerto_kernel, "IO", nombre_dispositivo);
    
    enviar_mensaje(socket_kernel, nombre_dispositivo);
    atender_solicitudes_io(socket_kernel);

    return 0;
}

t_log* iniciar_logger(void){
    t_log* nuevo_logger;
    nuevo_logger = log_create("io.log","LogIO",true,LOG_LEVEL_INFO);
    log_info(nuevo_logger, "Funciona logger IO :)");
    return nuevo_logger;
}

t_config* iniciar_config(void){
    t_config* nuevo_config = config_create("io.config");

    if(!config_has_property(nuevo_config, "IP_KERNEL") || !config_has_property(nuevo_config, "PUERTO_KERNEL")){
        log_error(logger, "Faltan valores en el archivo de config");
        config_destroy(nuevo_config);
        abort();
    }

    ip_kernel = config_get_string_value(nuevo_config, "IP_KERNEL");
    puerto_kernel = config_get_string_value(nuevo_config, "PUERTO_KERNEL");
    
    log_info(logger, "La IP del kernel es: %s", ip_kernel);
    log_info(logger, "El puerto del kernel es: %s", puerto_kernel);

    return nuevo_config;
}