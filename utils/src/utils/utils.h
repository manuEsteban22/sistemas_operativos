#ifndef UTILS_H_
#define UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<commons/collections/queue.h>
#include<readline/readline.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<pthread.h>
#include<semaphore.h>

//#include<conexion_cpu.c>

typedef enum
{
    CERRADO
    HANDSHAKE,
    PAQUETE,
    OK,
    ERROR
} op_code;
typedef struct
{
    int size;
    void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

int iniciar_servidor(char* puerto);
int esperar_cliente(int socket_servidor);
int crear_conexion(char* ip, char* puerto);

t_paquete* crear_paquete(void);
void crear_buffer(t_paquete*);
void agregar_a_paquete(t_paquete*, void* contenido, int tamanio);
void enviar_paquete(t_paquete*, int socket);
int recibir_operacion(int socket_cliente);
void borrar_paquete(t_paquete*);
void* serializar(t_paquete*, int bytes_a_enviar);

#endif