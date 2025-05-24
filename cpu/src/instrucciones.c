#include<instrucciones.h>

/*cpu recibe de memoria como parte del handshake:
tamaño_pagina
entradas_por_tabla
cantidad_niveles
*/
int tamanio_pagina = 64;
int entradas_por_tabla = 4;
int cantidad_niveles = 3;

t_instruccion* fetch(t_pcb* pcb, int socket_memoria){
    int pc = pcb->pc;
    int pid = pcb->pid;
    
    send(socket_memoria, &pid, sizeof(int), 0);
    send(socket_memoria, &pc, sizeof(int), 0);

    t_instruccion* proxima_instruccion;// = recibir_instruccion(socket_memoria);
    return proxima_instruccion;
}

bool requiere_traduccion(t_instruccion* instruccion){
    if(instruccion == WRITE || instruccion == READ){
        return true;
    }
    else{
        return false;
    }
}

void decode(t_instruccion* instruccion, t_pcb* pcb){
    if(!requiere_traduccion(instruccion)) return;

    int direccion_logica = instruccion->param1;
    int nro_pagina = direccion_logica / tamanio_pagina;
    int desplazamiento = direccion_logica % tamanio_pagina;
    int marco;

    if(entradas_tlb > 0 && encuentro_tlb(nro_pagina, &marco)){
        
    }

}