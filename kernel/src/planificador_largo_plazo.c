#include<planificador_largo_plazo.h>

t_queue* cola_new;
t_queue* cola_ready;

pthread_mutex_t mutex_new = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_ready = PTHREAD_MUTEX_INITIALIZER;

sem_t sem_procesos_en_new;
sem_t sem_procesos_en_memoria;

void inicializar_planificador_lp(){
    cola_new = queue_create();
    cola_ready = queue_create();
    sem_init(&sem_procesos_en_new, 0, 0);
    sem_init(&sem_procesos_en_memoria, 0, PROCESOS_MEMORIA);
}

void crear_proceso(int tamanio_proceso){

}

void planificador_largo_plazo(){
    while(1){
        sem_wait(&sem_procesos_en_new);
        sem_wait(&sem_procesos_en_memoria);
        //chequeo que haya procesos en new y que haya espacio en memoria con dos wait

        pthread_mutex_lock(&mutex_new);
        t_pcb* pcb = queue_pop(cola_new);
        pthread_mutex_unlock(&mutex_new);
        //hago pop al proximo proceso de cola 

        /*
        inicializar_proceso_en_memoria(pcb);
        cambiar_estado(pcb, READY);
        pthread_mutex_lock(&mutex_ready);
        queue_push(cola_ready, pcb);
        pthread_mutex_unlock(&mutex_ready);
        */
    }
}