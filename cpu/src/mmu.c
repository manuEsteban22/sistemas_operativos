#include<mmu.h>

/*
cpu recibe de memoria como parte del handshake:
tamaño_pagina
entradas_por_tabla
cantidad_niveles
*/

int calcular_entrada_nivel(int direccion_logica, int nivel, int entradas_por_tabla, int cant_niveles, t_pcb* pcb){
    int nro_pagina = direccion_logica / tam_pagina;
    return (nro_pagina / (int)pow(entradas_por_tabla, (cant_niveles - nivel - 1))) % entradas_por_tabla;
}

int traducir_direccion(t_pcb* pcb, int direccion_logica){
    int nro_pagina = direccion_logica / tam_pagina;
    log_trace(logger, "nada 1");
    int marco = esta_en_tlb(nro_pagina);
    log_trace(logger, "nada 2");
    if(marco == -1){ //TLB MISS
        log_info(logger, "PID: %d - TLB MISS - Pagina: %d", pcb->pid, nro_pagina);
        marco = pedir_frame(pcb, nro_pagina);
    }

    int desplazamiento = direccion_logica % tam_pagina;

    // if(entradas_cache > 0 && esta_en_cache(nro_pagina, &marco)){
    //     log_info(logger, "PID: %d - Cache Hit - Pagina: %d", pcb->pid, nro_pagina);
    //     usleep(retardo_cache);
    //     return marco * tam_pagina + desplazamiento;
    // }

    //if(entradas_tlb > 0 && esta_en_tlb(nro_pagina)){
    //    log_info(logger, "PID: %d - TLB HIT - Pagina: %d", pcb->pid, nro_pagina);
    //    return marco * tam_pagina + desplazamiento;
    //}
    

    if(entradas_tlb > 0){
        actualizar_tlb(nro_pagina, marco);
    }

    return marco * tam_pagina + desplazamiento;
}