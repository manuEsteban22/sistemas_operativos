#ifndef CONEXION_MEMORIA_CPU_H_
#define CONEXION_MEMORIA_CPU_H_

#include <memoria.h>

void* manejar_servidor(void* socket_cliente_ptr);
void* atender_cpu(void* socket_ptr);
void* lanzar_servidor(int socket_servidor);

#endif