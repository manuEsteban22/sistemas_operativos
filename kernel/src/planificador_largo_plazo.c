#include<planificador_largo_plazo.h>
#include <commons/collections/list.h>

int procesos_en_memoria = 0;
t_algoritmo_planificacion algoritmo_lp;
t_algoritmo_planificacion algoritmo_cp;
pthread_mutex_t mutex_procesos_en_memoria = PTHREAD_MUTEX_INITIALIZER;

t_queue* cola_new;
t_queue* cola_ready;
t_queue* cola_susp_ready;

pthread_mutex_t mutex_new = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_ready = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_susp_ready = PTHREAD_MUTEX_INITIALIZER;

sem_t sem_procesos_en_new;
sem_t sem_procesos_en_memoria;
//procesos en memoria se refiere a la cantidad de procesos permitidos en memoria simultaneamente

int pid_global = 0;

void chequear_algoritmo_planificacion(char* algoritmo_planificacion_lp, char* algoritmo_planificacion_cp){
    if (strcmp(algoritmo_planificacion_lp,"FIFO") == 0){
    algoritmo_lp = FIFO;
    } else if(strcmp(algoritmo_planificacion_lp,"MENOR_MEMORIA") == 0){
    algoritmo_lp = MENOR_MEMORIA;
    } else{
    log_info(logger,"Algoritmo de planificación invalido: %s",algoritmo_planificacion_lp);
    exit(EXIT_FAILURE);
    }

    if (strcmp(algoritmo_planificacion_cp,"FIFO") == 0){
    algoritmo_cp = FIFO;
    } else if(strcmp(algoritmo_planificacion_cp,"MENOR_MEMORIA") == 0){
    algoritmo_cp = MENOR_MEMORIA;
    } else{
    log_info(logger,"Algoritmo de planificación invalido: %s",algoritmo_planificacion_cp);
    exit(EXIT_FAILURE);
    }
}

void inicializar_planificador_lp(char* algoritmo_planificacion_lp, char* algoritmo_planificacion_cp){
    printf("Presione Enter para iniciar la planificacion largo plazo\n");
    getchar();
    chequear_algoritmo_planificacion(algoritmo_planificacion_lp, algoritmo_planificacion_cp);
    cola_new = queue_create();
    cola_ready = queue_create();
    cola_susp_ready = queue_create();
    sem_init(&sem_procesos_en_new, 0, 0);
    sem_init(&sem_procesos_en_memoria, 0, PROCESOS_MEMORIA);
}

//para testear por ahora ponemos dsp tamanio_proceso 256 por ejemplo (o una potencia de 2)
void crear_proceso(int tamanio_proceso){
    t_pcb* pcb = crear_pcb(pid_global++, tamanio_proceso);
    char* pid_str = string_itoa(pcb->pid);
    dictionary_put(tabla_pcbs, pid_str, pcb);
    free(pid_str);

    pthread_mutex_lock(&mutex_new);

    switch (algoritmo_lp) {
        case FIFO:
            queue_push(cola_new, pcb);
            break;
        case MENOR_MEMORIA:
            // aca iria la funcion de ordenar por menor memoria
            break;
    }

    pthread_mutex_unlock(&mutex_new);
    sem_post(&sem_procesos_en_new);
}

void finalizar_proceso(t_pcb* pcb) {
    //enviar_finalizacion_a_memoria(pcb->pid);
    
    pthread_mutex_lock(&mutex_procesos_en_memoria);
    procesos_en_memoria--;
    pthread_mutex_unlock(&mutex_procesos_en_memoria);

    borrar_pcb(pcb);

    sem_post(&sem_procesos_en_memoria);
    sem_post(&sem_procesos_en_new);
    //loguear_metricas();
}


bool enviar_pedido_memoria(t_pcb* pcb) {
    t_paquete* paquete = crear_paquete();
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    agregar_a_paquete(paquete, &(pcb->tamanio), sizeof(int));

    enviar_paquete(paquete, socket_memoria, logger);
    borrar_paquete(paquete);

    int respuesta = recibir_operacion(socket_memoria);
    if (respuesta == OK) {
        pthread_mutex_lock(&mutex_procesos_en_memoria);
        procesos_en_memoria++;
        pthread_mutex_unlock(&mutex_procesos_en_memoria);
        return true;
    } else {
        return false;
    }
}

void planificador_largo_plazo(){
    while(1){
        //chequeo que haya procesos en new y que haya espacio en memoria con dos wait
        sem_wait(&sem_procesos_en_new);
        sem_wait(&sem_procesos_en_memoria);
        t_pcb* pcb = NULL;

        pthread_mutex_lock(&mutex_susp_ready);
        if(!queue_is_empty(cola_susp_ready)){

            pcb = queue_peek(cola_susp_ready);

            if(enviar_pedido_memoria(pcb)){
                queue_pop(cola_susp_ready);
                pthread_mutex_unlock(&mutex_susp_ready);

                cambiar_estado(pcb, READY);

                pthread_mutex_lock(&mutex_ready);
                queue_push(cola_ready, pcb);
                pthread_mutex_unlock(&mutex_ready);
                continue;
            }
            else{
                pthread_mutex_unlock(&mutex_susp_ready);
                sem_post(&sem_procesos_en_new);
                sem_post(&sem_procesos_en_memoria);
                continue;
            }
        } else{
            pthread_mutex_unlock(&mutex_susp_ready);
            }

        pthread_mutex_lock(&mutex_new);
        if (!queue_is_empty(cola_new)) {

        pcb = queue_peek(cola_new);
        if(enviar_pedido_memoria(pcb)){//me fijo si puedo ejecutar el proximo proceso y lo paso a cola de ready
            queue_pop (cola_new);
            pthread_mutex_unlock(&mutex_new);
            cambiar_estado(pcb, READY);
       
            pthread_mutex_lock(&mutex_ready);
            queue_push(cola_ready, pcb);
            pthread_mutex_unlock(&mutex_ready);
        } else{
            pthread_mutex_unlock(&mutex_new);
            sem_post(&sem_procesos_en_new);
            sem_post(&sem_procesos_en_memoria);
        }
    } else{
        pthread_mutex_unlock(&mutex_new);
        sem_post(&sem_procesos_en_new);
        sem_post(&sem_procesos_en_memoria);
    }
    //inicializar_proceso_en_memoria(pcb);
}
}