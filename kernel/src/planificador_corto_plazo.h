#ifndef PLANIFICADOR_CORTO_PLAZO_H_
#define PLANIFICADOR_CORTO_PLAZO_H_

#include <planificador_largo_plazo.h>
#include <pthread.h>
#include <commons/collections/queue.h>
#include <pcb.h>

t_pcb* planificador_corto_plazo ();
t_pcb* planificar_sjf_sin_desalojo(t_queue* cola);
void* planificador_corto_plazo_loop(int socket_dispatch);
#endif