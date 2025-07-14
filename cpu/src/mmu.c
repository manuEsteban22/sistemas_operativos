#include<mmu.h>

int calcular_entrada_nivel(int direccion_logica, int nivel, int entradas_por_tabla, int cant_niveles, t_pcb* pcb){
    int nro_pagina = direccion_logica / tam_pagina;
    return (nro_pagina / (int)pow(entradas_por_tabla, (cant_niveles - nivel - 1))) % entradas_por_tabla;
}

int traducir_direccion(t_pcb* pcb, int direccion_logica){

    if (tam_pagina <= 0) {
        log_error(logger, "ERROR: tam_pagina inválido (%d)", tam_pagina);
        return -1;
    }
    int nro_pagina = direccion_logica / tam_pagina;
    int desplazamiento = direccion_logica % tam_pagina;

    int marco = esta_en_tlb(nro_pagina);

    if(entradas_tlb > 0 && marco != -1){
       log_info(logger, "PID: %d - TLB HIT - Pagina: %d", pcb->pid, nro_pagina);
       return marco * tam_pagina + desplazamiento;
    }

    if(marco == -1){ //TLB MISS
        log_info(logger, "PID: %d - TLB MISS - Pagina: %d", pcb->pid, nro_pagina);
        marco = pedir_frame(pcb, nro_pagina);

        if(entradas_tlb > 0){
        actualizar_tlb(nro_pagina, marco);
        }
    }

    

    // if(entradas_cache > 0 && esta_en_cache(nro_pagina, &marco)){
    //     log_info(logger, "PID: %d - Cache Hit - Pagina: %d", pcb->pid, nro_pagina);
    //     usleep(retardo_cache);
    //     return marco * tam_pagina + desplazamiento;
    // }

 
    

    

    return marco * tam_pagina + desplazamiento;
}