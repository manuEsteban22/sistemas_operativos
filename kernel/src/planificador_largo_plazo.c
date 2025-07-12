#include<planificador_largo_plazo.h>
#include <commons/collections/list.h>

int procesos_en_memoria = 0;
t_algoritmo_planificacion enum_algoritmo_lp;
pthread_mutex_t mutex_procesos_en_memoria = PTHREAD_MUTEX_INITIALIZER;

t_queue* cola_new;
t_queue* cola_ready;
t_queue* cola_susp_ready;
t_queue* cola_susp_blocked;
t_queue* cola_blocked;

pthread_mutex_t mutex_new = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_ready = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_blocked = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_susp_blocked = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_susp_ready = PTHREAD_MUTEX_INITIALIZER;

sem_t sem_procesos_en_new;
sem_t sem_procesos_en_memoria;
sem_t sem_procesos_ready;
sem_t sem_procesos_en_blocked;

//procesos en memoria se refiere a la cantidad de procesos permitidos en memoria simultaneamente

int pid_global = 0;

void chequear_algoritmo_planificacion(char* algoritmo_largo_plazo){
    if (strcmp(algoritmo_largo_plazo,"FIFO") == 0){
    enum_algoritmo_lp = FIFO;
    } else if(strcmp(algoritmo_largo_plazo,"MENOR_MEMORIA") == 0){
    enum_algoritmo_lp = MENOR_MEMORIA;
    } else{
    log_info(logger,"Algoritmo de planificación invalido: %s",algoritmo_largo_plazo);
    exit(EXIT_FAILURE);
    }
}

void inicializar_planificador_lp(char* algoritmo_largo_plazo){
    printf("Presione Enter para iniciar la planificacion largo plazo\n");
    getchar();
    chequear_algoritmo_planificacion(algoritmo_largo_plazo);
    cola_new = queue_create();
    cola_ready = queue_create();
    cola_susp_ready = queue_create();
    cola_blocked = queue_create();
    sem_init(&sem_procesos_en_new, 0, 0);
    sem_init(&sem_procesos_en_memoria, 0, PROCESOS_MEMORIA);
    sem_init(&sem_procesos_ready, 0, 0);
    sem_init(&sem_procesos_en_blocked, 0, 0);
}

//para testear por ahora ponemos dsp tamanio_proceso 256 por ejemplo (o una potencia de 2)
void crear_proceso(int tamanio_proceso){
    log_trace(logger, "Se creo un proceso");
    t_pcb* pcb = crear_pcb(pid_global, tamanio_proceso);
    pid_global++;
    char* pid_str = string_itoa(pcb->pid);
    dictionary_put(tabla_pcbs, pid_str, pcb);
    free(pid_str);

    pthread_mutex_lock(&mutex_new);

    switch(enum_algoritmo_lp){
        case FIFO:
            queue_push(cola_new, pcb);
            break;
        case MENOR_MEMORIA:
            insertar_en_orden_por_memoria(cola_new, pcb);
            break;
        case SJF_SIN_DESALOJO:
            log_warning(logger, "no se usa sjf sin desalojo en planificador largo plazo");
            exit(EXIT_FAILURE);
            break;
        case SJF_CON_DESALOJO:
            log_warning(logger, "no se usa sjf con desalojo en planificador largo plazo");
            exit(EXIT_FAILURE);
            break;
        default:
            log_warning(logger, "No hay un algoritmo adecuado en planificador largo plazo");
            exit(EXIT_FAILURE);
    }
    pthread_mutex_unlock(&mutex_new);
    sem_post(&sem_procesos_en_new);
}

void insertar_en_orden_por_memoria(t_queue* cola, t_pcb* nuevo){
    t_list* lista_aux = list_create();
    bool insertado = false;

    while (!queue_is_empty(cola)){
        t_pcb* actual = queue_pop(cola);

        if (!insertado && nuevo->tamanio < actual->tamanio){
            list_add(lista_aux, nuevo); 
            insertado = true;
        }
        list_add(lista_aux, actual);
    }

    if (!insertado){
        list_add(lista_aux, nuevo); 
    }

    for (int i = 0; i < list_size(lista_aux); i++){
        t_pcb* pcb = list_get(lista_aux, i);
        queue_push(cola, pcb);
    }

    list_destroy(lista_aux);
}


void finalizar_proceso(t_pcb* pcb){
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
    log_trace(logger, "Se envio un pedido a memoria");
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OC_INIT);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    agregar_a_paquete(paquete, &(pcb->tamanio), sizeof(int));

    socket_memoria = operacion_con_memoria();
    enviar_paquete(paquete, socket_memoria, logger);
    borrar_paquete(paquete);
    log_trace(logger,"Estoy esperando respuesta de espacio disponible");
    int respuesta = recibir_operacion(socket_memoria);
    cerrar_conexion_memoria(socket_memoria);
    if (respuesta == OK) {
        pthread_mutex_lock(&mutex_procesos_en_memoria);
        procesos_en_memoria++;
        pthread_mutex_unlock(&mutex_procesos_en_memoria);
        log_trace(logger, "Habia suficiente espacio");
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
        //pcb = crear_pcb(pid, tamanio);

        log_trace(logger, "aca arranco el plani lp");

        pthread_mutex_lock(&mutex_susp_ready);
        if(!queue_is_empty(cola_susp_ready)){

            pcb = queue_peek(cola_susp_ready);

            if(enviar_pedido_memoria(pcb)){
                queue_pop(cola_susp_ready);
                pthread_mutex_unlock(&mutex_susp_ready);

                cambiar_estado(pcb, READY);

                pthread_mutex_lock(&mutex_ready);
                queue_push(cola_ready, pcb);
                sem_post(&sem_procesos_ready);
                pthread_mutex_unlock(&mutex_ready);
                continue;

            } else{
                pthread_mutex_unlock(&mutex_susp_ready);
                sem_post(&sem_procesos_en_new);
                sem_post(&sem_procesos_en_memoria);
                continue;
            }

        } else{
            pthread_mutex_unlock(&mutex_susp_ready);
        }

        pthread_mutex_lock(&mutex_new);

        if (!queue_is_empty(cola_new)){
            log_trace(logger, "plp hay procesos en new");
            pcb = queue_peek(cola_new);

            if(enviar_pedido_memoria(pcb)){//me fijo si puedo ejecutar el proximo proceso y lo paso a cola de ready
                queue_pop (cola_new);
                pthread_mutex_unlock(&mutex_new);

                cambiar_estado(pcb, READY);

                pthread_mutex_lock(&mutex_ready);
                queue_push(cola_ready, pcb);
                sem_post(&sem_procesos_ready);
                pthread_mutex_unlock(&mutex_ready);

            } else{
                pthread_mutex_unlock(&mutex_new);
                sem_post(&sem_procesos_en_new);
                sem_post(&sem_procesos_en_memoria);
            }
        } 
        
        else {
            pthread_mutex_unlock(&mutex_new);
            log_trace(logger, "la cola de new esta vacia");
            
            sem_post(&sem_procesos_en_new);
            sem_post(&sem_procesos_en_memoria);
        }

    //inicializar_proceso_en_memoria(pcb);
}
}