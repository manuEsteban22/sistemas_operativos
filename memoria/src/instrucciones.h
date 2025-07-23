#ifndef INSTRUCCIONES_H_
#define INSTRUCCIONES_H_

#include <commons/collections/dictionary.h>

extern t_dictionary* lista_de_instrucciones_por_pid;
void cargar_instrucciones(int pid, char* nombre_archivo);
char* obtener_instruccion(int pc, char* pid);
int cantidad_instrucciones(char* pid);
int espacio_libre();
void liberar_memoria(char* pid);
int mandar_instruccion(int socket_cliente);
int mandar_frame(int socket_cliente);
int ejecutar_write(int socket_cliente);
int ejecutar_read(int socket_cliente);
int leer_pagina_completa(int socket_cliente);
int escribir_pagina_completa(int socket_cliente);
int obtener_marco(int pid, int nro_pagina);
void dumpear_memoria(int pid, int socket_cliente);
char* obtener_timestamp();

#include <config_memoria.h>
#include <memoria.h>
#include <administracion_memoria.h>

#endif