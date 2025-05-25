#ifndef CONEXION_IO_H_
#define CONEXION_IO_H_
#include<utils/utils.h>
#include<io.h>

int conectar_kernel(char* ip, char* puerto, char* nombre_dispositivo);
void atender_solicitudes_io(int socket_kernel);
void enviar_finalizacion_io(int socket_kernel, int pid);

#endif