#include <conexiones_memoria.h>
#include <pthread.h>
t_dictionary* semaforos_por_pid; // key: char* PID, value: sem_t*
t_dictionary* iniciados_por_pid;
pthread_mutex_t mutex_semaforos;

// Función para manejar conexiones PERSISTENTES de CPU
void* manejar_conexion_cpu(void* arg) {
    int socket_cliente = *((int*)arg);
    free(arg);
    while(1) {
        int codigo_operacion = recibir_operacion(socket_cliente);
        if (codigo_operacion < 0) {
            log_warning(logger, "CPU desconectada");
            break;
        }

        switch(codigo_operacion) {
            case FETCH:
                log_trace(logger, "Llegó un FETCH");
                mandar_instruccion(socket_cliente);         
                break;
            case OC_READ:
                log_trace(logger, "recibi un read");
                ejecutar_read(socket_cliente);
                break;
            case OC_WRITE:
                log_trace(logger, "recibi un write");
                ejecutar_write(socket_cliente);
                break;
            case OC_FRAME:
                mandar_frame(socket_cliente);
                log_trace(logger, "mande el frame");
                break;
            case OC_PAG_WRITE:
                escribir_pagina_completa(socket_cliente);
                log_trace(logger, "Cache me escribio una pagina");
                break;
            case OC_PAG_READ:
                log_trace(logger, "Cache me pidio una pagina");
                leer_pagina_completa(socket_cliente);
                break;
            case CERRADO:
                log_trace(logger, "Se cerro la conexion con CPU");
                break;
            default:
                //log_error(logger, "Operación CPU desconocida: %d", codigo_operacion);
                break;
        }
    }
    close(socket_cliente);
    return NULL;
}

// Función para manejar conexiones EFÍMERAS de Kernel
void manejar_conexion_kernel(int socket_cliente) {
    int codigo_operacion = recibir_operacion(socket_cliente);
    t_list* recibido;
    int tamanio;
    char* pid_str;
    int pid;
    switch(codigo_operacion) {
        case OC_INIT:
            recibido = recibir_paquete(socket_cliente);
            pid = *((int*)list_get(recibido, 0));
            tamanio = *((int*)list_get(recibido, 1));
            char* nombre_archivo = list_get(recibido, 2);
            log_trace(logger, "Llego una peticion de crear un proceso");
            log_trace(logger, "Proceso PID=%d - Tamanio=%d", pid, tamanio);
            inicializar_proceso(tamanio, pid, nombre_archivo);
            pid_str = string_itoa(pid);
            sem_t* sem = malloc(sizeof(sem_t));
            sem_init(sem, 0, 0);
            pthread_mutex_lock(&mutex_semaforos);
            dictionary_put(semaforos_por_pid, pid_str, sem);
            dictionary_put(iniciados_por_pid, pid_str, true);
            pthread_mutex_unlock(&mutex_semaforos);
            sem_post(sem);
            t_paquete* paquete = crear_paquete();
            cambiar_opcode_paquete(paquete, OK);
            enviar_paquete(paquete, socket_cliente, logger);
            borrar_paquete(paquete);
            free(pid_str);
            list_destroy_and_destroy_elements(recibido, free);
            break;
        case ESPACIO_DISPONIBLE:
            recibido = recibir_paquete(socket_cliente);
            pid = *((int*)list_get(recibido, 0));
            tamanio = *((int*)list_get(recibido, 1));
            pid_str = string_itoa(pid);
            log_trace(logger, "Kernel me pregunto si tenia espacio disponible - PID:(%s) - Tamanio: %d", pid_str, tamanio);
            if (dictionary_has_key(tablas_por_pid, pid_str)) {
                log_trace(logger, "PID %s ya tiene estructuras cargadas", pid_str);
                free(pid_str);
                t_paquete* paquete = crear_paquete();
                cambiar_opcode_paquete(paquete, OK);
                enviar_paquete(paquete, socket_cliente, logger);
                borrar_paquete(paquete);
                list_destroy_and_destroy_elements(recibido, free);
                //free(pid_str);
                break;
            }else{
               if(entra_el_proceso(tamanio, pid)){
                t_paquete* paquete = crear_paquete();
                cambiar_opcode_paquete(paquete, OK);
                enviar_paquete(paquete, socket_cliente, logger);
                borrar_paquete(paquete);
               }else{
                log_trace(logger, "El proceso no entraba en memoria");
                t_paquete* paquete = crear_paquete();
                cambiar_opcode_paquete(paquete, NO);
                enviar_paquete(paquete, socket_cliente, logger);
                borrar_paquete(paquete);
               }
                list_destroy_and_destroy_elements(recibido, free);
                free(pid_str);
            }
            break;
        case KILL_PROC:
            recibido = recibir_paquete(socket_cliente);
            pid = *((int*)list_get(recibido, 0));
            finalizar_proceso(pid);
            t_paquete* confirmacion = crear_paquete();
            cambiar_opcode_paquete(confirmacion, OK);
            enviar_paquete(confirmacion, socket_cliente, logger);
            borrar_paquete(confirmacion);
            list_destroy_and_destroy_elements(recibido, free);
            break;
        case SOLICITUD_DUMP_MEMORY:
            t_list* recibido_pid_dump = recibir_paquete(socket_cliente);
            int pid_dumpeo = *((int*)list_get(recibido_pid_dump, 0));
            log_info(logger, "## PID: %d - Memory Dump solicitado", pid_dumpeo);
            dumpear_memoria(pid_dumpeo, socket_cliente);
            list_destroy_and_destroy_elements(recibido_pid_dump, free);
            break;
        case OC_SUSP:
            log_info(logger, "Recibi solicitud de hacer una suspension");
            t_list* recibido = recibir_paquete(socket_cliente);
            int* pid_ptr = list_get(recibido, 0);
            int pid = *pid_ptr;
            
            suspender_proceso(pid);

            list_destroy_and_destroy_elements(recibido, free);
            break;
        case OC_DESUSP:
            log_info(logger, "Recibi solicitud de hacer una desuspension");
            recibido = recibir_paquete(socket_cliente);
            int* pid_recibido = list_get(recibido, 0);
            pid = *pid_recibido;

            des_suspender_proceso(pid);

            t_paquete* confirmacion_desusp = crear_paquete();
            cambiar_opcode_paquete(confirmacion_desusp, OK);
            enviar_paquete(confirmacion_desusp, socket_cliente, logger);
            borrar_paquete(confirmacion_desusp);

            list_destroy_and_destroy_elements(recibido, free);
            break;
        default:
            log_trace(logger, "Operación Kernel desconocida: %d", codigo_operacion);
            break;
    }
}

