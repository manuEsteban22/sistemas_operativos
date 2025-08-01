#include <planificador_corto_plazo.h>
#include <planificador_largo_plazo.h>
#include <kernel.h>

t_algoritmo_planificacion algoritmo_cp;
sem_t cpus_disponibles;

t_algoritmo_planificacion parsear_algoritmo_cp(char* algoritmo){
    if (strcmp(algoritmo, "FIFO") == 0) return FIFO;
    if (strcmp(algoritmo, "PMCP") == 0) return PMCP;
    if (strcmp(algoritmo, "SJF") == 0) return SJF;
    if (strcmp(algoritmo, "SRT") == 0) return SRT;

    log_error(logger, "Algoritmo inválido: %s", algoritmo);
    exit(EXIT_FAILURE);
}



t_pcb* planificador_corto_plazo(){
    pthread_mutex_lock(&mutex_ready);

    if (queue_is_empty(cola_ready)){
        log_trace(logger, "la cola de ready estaba vacia");
        pthread_mutex_unlock(&mutex_ready);
        return NULL;
    }

    t_pcb* pcb = NULL;

    switch(parsear_algoritmo_cp(algoritmo_corto_plazo)){
        case FIFO:
            pcb = queue_pop(cola_ready);
            break;
        case SJF:
            pcb = planificar_sjf(cola_ready);
            log_info(logger, "%d Desalojado por algoritmo SJF", pcb->pid);
            break;
        case SRT:
            pcb = planificar_sjf(cola_ready);
            log_info(logger, "%d Desalojado por algoritmo SRT", pcb->pid);
            // probablemente despues de cada proceso hay que actualizar la
            // estimacion y la rafaga real anterior y asi quedan los valores para el que sigue
            break;
        case PMCP:
            log_error(logger, "Proceso mas chico primero no es un algoritmo valido de pcp");
            break;
    }
    pthread_mutex_unlock(&mutex_ready);
    log_trace(logger, "pcp saco un proceso de ready");
    return pcb;
}

t_pcb* planificar_sjf(t_queue* cola){

    if(queue_is_empty(cola)){
        return NULL;
    }

    t_list* lista_aux = list_create();
    t_pcb* pcb_menor = NULL;

    while(!queue_is_empty(cola)){
        t_pcb* pcb_actual = queue_pop(cola);
        
        list_add(lista_aux, pcb_actual);
        log_debug(logger, "El pcb actual es PID %d y tiene estimacion %f", pcb_actual->pid, pcb_actual->estimacion_rafaga);
        if(pcb_menor == NULL || pcb_actual->estimacion_rafaga < pcb_menor->estimacion_rafaga){
            pcb_menor = pcb_actual;
            log_debug(logger, "Asigna PCB menor a PID: %d con estimacion %f", pcb_menor->pid, pcb_menor->estimacion_rafaga);
        }  
    }

    for(int i = 0; i < list_size(lista_aux); i++){
        t_pcb* pcb = list_get(lista_aux, i);
        if(pcb != pcb_menor){
            queue_push(cola, pcb);
        }
    }

    list_destroy(lista_aux);
    //log_error(logger, "aca se hace un pop a cola de ready de PID %d", pcb_menor->pid);
    return pcb_menor;    
}

void ejecutar_proceso(t_pcb* pcb, int socket_dispatch, int cpu_id){
    int estado_anterior;

    estado_anterior = pcb->estado_actual;
    if(estado_anterior == BLOCKED){
        log_error(logger, "aca pasa esto");
    }
    cambiar_estado(pcb, EXEC);
    log_info(logger, "(%d) Pasa del estado %s al estado %s",pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));


    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OC_EXEC);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    agregar_a_paquete(paquete, &(pcb->pc), sizeof(int));
    enviar_paquete(paquete, socket_dispatch, logger);
    borrar_paquete(paquete);


    char* pid_str = string_itoa(pcb->pid);
    int* cpu_id_ptr = malloc(sizeof(int));
    *cpu_id_ptr = cpu_id;

    pthread_mutex_lock(&mutex_exec);
    int* viejo_valor = dictionary_get(tabla_exec, pid_str);
    if (viejo_valor != NULL) free(viejo_valor);

    dictionary_put(tabla_exec, pid_str, cpu_id_ptr);
    
    pthread_mutex_unlock(&mutex_exec);
    free(pid_str);//hay que ver si esto es corrrecto 
    //porque capaz libera memoria que se va a usar

    log_info(logger, "Envie el proceso PID=%d a CPU - PC=%d", (pcb->pid), (pcb->pc));
}

void* planificador_corto_plazo_loop(void* _) {
    log_trace(logger, "Arranqué el PCP");

    while (1) {
        sem_wait(&sem_procesos_ready);

        while (true) {
            log_trace(logger, "estoy dando una vuelta de pcp");
            pthread_mutex_lock(&mutex_cpus_libres);
            if (list_is_empty(cpus_libres)) {
                pthread_mutex_unlock(&mutex_cpus_libres);
                log_trace(logger, "La cola de cpus libres esta vacia");
                break;
            }
            pthread_mutex_unlock(&mutex_cpus_libres);
            log_debug(logger, "PCP: CPUs libres: %d - READY size: %d", list_size(cpus_libres), queue_size(cola_ready));

            pthread_mutex_lock(&mutex_ready);
            bool ready_empty = queue_is_empty(cola_ready);
            pthread_mutex_unlock(&mutex_ready);
            if (ready_empty) break;

            sem_wait(&cpus_disponibles); 

            t_pcb* pcb = planificador_corto_plazo();
            if (pcb == NULL) {
                log_trace(logger, "No había PCB en READY");
                sem_post(&cpus_disponibles); 
                break;
            }

            //log_warning(logger, "popeo una cpu de cpus libres");
            pthread_mutex_lock(&mutex_cpus_libres);
            int* cpu_id_ptr = list_remove(cpus_libres, 0);
            log_debug(logger, "La cola de CPUs libres tiene un tamaño de %d", list_size(cpus_libres));
            pthread_mutex_unlock(&mutex_cpus_libres);

            if (!cpu_id_ptr) {
                log_error(logger, "No hay CPU disponible aunque lo parezca");
                sem_post(&cpus_disponibles);
                continue;
            }

            int cpu_id = *cpu_id_ptr;
            free(cpu_id_ptr);

            char* cpu_id_str = string_itoa(cpu_id);
            int* socket_dispatch_ptr = dictionary_get(tabla_dispatch, cpu_id_str);
            free(cpu_id_str);

            if (!socket_dispatch_ptr) {
                log_error(logger, "No se encontró el socket dispatch para CPU %d", cpu_id);
                sem_post(&cpus_disponibles);
                continue;
            }

            ejecutar_proceso(pcb, *socket_dispatch_ptr, cpu_id);
        }
    }

    return NULL;
}
