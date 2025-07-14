#ifndef TLB_CACHE_H_
#define TLB_CACHE_H_
#include<utils/utils.h>
#include<cpu.h>


extern t_list* tlb;
extern t_list* cache;

typedef struct {
    int pagina;
    int marco;
    int ultimo_acceso;
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

// extern t_list* cache;
extern int puntero_clock;

void inicializar_tlb();
void inicializar_cache();
int esta_en_tlb(int pagina);
int pedir_frame(t_pcb* pcb, int nro_pagina);
void* leer_de_cache(int direccion_logica, int tamanio, t_pcb* pcb);

#endif