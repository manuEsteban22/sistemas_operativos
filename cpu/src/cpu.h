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
extern int enradas_tlb;


#endif