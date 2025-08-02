#ifndef PLANIFICADOR_MEDIANO_PLAZO_H_
#define PLANIFICADOR_MEDIANO_PLAZO_H_

#include<utils/utils.h>
#include<kernel.h>
#include<pcb.h>
//#include<conexion_kernel.h>


void planificador_mediano_plazo();
void informar_memoria_suspension(int pid);
void inicializar_planificador_mp();
int operacion_con_memoria();
void cerrar_conexion_memoria(int socket);

#endif