void* manejar_conexiones_memoria(void* socket_ptr) {
    int socket_cliente = *((int*)socket_ptr);
    free(socket_ptr);

    int codigo_operacion = recibir_operacion(socket_cliente);
    
    if (codigo_operacion == HANDSHAKE_CPU_MEMORIA) {
        log_trace(logger, "Recibi el handshake de una CPU");
        t_paquete* paquete = crear_paquete();
        cambiar_opcode_paquete(paquete, OK);

        agregar_a_paquete(paquete, &(campos_config.entradas_por_tabla), sizeof(int));
        agregar_a_paquete(paquete, &(campos_config.tam_pagina), sizeof(int));
        agregar_a_paquete(paquete, &(campos_config.cantidad_niveles), sizeof(int));
        enviar_paquete(paquete, socket_cliente, logger);
        borrar_paquete(paquete);
    

        int* socket_cpu = malloc(sizeof(int));
        *socket_cpu = socket_cliente;
        log_trace(logger, "socket cpu: %d", *socket_cpu);
        
        pthread_t hilo_cpu;
        pthread_create(&hilo_cpu, NULL, (void*)manejar_conexion_cpu, socket_cpu);
        pthread_detach(hilo_cpu);
        return NULL;
    }
    else if (codigo_operacion == HANDSHAKE) {
        log_info(logger, "## Kernel Conectado - FD del socket: %d", socket_cliente);
        op_code respuesta = OK;
        send(socket_cliente, &respuesta, sizeof(int), 0);
        
        manejar_conexion_kernel(socket_cliente);
        close(socket_cliente);
        return NULL;
    }
    else {
        log_error(logger, "Handshake invalido: %d", codigo_operacion);
        close(socket_cliente);
        return NULL;
    }
}

void* manejar_servidor(void* socket_ptr) 
{
    //por cada accept esta funcion tira un hilo
    int socket_servidor = *((int*)socket_ptr);
    free(socket_ptr);
    while (1) {
        int socket_cliente = esperar_cliente(socket_servidor, logger);

        int* socket_cliente_ptr = malloc(sizeof(int));
        *socket_cliente_ptr = socket_cliente;

        pthread_t hilo_cliente;
        pthread_create(&hilo_cliente, NULL, manejar_conexiones_memoria, socket_cliente_ptr);
        pthread_detach(hilo_cliente);
    }
    return NULL;
}

void* lanzar_servidor(int socket_servidor)
{
    //este es el hilo main que lanza todas las conexiones
    pthread_t hilo_conexion;

    int* socket_ptr = malloc(sizeof(int));
    *socket_ptr = socket_servidor;

    pthread_create(&hilo_conexion, NULL, manejar_servidor, socket_ptr);
    pthread_detach(hilo_conexion);

    return NULL;
}

