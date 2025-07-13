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
        return;
    }else {
        log_error(logger, "Fallo en el handshake (%s), recibí %d", nombre, respuesta);
        return;
    }

    return;
}
void handshake_memoria(int socket){
    int cod_op = HANDSHAKE_CPU_MEMORIA;    
    send(socket, &cod_op, sizeof(int), 0);
    log_info(logger, "envié el handshake a memoria");

    int respuesta = recibir_operacion(socket);
    if(respuesta <= 0){
        log_error(logger, "Fallo al recibir OK de memoria");
        return;
    }
    if(respuesta == OK){
        log_info(logger, "Recibi el OK de memoria");
        t_list* recibido = recibir_paquete(socket);
        entradas_por_tabla = *((int*)list_get(recibido, 0));
        tam_pagina = *((int*)list_get(recibido, 1));
        cant_niveles = *((int*)list_get(recibido, 2));
        log_trace(logger, "Entradas por tabla %d - Tamanio pagina %d - Cantidad niveles %d", entradas_por_tabla,tam_pagina, cant_niveles);
        list_destroy_and_destroy_elements(recibido, free);
        return;
    }else {
        log_error(logger, "Fallo en el handshake de memoria, recibí %d", respuesta);
        return;
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

    send(fd_socket, &cpu_id, sizeof(int), 0);
    return fd_socket;
}

int conectar_memoria(char* ip, char* puerto){
    struct addrinfo hints;
    struct addrinfo *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    //guardo en server_info los datos para conectar el socket
    getaddrinfo(ip, puerto, &hints, &server_info);
    int fd_socket = socket(server_info->ai_family,
                                server_info->ai_socktype,
                                server_info->ai_protocol);

    if (connect(fd_socket, server_info->ai_addr, server_info->ai_addrlen) == -1) {
        log_error(logger, "Error al conectar con memoria");
        return -1;
    }
    freeaddrinfo(server_info);
    //envio y recibo un handhsake a memoria
    handshake_memoria(fd_socket);

    return fd_socket;
}
