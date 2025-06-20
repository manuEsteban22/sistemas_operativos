#ifndef CICLO_INSTRUCCIONES_H_
#define CICLO_INSTRUCCIONES_H_
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

#include<instrucciones.h>


void prueba(int socket_memoria, int socket_kernel_dispatch, int socket_kernel_interrupt);
void iniciar_ciclo_de_instrucciones(t_pcb* pcb, int socket_memoria, int socket_kernel_dispatch, int socket_kernel_interrupt);
t_instruccion* leerInstruccion(char* instruccion_raw);
//void execute(t_instruccion* instruccion, int socket_memoria, int socket_kernel_dispatch, t_pcb* pcb);


#endif