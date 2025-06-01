#ifndef CONEXION_MEMORIA_CPU_H_
#define CONEXION_MEMORIA_CPU_H_

#include <memoria.h>

void* manejar_servidor(int socket_servidor);
void* atender_cpu(void* socket_ptr);

#endif