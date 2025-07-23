#include<pcb.h>
#include<planificador_corto_plazo.h>

t_pcb* crear_pcb(int pid, int tamanio_proceso) {
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->pid = pid;
    pcb->pc = 0;
    pcb->tamanio = tamanio_proceso;
    pcb->estado_actual = NEW;
    pcb->temporal_estado = temporal_create();
    //Inicializo el pcb en estado NEW y creo el cronometro

    for(int i = 0; i < CANTIDAD_ESTADOS; i++) {
        pcb->metricas_estado[i] = 0;
        pcb->metricas_tiempo[i] = 0;
        //inicializo ambos arrays en 0 para cada estado
    }

    pcb->metricas_estado[NEW]++;//sumo 1 a las veces que pasó por estado NEW

    pcb->estimacion_rafaga = estimacion_inicial;
    pcb->rafaga_real_anterior = 0;

    log_info(logger, "%d Se crea el proceso - Estado: NEW", pcb->pid);

    return pcb;
}

//para ver a que pcb esta asignado dicho pid
t_pcb* obtener_pcb(int pid) {
    char* pid_str = string_itoa(pid);
    t_pcb* pcb = dictionary_get(tabla_pcbs, pid_str);
    if (pcb == NULL) {
        log_error(logger, "Error para obtener PCB NULL");
    }
    free(pid_str);
    return pcb;
}

void cambiar_estado(t_pcb* pcb, t_estado_proceso nuevo_estado) {
    if (pcb == NULL) {
        log_error(logger, "Error en cambio de estado PCB NULL");
        return;
    }

    int duracion = temporal_gettime(pcb->temporal_estado);
    pcb->metricas_tiempo[pcb->estado_actual] += duracion;
    //devuelve el tiempo que tomó el cronometro y lo suma a la metrica de tiempo del estado actual

    pcb->estado_actual = nuevo_estado;
    pcb->metricas_estado[nuevo_estado]++;
    //cambia el estado del pcb y suma 1 a la metrica de estado del estado nuevo

    if (nuevo_estado == READY && (strcmp(algoritmo_corto_plazo, "SRT") == 0)){
    chequear_sjf_con_desalojo(pcb);
    }

    temporal_destroy(pcb->temporal_estado);
    pcb->temporal_estado = temporal_create();
    //borra el cronometro del estado anterior y crea uno nuevo
}

void borrar_pcb(t_pcb* pcb){
    if (pcb == NULL) return;
    temporal_destroy(pcb->temporal_estado);
    free(pcb);
}

void actualizar_estimacion_rafaga(t_pcb* pcb) {
    if (pcb == NULL) {
        // devolver el valor inicial de la estimacion o un error (?)
        return;
    }

    double rafaga_real = temporal_gettime(pcb->temporal_estado);
    pcb->rafaga_real_anterior = rafaga_real;

    log_error(logger, "Se actualizo la estimacion de PID %d - Rafaga %f - Estimacion previa %f", pcb->pid, pcb->rafaga_real_anterior, pcb->estimacion_rafaga);
    double nueva_estimacion = (alfa * rafaga_real) + ((1 - alfa) * pcb->estimacion_rafaga);
    pcb->estimacion_rafaga = nueva_estimacion;
    log_error(logger, "Nueva estimacion = %f", nueva_estimacion);
    temporal_destroy(pcb->temporal_estado);
    pcb->temporal_estado = temporal_create();
}

void chequear_sjf_con_desalojo(t_pcb* nuevo) {
    if (algoritmo_corto_plazo != "SRT"){
        return;
    }

        

    // if (!hay_proceso_en_exec()){
    //     return;
    // }

    //t_pcb* ejecutando = obtener_proceso_en_ejecucion();

    // if (nuevo->estimacion_rafaga < ejecutando->estimacion_rafaga) {
    //     enviar_interrupcion_a_cpu();
    //     // y hay que replanificar
    // }
}


// bool hay_proceso_en_exec(){
//     bool hay_exec = false;
    
//     void buscar_exec(char* key, void* pcb){
//         if (((t_pcb*) pcb)->estado_actual == EXEC)
//             hay_exec = true;
//     }

//     dictionary_iterator(tabla_pcbs, buscar_exec);
//     return hay_exec;
// }

void asignar_timer_blocked(t_pcb* pcb){
    if (pcb->temporal_blocked != NULL) {
        temporal_destroy(pcb->temporal_blocked);
    }
    log_error(logger, "COSA 1");
    pcb->temporal_blocked = temporal_create();
    sem_post(&sem_procesos_en_blocked);
}

//saca un pcb de la cola de susp_blocked mediante el pid
t_pcb* sacar_pcb_de_cola(t_queue* cola, int pid){
    t_list* aux = list_create();
    t_pcb* pcb_encontrado = NULL;

    while (!queue_is_empty(cola)){
        t_pcb* actual = queue_pop(cola);

        if (actual->pid == pid){
            pcb_encontrado = actual;
        } else {
            list_add(aux, actual);
        }
    }

    for (int i = 0; i < list_size(aux); i++) {
        queue_push(cola, list_get(aux, i));
    }

    list_destroy(aux);
    return pcb_encontrado;
}

char* parsear_estado(int estado){
    switch (estado) {
        case NEW: return "NEW";
        case READY: return "READY";
        case EXEC: return "EXEC";
        case BLOCKED: return "BLOCKED";
        case SUSP_BLOCKED: return "SUSP_BLOCKED";
        case SUSP_READY: return "SUSP_READY";
        case EXIT: return "EXIT";
        default: return "DESCONOCIDO";
    }
}

void log_metricas_estado(t_pcb* pcb){
    if (pcb == NULL) return;

    log_info(logger, "%d - Métricas de estado:", pcb->pid);
    log_info(logger, "NEW (%d) (%d)", pcb->metricas_estado[NEW], (int)pcb->metricas_tiempo[NEW]);
    log_info(logger, "READY (%d) (%d)", pcb->metricas_estado[READY], (int)pcb->metricas_tiempo[READY]);
    log_info(logger, "EXEC (%d) (%d)", pcb->metricas_estado[EXEC], (int)pcb->metricas_tiempo[EXEC]);
    log_info(logger, "BLOCKED (%d) (%d)", pcb->metricas_estado[BLOCKED], (int)pcb->metricas_tiempo[BLOCKED]);
    log_info(logger, "SUSP_BLOCKED (%d) (%d)", pcb->metricas_estado[SUSP_BLOCKED], (int)pcb->metricas_tiempo[SUSP_BLOCKED]);
    log_info(logger, "SUSP_READY (%d) (%d)", pcb->metricas_estado[SUSP_READY], (int)pcb->metricas_tiempo[SUSP_READY]);
    log_info(logger, "EXIT (%d) (%d)", pcb->metricas_estado[EXIT], (int)pcb->metricas_tiempo[EXIT]);
}