
//int tamanio_pagina = 64;
//int entradas_por_tabla = 4;
//int cantidad_niveles = 3;
//t_list* tlb;
/*bool esta_en_tlb(int pagina, int* marco){
    for(int i=0;i < list_size(tlb);i++){
        t_entrada_tlb* entrada = list_get(tlb, i);
        if(entrada->pagina == pagina){
            *marco = entrada->marco;
            return true;
        }
    }
    return false;
}*/

/*void limpiar_tlb() {
    list_destroy_and_destroy_elements(tlb, free);
    tlb = list_create();
}*/

/*int pedir_frame(t_pcb* pcb, int nro_pagina, int marco, int socket_memoria){
    t_paquete* paquete = crear_paquete();
    t_paquete_frame* contenido;
    contenido->pagina = nro_pagina;
    contenido->pid = pcb->pid;
    agregar_a_paquete(paquete, contenido, sizeof(t_paquete_frame));
}*/