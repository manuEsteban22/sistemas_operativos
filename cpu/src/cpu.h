#ifndef CPU_H_
#define CPU_H_
#include<utils/utils.h>

extern t_log* logger;
extern int enradas_tlb;

typedef struct
{
    int pid;
    int pc;
    int tamanio;
    //t_list* instrucciones;
} t_pcb;

#endif