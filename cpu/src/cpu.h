#ifndef CPU_H_
#define CPU_H_
#include<utils/utils.h>

typedef struct
{
    int pid;
    int pc;
    int tamanio;
    //t_list* instrucciones;
} t_pcb;


#include<ciclo_instrucciones.h>
#include<procesos.h>

extern t_log* logger;
extern int entradas_tlb;
extern char* reemplazo_tlb;
extern int entradas_cache;
extern char* reemplazo_cache;
extern int retardo_cache;
extern int entradas_por_tabla;
extern int tam_pagina;
extern int cant_niveles;
extern int socket_kernel_dispatch;
extern int socket_memoria;
extern int socket_kernel_interrupt;

#endif