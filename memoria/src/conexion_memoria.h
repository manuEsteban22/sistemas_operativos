#ifndef CONEXION_MEMORIA_H_
#define CONEXION_MEMORIA_H_
#include<utils/utils.h>
#include<memoria.h>

void* manejar_servidor(int socket_servidor);
void* atender_clientes(void* socket_ptr);

#endif