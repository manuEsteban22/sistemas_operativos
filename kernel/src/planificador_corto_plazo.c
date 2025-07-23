#include <planificador_corto_plazo.h>
#include <planificador_largo_plazo.h>
#include <kernel.h>

t_algoritmo_planificacion algoritmo_cp;
sem_t cpus_disponibles;

t_algoritmo_planificacion parsear_algoritmo_cp(char* algoritmo){
    if (strcmp(algoritmo, "FIFO") == 0) return FIFO;
    if (strcmp(algoritmo, "PMCP") == 0) return PMCP;
    if (strcmp(algoritmo, "SJF_SIN_DESALOJO") == 0) return SJF_SIN_DESALOJO;
    if (strcmp(algoritmo, "SJF_CON_DESALOJO") == 0) return SJF_CON_DESALOJO;

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
        case SJF_SIN_DESALOJO:
            pcb = planificar_sjf_sin_desalojo(cola_ready);
            log_info(logger, "%d Desalojado por algoritmo SJF", pcb->pid);
            break;
        case SJF_CON_DESALOJO:
            pcb = planificar_sjf_sin_desalojo(cola_ready);
            log_info(logger, "%d Desalojado por algoritmo SRT", pcb->pid);
            // probablemente despues de cada proceso hay que actualizar la
            // estimacion y la rafaga real anterior y asi quedan los valores para el que sigue
            break;
    }
    //pcb = queue_pop(cola_ready);//borrar
    pthread_mutex_unlock(&mutex_ready);
    log_trace(logger, "pcp saco un proceso de ready");
    return pcb;
}

t_pcb* planificar_sjf_sin_desalojo(t_queue* cola){

    if(queue_is_empty(cola)){
        return NULL;
    }

    t_list* lista_aux = list_create();
    t_pcb* pcb_menor = NULL;

    while(!queue_is_empty(cola)){
        t_pcb* pcb_actual = queue_pop(cola);
        list_add(lista_aux, pcb_actual);

        if(pcb_menor == NULL || pcb_actual->estimacion_rafaga < pcb_menor->estimacion_rafaga){
            pcb_menor = pcb_actual;
        }  
    }

    for(int i = 0; i < list_size(lista_aux); i++){
            t_pcb* pcb = list_get(lista_aux, i);

            if(pcb != pcb_menor){
                queue_push(cola, pcb);
            }
    }

    list_destroy(lista_aux);
    return pcb_menor;    
}

void ejecutar_proceso(t_pcb* pcb, int socket_dispatch, int cpu_id){
    int estado_anterior;

    estado_anterior = pcb->estado_actual;
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
    dictionary_put(tabla_exec, pid_str, cpu_id_ptr);
    pthread_mutex_unlock(&mutex_exec);
    //free(cpu_id_ptr);
    //free(pid_str);

    log_info(logger, "envie el proceso PID=%d a CPU - PC=%d", (pcb->pid), (pcb->pc));
}

void* planificador_corto_plazo_loop(int socket_dispatch) {
    log_trace(logger, "arranque a correr pcp");
    while (1) {
        sem_wait(&sem_procesos_ready);
        log_trace(logger, "arranque una vuelta de pcp");
        t_pcb* pcb = planificador_corto_plazo();  // elige un PCB de READY
        if (pcb == NULL) {
            log_trace(logger, "el pcb que agarro de ready es null");
            continue;
        }

        sem_wait(&cpus_disponibles);
        if(queue_is_empty(cpus_libres)){
            log_error(logger, "No hay CPUs disponibles");
        }
        pthread_mutex_lock(&mutex_cpus_libres);
        int* cpu_id_ptr = queue_pop(cpus_libres);
        pthread_mutex_unlock(&mutex_cpus_libres);

        if (cpu_id_ptr == NULL) {
            log_error(logger, "No se encontró la CPU asignada al PCB");
            continue;
        }
        int cpu_id = *cpu_id_ptr;

        log_warning(logger,"cpu id: %d", cpu_id);//borrar

        char* cpu_id_str = string_itoa(cpu_id);
        int* socket_dispatch_ptr = dictionary_get(tabla_dispatch, cpu_id_str);
        free(cpu_id_str);

        if (tabla_dispatch == NULL) {
            log_error(logger, "Tabla dispatch no inicializada");
            continue;
        }

        if (socket_dispatch_ptr == NULL) {
            log_error(logger, "No se encontró el socket dispatch para CPU %d", cpu_id);
            continue;
        }
        //log_trace(logger, "la cpu corresponde al socket: %d segun pcp", *socket_dispatch_ptr);
        

        
        ejecutar_proceso(pcb, *socket_dispatch_ptr, cpu_id);
        log_trace(logger, "termino una vuelta pcp");
    }
    return NULL;
}