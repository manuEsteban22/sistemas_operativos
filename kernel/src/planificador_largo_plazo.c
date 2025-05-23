#include<planificador_largo_plazo.h>

t_queue* cola_new;
t_queue* cola_ready;

pthread_mutex_t mutex_new = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_ready = PTHREAD_MUTEX_INITIALIZER;

sem_t sem_procesos_en_new;
sem_t sem_procesos_en_memoria;
//procesos en memoria se refiere a la cantidad de procesos permitidos en memoria simultaneamente

int pid_global = 0;

void inicializar_planificador_lp(){
    cola_new = queue_create();
    cola_ready = queue_create();
    sem_init(&sem_procesos_en_new, 0, 0);
    sem_init(&sem_procesos_en_memoria, 0, PROCESOS_MEMORIA);
}

void crear_proceso(int pid, int tamanio_proceso){
    t_pcb* pcb = crear_pcb(pid_global++, tamanio_proceso);

    pthread_mutex_lock(&mutex_new);
    queue_push(cola_new, pcb);
    pthread_mutex_unlock(&mutex_new);
    sem_post(&sem_procesos_en_new);
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

        if(enviar_pedido_memoria(pcb)){
            cambiar_estado(pcb, READY);

            pthread_mutex_lock(&mutex_ready);
            queue_push(cola_ready, pcb);
            pthread_mutex_unlock(&mutex_ready);

        } else{
            pthread_mutex_lock(&mutex_new);
            queue_push(cola_new, pcb);
            pthread_mutex_unlock(&mutex_new);

            sem_post(&sem_procesos_en_new);
        }
        /*
        inicializar_proceso_en_memoria(pcb);
        cambiar_estado(pcb, READY);
        pthread_mutex_lock(&mutex_ready);
        queue_push(cola_ready, pcb);
        pthread_mutex_unlock(&mutex_ready);
        */
    }
}

bool enviar_pedido_memoria(t_pcb* pcb) {
    t_paquete* paquete = crear_paquete();
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    agregar_a_paquete(paquete, &(pcb->tamanio), sizeof(int));

    enviar_paquete(paquete, socket_memoria);
    borrar_paquete(paquete);

    int respuesta = recibir_operacion(socket_memoria);
    if (respuesta == 3) {
        procesos_en_memoria++;
        return true;
    } else {
        return false;
    }
}
