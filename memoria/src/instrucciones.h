#ifndef INSTRUCCIONES_H_
#define INSTRUCCIONES_H_


void cargar_instrucciones(int pid);
char* obtener_instruccion(int pc);
int cantidad_instrucciones();
int espacio_libre();
void liberar_memoria();
int mandar_instrucciones(int socket_cliente);








#endif

