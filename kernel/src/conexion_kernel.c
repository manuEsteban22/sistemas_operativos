#include <utils/utils.h>
#include <kernel.h>
#include <conexion_kernel.h>

void* manejar_servidor_cpu(void* arg){
    t_args_hilo* argumentos = (t_args_hilo*) arg;
    int socket = argumentos->socket;
    char* nombre_cliente = strdup(argumentos->nombre);
    free(arg);

    int socket_cliente = esperar_cliente(socket, logger);
    
    while(1){
        int codigo_operacion = recibir_operacion(socket_cliente);

        if(codigo_operacion == -1){
            log_info(logger, "Se cerró la conexion de %s", nombre_cliente);
            break;
        }

        log_info(logger, "[%s] Código de operación recibido: %d", nombre_cliente, codigo_operacion);

        switch (codigo_operacion){
            case CERRADO:
                log_info(logger, "Se cerro la conexion");
                break;
            case HANDSHAKE:
                int cpu_id;
                log_trace(logger, "recibi un handshake de cpu");
                op_code respuesta = OK;
                send(socket_cliente, &respuesta, sizeof(int),0);
                log_trace(logger, "Envie OK a %s", nombre_cliente);

                recv(socket_cliente, &cpu_id, sizeof(int), MSG_WAITALL);
                log_trace(logger, "Conexion de CPU ID: %d", cpu_id);

                if (strcmp(nombre_cliente, "DISPATCH") == 0) {
                    int* socket_dispatch_ptr = malloc(sizeof(int));
                    *socket_dispatch_ptr = socket_cliente;

                    char* cpu_id_str = string_itoa(cpu_id);
                    dictionary_put(tabla_dispatch, cpu_id_str, socket_dispatch_ptr);

                    int* cpu_id_ptr = malloc(sizeof(int));
                    *cpu_id_ptr = cpu_id;

                    pthread_mutex_lock(&mutex_cpus_libres);
                    queue_push(cpus_libres, cpu_id_ptr);
                    pthread_mutex_unlock(&mutex_cpus_libres);

                    log_trace(logger, "Socket DISPATCH guardado: %d", socket_cliente);
                    free(cpu_id_str);
                }else if(strcmp(nombre_cliente, "INTERRUPT") == 0){
                    int* socket_interrupt_ptr = malloc(sizeof(int));
                    *socket_interrupt_ptr = socket_cliente;

                    char* cpu_id_str = string_itoa(cpu_id);
                    dictionary_put(tabla_interrupt, cpu_id_str, socket_interrupt_ptr);
                    log_trace(logger, "Socket INTERRUPT guardado: %d", socket_cliente);
                    free(cpu_id_str);
                }
                break;
            case PAQUETE:
                log_trace(logger, "llego un paquete");
                break;
            case SYSCALL_IO:
                llamar_a_io(socket_cliente);
                break;
            case SYSCALL_DUMP_MEMORY:
                dump_memory(socket_cliente);
                log_trace(logger, "se corrio un dump memory");
            case SYSCALL_INIT:
                log_trace(logger, "me llego syscall INIT_PROC");
                iniciar_proceso(socket_cliente);
                break;
            case ERROR:
                break;
            default:
                log_info(logger, "error en el recv");
                break;
        }
    }
    close(socket_cliente);
    free(nombre_cliente);
    return NULL;
}


bool handshake_memoria(int socket){
    enviar_handshake(socket);
    log_info(logger, "Envié un handshake a memoria");

    int respuesta;
    if(0 >= recv(socket, &respuesta, sizeof(int), MSG_WAITALL)){
        log_error(logger, "Fallo al recibir OK de memoria");
        return false;
    }
    if(respuesta == OK){
        log_info(logger, "Recibi el OK de memoria");
        return true;
    }else {
        log_error(logger, "Fallo en el handshake de memoria, recibí %d", respuesta);
        return false;
    }

    return false;
}


