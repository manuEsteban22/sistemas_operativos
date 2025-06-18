#include <planificador_corto_plazo.h>
#include <planificador_largo_plazo.h>
#include <kernel.h>

t_algoritmo_planificacion algoritmo_cp;

t_algoritmo_planificacion parsear_algoritmo_cp(char* algoritmo){
    if (strcmp(algoritmo, "FIFO") == 0) return FIFO;
    if (strcmp(algoritmo, "MENOR_MEMORIA") == 0) return MENOR_MEMORIA;
    if (strcmp(algoritmo, "SJF_SIN_DESALOJO") == 0) return SJF_SIN_DESALOJO;
    if (strcmp(algoritmo, "SJF_CON_DESALOJO") == 0) return SJF_CON_DESALOJO;

    log_error(logger, "Algoritmo inválido: %s", algoritmo);
    exit(EXIT_FAILURE);
}

//algoritmo_cp = parsear_algoritmo_cp(algoritmo_planificacion_cp);

t_pcb* planificador_corto_plazo(){
    pthread_mutex_lock(&mutex_ready);

    if (queue_is_empty(cola_ready)){
        pthread_mutex_unlock(&mutex_ready);
        return NULL;
    }

    t_pcb* pcb = NULL;

    switch(algoritmo_cp){
        case FIFO:
            pcb = queue_pop(cola_ready);
            break;
        case SJF_SIN_DESALOJO:
            pcb = planificar_sjf_sin_desalojo(cola_ready);
            break;
        case SJF_CON_DESALOJO:
            break;
    }
    
    pthread_mutex_unlock(&mutex_ready);

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

double calculo_estimacion (t_pcb* actual, t_config* config){
    double alpha = config_get_double_value(config, "ALPHA");
    double estimacion = (alpha * pcb->rafaga_real_anterior) + ((1 - alpha) * pcb->estimacion_rafaga);
}

void ejecutar_proceso(t_pcb* pcb, int socket_dispatch){
    cambiar_estado(pcb, EXEC);
    int pid = pcb->pid;
    int pc = pcb->pc;

    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OC_EXEC);
    agregar_a_paquete(paquete, pid, sizeof(int));
    agregar_a_paquete(paquete, pc, sizeof(int));
    enviar_paquete(paquete, socket_dispatch, logger);
    borrar_paquete(paquete);

    log_info(logger, "envie el proceso PID=%d a CPU - PC=%d", pid, pc);
}