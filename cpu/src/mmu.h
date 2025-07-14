#ifndef MMU_H_
#define MMU_H_

#include<cpu.h>
#include<tlb_cache.h>

int traducir_direccion(t_pcb* pcb, int direccion_fisica);
int calcular_entrada_nivel(int direccion_logica, int nivel, int entradas_por_tabla, int cant_niveles, t_pcb* pcb);

#endif