#include<mmu.h>
#include<math.h>

int calcular_entrada_nivel(int direccion_logica, int nivel, int entradas_por_tabla, int cant_niveles, t_pcb* pcb){
    int nro_pagina = direccion_logica / tam_pagina;
    return (nro_pagina / (int)pow(entradas_por_tabla, (cant_niveles - nivel - 1))) % entradas_por_tabla;
}

int traducir_direccion(t_pcb* pcb, int direccion_logica){
    /*
    toma una direccion logica, calcula el nro_pagina y pide a memoria el frame
    con el frame calcula la direccion fisica y la devuelve
    */

    if (tam_pagina <= 0) {
        log_error(logger, "ERROR: tam_pagina inválido (%d)", tam_pagina);
        return -1;
    }
    int nro_pagina = direccion_logica / tam_pagina;
    int desplazamiento = direccion_logica % tam_pagina;

    int marco = -1;

    if(entradas_tlb > 0){
        marco = esta_en_tlb(nro_pagina);
        if(marco != -1){
            log_info(logger, "PID: %d - TLB HIT - Pagina: %d", pcb->pid, nro_pagina);
            return marco * tam_pagina + desplazamiento;
        }
        else{
            marco = pedir_frame(pcb, nro_pagina);
            actualizar_tlb(nro_pagina, marco);
            log_info(logger, "PID: %d - TLB MISS - Pagina: %d", pcb->pid, nro_pagina);
            return marco * tam_pagina + desplazamiento;
        }
    }

    marco = pedir_frame(pcb, nro_pagina);

    if(marco == -1){
        log_error(logger, "Error: marco inválido para PID %d - Página %d", pcb->pid, nro_pagina);
        return -1;
    }

    return marco * tam_pagina + desplazamiento;
}