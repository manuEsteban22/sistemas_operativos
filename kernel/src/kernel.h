#ifndef KERNEL_H_
#define KERNEL_H_
#include<utils/utils.h>

extern t_log* logger;
extern int PROCESOS_MEMORIA;
extern int socket_memoria;
extern int socket_io;
extern t_dictionary* dispositivos_io;
extern t_dictionary* tabla_pcbs;

t_pcb* obtener_pcb(int pid);
typedef struct
{
    int socket;
    char* nombre;
} t_args_hilo;

typedef struct {
    int socket_io;
    t_queue* cola_bloqueados;
    bool ocupado;
} t_dispositivo_io;

#endif