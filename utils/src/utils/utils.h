#ifndef UTILS_H_
#define UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<commons/collections/queue.h>
#include<commons/collections/dictionary.h>
#include<readline/readline.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<pthread.h>
#include<semaphore.h>
#include<commons/bitarray.h>


typedef enum
{
    CERRADO,
    HANDSHAKE,
    HANDSHAKE_CPU_MEMORIA,
    PAQUETE,
    MENSAJE,
    OK,
    NO,
    FETCH,
    OC_WRITE,
    OC_READ,
    OC_EXEC,
    OC_INIT,
    OC_INTERRUPT,
    OC_SUSP,
    OC_DESUSP,
    SYSCALL_IO,
    SYSCALL_INIT,
    SYSCALL_DUMP_MEMORY,
    SYSCALL_EXIT,
    SOLICITUD_IO,
    SOLICITUD_DUMP_MEMORY,
    FINALIZA_IO,
    MEMORY_DUMP,
    OC_FRAME,
    OC_PAG_WRITE,
    OC_PAG_READ,
    ESPACIO_DISPONIBLE,
    KILL_PROC,
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

void* atender_cliente(void* fd_conexion_ptr, t_log* logger);
int iniciar_servidor(char* puerto, t_log* logger);
int esperar_cliente(int socket_servidor, t_log* logger);
int crear_conexion(char* ip, char* puerto);

t_paquete* crear_paquete(void);
void crear_buffer(t_paquete*);
void agregar_a_paquete(t_paquete*, void* contenido, int tamanio);
void enviar_paquete(t_paquete*, int socket, t_log* logger);
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