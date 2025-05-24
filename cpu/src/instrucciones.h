#ifndef INSTRUCCIONES_H_
#define INSTRUCCIONES_H_
#include<utils/utils.h>
#include<cpu.h>
#include<conexion_cpu.h>

extern t_list* tlb;

typedef enum {
    NOOP,
    WRITE,
    READ,
    GOTO,
    IO,
    INIT_PROC,
    DUMP_MEMORY,
    EXIT
} t_id_instruccion;

typedef struct {
    t_id_instruccion identificador;
    void* param1;
    void* param2;
} t_instruccion;


typedef struct {
    int pagina;
    int marco;
} t_entrada_tlb;

typedef struct {
    int pid;
    int pagina;
} t_paquete_frame;

#endif