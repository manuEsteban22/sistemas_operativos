#ifndef INSTRUCCIONES_H_
#define INSTRUCCIONES_H_


typedef struct
{
    int pc;
    char* instruccion;
} t_instruccion;

int devolver_instrucciones();

t_instruccion* get_instruccion(t_list* lista, int pc);

t_list* cargar_instrucciones(const char* rutaArchivo);









#endif

