#ifndef CONEXION_KERNEL_H_
#define CONEXION_KERNEL_H_
#include<utils/utils.h>
#include<kernel.h>
#include<syscalls.h>
#include<planificador_largo_plazo.h>//por las colas nomas

void* manejar_servidor_cpu(void* arg);
void* manejar_servidor_io(int socket_io);
bool handshake_memoria(int socket);
int conectar_memoria(char* ip, char* puerto);
int operacion_con_memoria();
void cerrar_conexion_memoria(int socket);
#endif