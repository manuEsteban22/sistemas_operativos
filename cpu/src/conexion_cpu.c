#include <conexion_cpu.h>

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
    }else {
        log_error(logger, "Fallo en el handshake (%s), recibí %d", nombre, respuesta);
    }

    return;
}

int conectar_kernel(char* ip, char* puerto, char* nombre, int cpu_id){
    struct addrinfo hints;
    struct addrinfo *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    //guardo en server_info los datos para conectar el socket dispatch/interrupt
    getaddrinfo(ip, puerto, &hints, &server_info);
    int fd_socket = socket(server_info->ai_family,
                                server_info->ai_socktype,
                                server_info->ai_protocol);
    
    connect(fd_socket, server_info->ai_addr, server_info->ai_addrlen);
    freeaddrinfo(server_info);
    //envio y recibo un handhsake a kernel
    handshake_kernel(fd_socket, nombre);
    send(fd_socket, CPUID, sizeof(int), 0);
    send(fd_socket, &cpu_id, sizeof(int), 0);
    return fd_socket;
}
