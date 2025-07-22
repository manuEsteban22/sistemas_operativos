#ifndef CONEXIONES_MEMORIA_H_
#define CONEXIONES_MEMORIA_H_
#include <memoria.h>
#include <atencion_kernel.h>


extern t_dictionary* semaforos_por_pid; // key: char* PID, value: sem_t*
extern t_dictionary* iniciados_por_pid;
extern pthread_mutex_t mutex_semaforos;

void* manejar_servidor(void* socket_cliente_ptr);
void* atender_cpu(void* socket_ptr);
void* lanzar_servidor(int socket_servidor);

#endif