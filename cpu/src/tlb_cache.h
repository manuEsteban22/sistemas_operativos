#ifndef TLB_CACHE_H_
#define TLB_CACHE_H_
#include<utils/utils.h>
#include<cpu.h>


extern t_list* tlb;
extern t_list* cache;

typedef struct {
    int pagina;
    int marco;
} t_entrada_tlb;

// typedef struct {
//     int pid;
//     int pagina;
// } t_paquete_frame;

typedef struct{
    int pagina;
    int marco;
    void* contenido;
    bool modificado;
    bool usado;
} t_entrada_cache;

bool esta_en_tlb(int pagina, int* marco);
int pedir_frame(t_pcb* pcb, int nro_pagina, int socket_memoria);

#endif