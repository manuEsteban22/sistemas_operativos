#include <utils/utils.h>
#include <conexion_cpu.h>

void handshake_kernel(int socket){
    t_paquete* handshake = crear_paquete();
    enviar_paquete(handshake, socket);
    //enviar_buffer
    //destruir_buffer
    return;
}

int conectar_dispatch(char* ip, char* puerto_dispatch){
    struct addrinfo hints;
    struct addrinfo *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    //guardo en server_info los datos para conectar el socket dispatch
    getaddrinfo(ip, puerto_dispatch, &hints, &server_info);
    int socket_dispatch = socket(server_info->ai_family,
                                server_info->ai_socktype,
                                server_info->ai_protocol);
    
    connect(socket_dispatch, server_info->ai_addr, server_info->ai_addrlen);
    freeaddrinfo(server_info);
    //handshake_kernel(socket_dispatch);
    return socket_dispatch;
}
