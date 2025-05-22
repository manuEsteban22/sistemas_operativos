#include<utils/utils.h>
#include<pcb.h>
#include<kernel.h>

extern t_queue* cola_new;
extern t_queue* cola_ready;

extern pthread_mutex_t mutex_new;
extern pthread_mutex_t mutex_ready;

extern sem_t sem_procesos_en_new;
extern sem_t sem_procesos_en_memoria;