#ifndef INSTRUCCIONES_H_
#define INSTRUCCIONES_H_
#include<ciclo_instrucciones.h>
#include<utils/utils.h>
#include<cpu.h>
#include<conexion_cpu.h>


void ejecutar_write(t_instruccion* instruccion, int direccion_fisica, t_pcb* pcb);
char* ejecutar_read(t_instruccion* instruccion, int direccion_fisica, t_pcb* pcb);
void ejecutar_io(t_instruccion* instruccion, t_pcb* pcb);
void init_proc(t_instruccion* instruccion, t_pcb* pcb);
void dump_memory(t_pcb* pcb);
void exit_syscall(t_pcb* pcb);

#endif