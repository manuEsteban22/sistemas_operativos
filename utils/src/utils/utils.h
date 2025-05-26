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


typedef enum
{
    CERRADO,
    HANDSHAKE,
    PAQUETE,
    MENSAJE,
    OK,
    OC_WRITE,
    OC_READ,
    OC_EXEC,
    SYSCALL_IO,
    SYSCALL_INIT,
    SYSCALL_DUMP_MEMORY,
    SYSCALL_EXIT,
    SOLICITUD_IO,
    FINALIZA_IO,
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
t_list* deserializar(t_buffer* buffer);
void enviar_handshake(int socket_cliente);
t_list* recibir_paquete (int socket_cliente);
t_paquete* cambiar_opcode_paquete(t_paquete* paquete, op_code codigo);
void enviar_mensaje(int socket, char* mensaje);
char* recibir_mensaje(int socket);

#endif