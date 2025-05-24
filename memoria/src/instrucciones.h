#ifndef INSTRUCCIONES_H_
#define INSTRUCCIONES_H_


void devolver_instrucciones(int pid);
char* obtener_instruccion(int pc);
int cantidad_instrucciones();
int espacio_libre();
void liberar_memoria();
int mandar_instrucciones();







#endif

