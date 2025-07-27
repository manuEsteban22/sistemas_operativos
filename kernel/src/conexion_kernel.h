#ifndef CONEXION_KERNEL_H_
#define CONEXION_KERNEL_H_
#include<kernel.h>


void* manejar_servidor_cpu(void* arg);
void enviar_interrupcion_a_cpu(int socket_interrupt);
void* hilo_main_cpu(void* args);
void* hilo_main_io(void* args);
void* manejar_servidor_io(void* arg);
bool handshake_memoria(int socket);
int conectar_memoria(char* ip, char* puerto);
int operacion_con_memoria();
void cerrar_conexion_memoria(int socket);
t_instancia_io* obtener_instancia_disponible(t_dispositivo_io* dispositivo);

#include<utils/utils.h>
#include<planificador_largo_plazo.h>
#include<syscalls.h>
#endif