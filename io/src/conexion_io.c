#include <conexion_io.h>

void handshake_kernel(int socket){
    enviar_handshake(socket);
    log_info(logger, "envié el handshake a kernel");

    int respuesta;
    if(0 >= recv(socket, &respuesta, sizeof(int), MSG_WAITALL)){
        log_error(logger, "Fallo al recibir OK de kernel");
        return;
    }
    if(respuesta == OK){
        log_info(logger, "Recibi el OK de kernel");
        return;
    }else {
        log_error(logger, "Fallo en el handshake, recibí %d", respuesta);
        return;
    }
}

int conectar_kernel(char* ip, char* puerto, char* nombre_dispositivo){
    struct addrinfo hints;
    struct addrinfo *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    //guardo en server_info los datos para conectar el io
    getaddrinfo(ip, puerto, &hints, &server_info);
    int fd_socket = socket(server_info->ai_family,
                                server_info->ai_socktype,
                                server_info->ai_protocol);
    
    connect(fd_socket, server_info->ai_addr, server_info->ai_addrlen);
    freeaddrinfo(server_info);
    //envio y recibo un handhsake a kernel
    handshake_kernel(fd_socket);
    enviar_mensaje(fd_socket, nombre_dispositivo);
    log_info(logger, "Envié el nombre de dispositivo");
    return fd_socket;
}

void atender_solicitudes_io(int socket_kernel){
    while (1) {
        int cod_op = recibir_operacion(socket_kernel);
        if (cod_op <= 0){
            log_error(logger, "Esta cerrado %d", cod_op);
            return;
        }

            switch(cod_op) {
                case SOLICITUD_IO: {
                    t_list* contenido = recibir_paquete(socket_kernel);
                    int* pid = list_get(contenido, 0);
                    int* tiempo_io = list_get(contenido, 1);
                    int* cpu_id = list_get(contenido, 2);
                    char* dispositivo = list_get(contenido, 3);

                    log_debug(logger, "Recibi el CPU ID %d", *cpu_id);
                    log_info(logger, "PID: %d - Inicio de IO - Tiempo: %d", *pid, *tiempo_io);
                    usleep(*tiempo_io * 1000);
                    log_info(logger, "PID: %d - Fin de IO", *pid);
                    enviar_finalizacion_io(socket_kernel, *pid, *cpu_id, dispositivo);
                    list_destroy_and_destroy_elements(contenido, free);
                    break;
                }   
                default:
                    log_error(logger, "Código de operación desconocido: %d", cod_op);
                    break;
            }
    }
}

void enviar_finalizacion_io(int socket_kernel, int pid, int cpu_id, char* nombre_dispositivo){
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, FINALIZA_IO);
    agregar_a_paquete(paquete, &pid, sizeof(int));
    agregar_a_paquete(paquete, nombre_dispositivo, strlen(nombre_dispositivo) + 1);
    agregar_a_paquete(paquete, &cpu_id, sizeof(int));
    enviar_paquete(paquete, socket_kernel, logger);
    borrar_paquete(paquete);
    log_trace(logger, "## Mande que finalizo IO, PID %d", pid);//no es log obligatorio
}