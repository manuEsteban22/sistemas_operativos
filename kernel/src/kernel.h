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
extern double alfa;
extern double estimacion_inicial;
extern t_dictionary* dispositivos_io; // tiene nombre del io -> struct io
extern t_dictionary* tabla_pcbs; // tiene pid -> pcb
extern t_dictionary* tabla_exec; // tiene pid -> cpuid
extern t_dictionary* tabla_dispatch; // tiene cpuid -> socket dispatch
extern t_dictionary* tabla_interrupt; // tiene cpuid -> socket interrupt
extern t_list* cpus_libres; // tiene los cpuid libres
extern pthread_mutex_t mutex_cpus_libres;
extern pthread_mutex_t mutex_exec;
extern pthread_mutex_t mutex_tabla_pcbs;
extern pthread_mutex_t mutex_interrupt;
extern pthread_mutex_t mutex_dispositivos;
extern char* algoritmo_largo_plazo;
extern char* algoritmo_corto_plazo;
extern int tiempo_suspension;

void iniciar_estructuras();
void proceso_arranque(int tamanio_proceso, char* nombre_archivo);

typedef struct
{
    int socket;
    char* nombre;
} t_args_hilo;

typedef struct {
    t_list* sockets_io;//lista de t_instancia_io
    t_queue* cola_bloqueados;
    pthread_mutex_t mutex_dispositivos;
} t_dispositivo_io;

typedef struct {
    int socket;
    bool ocupado;
    int pid_ocupado;
} t_instancia_io;

#endif