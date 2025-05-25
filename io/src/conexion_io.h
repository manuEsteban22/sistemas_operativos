#ifndef CONEXION_IO_H_
#define CONEXION_IO_H_
#include<utils/utils.h>
#include<io.h>

int conectar_kernel(char* ip, char* puerto, char* nombre, int io_id);
void atender_solicitudes_io(int socket_kernel);

#endif