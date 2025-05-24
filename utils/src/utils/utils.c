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

    socket_servidor = socket(server_info->ai_family,
                        server_info->ai_socktype,
                        server_info->ai_protocol);


    setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int));

    bind(socket_servidor,server_info->ai_addr,server_info->ai_addrlen);

    listen(socket_servidor, SOMAXCONN);
    freeaddrinfo(server_info);
    log_info(logger, "Listo para escucha");
    return socket_servidor;
}

int esperar_cliente(int socket_servidor){
    int socket_cliente;
    socket_cliente = accept(socket_servidor, NULL, NULL);
    log_info(logger, "se conecto un cliente");
    return socket_cliente;
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

void* serializar(t_paquete* paquete, int bytes_a_enviar){
    //meto el contenido del paquete en un unico stream de datos para hacer el send
    //se serializa con el formato opcode / size / contenido
    void* stream_a_enviar = malloc(bytes_a_enviar);
    int offset = 0;

    memcpy(stream_a_enviar + offset, &(paquete->codigo_operacion), sizeof(int));
    offset += sizeof(int);

    memcpy(stream_a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);

    if(paquete->buffer->size > 0 && paquete->buffer->stream != NULL){
        memcpy(stream_a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);
        offset += paquete->buffer->size;
    }
    
    return stream_a_enviar;
}

t_list* deserializar(t_buffer* buffer){
    void* stream = buffer->stream;
    int size = buffer->size;


    t_list* elementos = list_create();
    int offset = 0;

    while(offset < size){
        int tamanio_elemento = 0;

        memcpy(&tamanio_elemento, stream + offset, sizeof(int));
        offset += sizeof(int);

        void* elemento = malloc(tamanio_elemento);

        memcpy(elemento, stream + offset, tamanio_elemento);
        offset += tamanio_elemento;

        list_add(elementos,elemento);
    }
    
    return elementos;
}

t_list* recibir_paquete (int socket_cliente){
    t_buffer* buffer = malloc(sizeof(t_buffer));
    t_list* contenido;

    recv(socket_cliente, &(buffer->size), sizeof(int), MSG_WAITALL);
    
    buffer->stream = malloc(buffer->size);

    recv(socket_cliente, buffer->stream, buffer->size, MSG_WAITALL);

    contenido = deserializar(buffer);

    free(buffer->stream);
    free(buffer);
    return contenido;
}

void crear_buffer(t_paquete* paquete){
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = 0;
    paquete->buffer->stream = NULL;
}

t_paquete* crear_paquete(void){
    t_paquete* paquete;
    paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = PAQUETE;
    crear_buffer(paquete);
    return paquete;
}

/*void enviar_handshake(int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = HANDSHAKE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	borrar_paquete(paquete);
}*/

void enviar_handshake(int socket_cliente){
    int cod_op = HANDSHAKE;    
    send(socket_cliente, &cod_op, sizeof(int), 0);
}

void agregar_a_paquete(t_paquete* paquete, void* contenido, int tamanio){
    paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

    memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
    memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), contenido, tamanio);

    paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(t_paquete* paquete, int socket){
    int bytes_a_enviar = paquete->buffer->size + 2*sizeof(int);
    void* stream_a_enviar = serializar(paquete, bytes_a_enviar);

    send(socket, stream_a_enviar, bytes_a_enviar, 0);
    free(stream_a_enviar);
}

int recibir_operacion(int socket_cliente){
    int codigo_operacion;
    int resultado = recv(socket_cliente, &codigo_operacion, sizeof(int), MSG_WAITALL);
    if(resultado > 0){
        return codigo_operacion;
    }
    else if(resultado == 0){
        //el cliente cerro la conexion
        close(socket_cliente);
        return 0;
    }
    else{
        close(socket_cliente);
        return -1;
    }
}

/*void* recibir_buffer(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}*/

void borrar_paquete(t_paquete* paquete){
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}