void handshake_io(int socket_dispositivo){
    log_info(logger, "recibi un handshake de io");
    op_code respuesta = OK;

    send(socket_dispositivo, &respuesta, sizeof(int),0);
    log_info(logger, "Envie OK a IO");

    char* nombre_dispositivo;
    op_code op = recibir_operacion(socket_dispositivo);


    if(op != MENSAJE){
        log_error(logger, "No llegó el mensaje de IO");
        return;
    }

    nombre_dispositivo = recibir_mensaje(socket_dispositivo);
    log_info(logger, "Conexion de IO: %s", nombre_dispositivo);

    t_dispositivo_io* nuevo_io = malloc(sizeof(t_dispositivo_io));
    nuevo_io->socket_io = socket_dispositivo;
    nuevo_io->cola_bloqueados = queue_create();
    nuevo_io->ocupado = false;

    dictionary_put(dispositivos_io, strdup(nombre_dispositivo), nuevo_io);

    log_info(logger, "Se registró el dispositivo IO [%s] en el diccionario", nombre_dispositivo);

    free(nombre_dispositivo);
    return;
}

void* manejar_servidor_io(int socket_io){
    int estado_anterior;
    int socket_cliente = esperar_cliente(socket_io, logger);

    while(1){
        int codigo_operacion = recibir_operacion(socket_cliente);

        if(codigo_operacion == -1){
            log_info(logger, "Se cerró la conexion de IO");
            break;
        }

        log_info(logger, "IO Código de operación recibido: %d", codigo_operacion);

        switch (codigo_operacion){
            case CERRADO:
                log_info(logger, "termino la conexion con exito");
                break;

            case HANDSHAKE:
                handshake_io(socket_cliente);
                break;

            case PAQUETE:
                log_info(logger, "llego un paquete");
                break;

            case FINALIZA_IO:
                t_list* recibido = recibir_paquete(socket_cliente);
                int* pid = list_get(recibido, 0);
                log_trace(logger, "Recibi finalizacion de io - pid %d", *pid);
                t_pcb* pcb = obtener_pcb(*pid);

                if (pcb == NULL){
                    log_error(logger, "FINALIZA_IO: No se encontró el PCB del PID %d", *pid);
                    list_destroy_and_destroy_elements(recibido, free);
                    break;
                }

                if (pcb->estado_actual == SUSP_BLOCKED){

                    pthread_mutex_lock(&mutex_susp_blocked);
                    sacar_pcb_de_cola(cola_susp_blocked, pcb->pid);
                    pthread_mutex_unlock(&mutex_susp_blocked);

                    estado_anterior = pcb->estado_actual;
                    cambiar_estado(pcb, SUSP_READY);
                    log_info(logger, "(%d) Pasa del estado %s al estado %s",pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));


                    pthread_mutex_lock(&mutex_susp_ready);
                    queue_push(cola_susp_ready, pcb);
                    pthread_mutex_unlock(&mutex_susp_ready);

                } else if (pcb->estado_actual == BLOCKED){
                    estado_anterior = pcb->estado_actual;
                    cambiar_estado(pcb, READY);
                    log_info(logger, "(%d) Pasa del estado %s al estado %s",pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));

                    queue_push(cola_ready, pcb);
                    log_info(logger, "%d Finalizo IO y pasa a READY", pcb->pid);
                    sem_post(&sem_procesos_ready);

                }
                list_destroy_and_destroy_elements(recibido, free);
                break;

            case ERROR:
                break;

            default:
                log_info(logger, "error en el recv");
                break;
        }
    }
    close(socket_cliente);
    return NULL;
}

int conectar_memoria(char* ip, char* puerto){
    struct addrinfo hints;
    struct addrinfo *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    //guardo en server_info los datos para conectar el socket
    getaddrinfo(ip, puerto, &hints, &server_info);
    int fd_socket = socket(server_info->ai_family,
                                server_info->ai_socktype,
                                server_info->ai_protocol);

    if (connect(fd_socket, server_info->ai_addr, server_info->ai_addrlen) == -1) {
        log_error(logger, "Error al conectar con memoria");
        return -1;
    }
    freeaddrinfo(server_info);

    return fd_socket;
}

int operacion_con_memoria(){
    int socket = conectar_memoria(ip_memoria, puerto_memoria);
    log_trace(logger, "Se abrio una conexion con memoria");
    if(handshake_memoria(socket)){
        return socket;
    }
    else{
        return -1;
    }
}

void cerrar_conexion_memoria(int socket){
    close(socket);
    log_trace(logger, "La conexion con memoria se cerro con exito");
}


//por cada cpu(o conexion) queremos un accept nuevo, por lo tanto hay que tirar un hilo por cada accept