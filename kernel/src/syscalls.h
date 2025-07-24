#ifndef SYSCALLS_H_
#define SYSCALLS_H_
#include<utils/utils.h>
#include<kernel.h>
#include<pcb.h>

typedef struct {
    int pid;
    int tiempo;
} t_pcb_io;

typedef struct {
    int pid;
    int socket_memoria;
} t_args_dump;

void llamar_a_io(int socket_cpu);
void dump_memory(int socket_cpu);
void iniciar_proceso(int socket_cpu);
void ejecutar_exit(int socket_cpu);
void enviar_finalizacion_a_memoria(int pid);
void manejar_finaliza_io(int socket_io);

#endif