#ifndef KERNEL_H_
#define KERNEL_H_
#include<utils/utils.h>
#include<pcb.h>
#include<planificador_mediano_plazo.h>

extern t_log* logger;
extern char* ip_memoria;
extern char* puerto_memoria;
extern char* puerto_interrupt;
extern char* puerto_dispatch;
extern int socket_memoria;
extern int socket_io;
extern int socket_interrupt;
extern int socket_dispatch;
extern int alfa;
extern int estimacion_inicial;
extern t_dictionary* dispositivos_io; //
extern t_dictionary* tabla_pcbs; // tiene pcb -> pid
extern t_dictionary* tabla_exec; // tiene pid -> cpuid
extern t_dictionary* tabla_dispatch; // tiene cpuid -> socket dispatch
extern t_dictionary* tabla_interrupt; // tiene cpuid -> socket interrupt
extern t_queue* cpus_libres; // tiene los cpuid libres
extern pthread_mutex_t mutex_cpus_libres;
extern pthread_mutex_t mutex_exec;
extern char* algoritmo_largo_plazo;
extern char* algoritmo_corto_plazo;
extern int tiempo_suspension;

void iniciar_estructuras();

typedef struct
{
    int socket;
    char* nombre;
} t_args_hilo;

typedef struct {
    int socket_io;
    t_queue* cola_bloqueados;
    bool ocupado;
} t_dispositivo_io;

#endif