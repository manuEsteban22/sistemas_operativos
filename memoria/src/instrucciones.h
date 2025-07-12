#ifndef INSTRUCCIONES_H_
#define INSTRUCCIONES_H_
#include <memoria.h>

void cargar_instrucciones(int pid);
char* obtener_instruccion(int pc);
int cantidad_instrucciones();
int espacio_libre();
void liberar_memoria();
int mandar_instruccion(int socket_cliente);
//int mandar_instrucciones(int socket_cliente);
int ejecutar_write(int socket_cliente);
void dumpear_memoria();
int mandar_frame(int socket_cliente);




#endif