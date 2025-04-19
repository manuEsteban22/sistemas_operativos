#include <utils/utils.h>

t_log* logger;
int fd_escucha;

void* atender_cliente(void* fd_conexion_ptr) {
    int socket_cliente = *((int*)fd_conexion_ptr);
    free(fd_conexion_ptr);

    log_info(logger, "se conectó un cliente");

    close(socket_cliente);

    return NULL;
}

int iniciar_servidor(char* PUERTO){
    int socket_servidor;

    struct addrinfo hints, *server_info, *p;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, PUERTO, &hints, &server_info);

    fd_escucha = socket(server_info->ai_family,
                        server_info->ai_socktype,
                        server_info->ai_protocol);


    setsockopt(fd_escucha, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int));

    bind(fd_escucha,server_info->ai_addr,server_info->ai_addrlen);

    listen(fd_escucha, SOMAXCONN);
    freeaddrinfo(server_info);
    log_info(logger, "Listo para escucha");
    return fd_escucha;
}

int esperar_cliente(int socket_servidor){
    int socket_cliente;
    socket_cliente = accept(socket_servidor, NULL, NULL);
    log_info(logger, "se conecto un cliente");
    return socket_cliente;
}
int esperar_clientes_multiplexado(int socket_servidor){
    int nuevo_socket = accept(socket_servidor, NULL, NULL);
    if(nuevo_socket == -1){
        log_error(logger, "error en el accept");
    }
        pthread_t thread;
        int *fd_conexion_ptr = malloc(sizeof(int));
        *fd_conexion_ptr = nuevo_socket;
        pthread_create(&thread,
                       NULL,
                       atender_cliente,
                       fd_conexion_ptr);
    pthread_detach(thread);
}




int recibir_operacion(int socket_cliente){
    int cod_op;
    if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
        return cod_op;
    else
    {
        close(socket_cliente);
        return -1;
    }
}

int crear_conexion(char* ip, char* puerto){
    struct addrinfo hints;
    struct addrinfo *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(ip, puerto, &hints, &server_info);

    int socket_cliente = socket(server_info->ai_family,
                                server_info->ai_socktype,
                                server_info->ai_protocol);
    
    connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);

    freeaddrinfo(server_info);

    return socket_cliente;
}