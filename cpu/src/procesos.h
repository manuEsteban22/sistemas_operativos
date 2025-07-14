#ifndef PROCESOS_H_
#define PROCESOS_H_
#include<ciclo_instrucciones.h>
#include<utils/utils.h>
#include<cpu.h>

bool esperar_procesos(int cpu_id);
void bucle_esperar_procesos(int cpu_id);

#endif