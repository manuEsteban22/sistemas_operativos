#ifndef CONEXION_KERNEL_H_
#define CONEXION_KERNEL_H_
#include<utils/utils.h>

void* manejar_servidor_cpu(void* arg);
void* manejar_servidor_io(void* arg);

#endif