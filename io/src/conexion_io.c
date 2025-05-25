#include <conexion_io.h>

void handshake_kernel(int socket, char* nombre){
    enviar_handshake(socket);
    log_info(logger, "envié el handshake (%s)", nombre);

    int respuesta;
    if(0 >= recv(socket, &respuesta, sizeof(int), MSG_WAITALL)){
        log_error(logger, "Fallo al recibir OK de kernel (%s)", nombre);
        return;
    }
    if(respuesta == OK){
        log_info(logger, "Recibi el OK de kernel (%s)", nombre);
        return;
    }else {
        log_error(logger, "Fallo en el handshake (%s), recibí %d", nombre, respuesta);
        return;
    }
}

int conectar_kernel(char* ip, char* puerto, char* nombre, int io_id){
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
    handshake_kernel(fd_socket, nombre);

    send(fd_socket, &io_id, sizeof(int), 0);
    return fd_socket;
}

void atender_solicitudes_io(int socket_kernel){
    while (1) {
    int cod_op = recibir_operacion(socket_kernel);
    if (cod_op <= 0) break;

        switch(cod_op) {
            case SOLICITUD_IO: {
                t_list* contenido = recibir_paquete(socket_kernel);
                int* pid = list_get(contenido, 0);
                int* tiempo_io = list_get(contenido, 1);

                log_info(logger, "PID: %d - Inicio de IO - Tiempo: %d", *pid, *tiempo_io);
                usleep(*tiempo_io * 1000);
                log_info(logger, "PID: %d - Fin de IO", *pid);

                list_destroy_and_destroy_elements(contenido, free);
                break;
            }   
            default:
                log_error(logger, "Código de operación desconocido: %d", cod_op);
                break;
        }
    }
}