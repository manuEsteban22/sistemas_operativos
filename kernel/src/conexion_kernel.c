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
                //close(socket_cliente);
                //la conexion sigue abierta pero sin mensajes(?
                log_info(logger, "termino la conexion con exito");
                break;
            case HANDSHAKE:
                int cpu_id;
                log_info(logger, "recibi un handshake de cpu");
                op_code respuesta = OK;
                send(socket_cliente, &respuesta, sizeof(int),0);
                log_info(logger, "Envie OK a %s", nombre_cliente);
                recv(socket_cliente, &cpu_id, sizeof(int), MSG_WAITALL);
                log_info(logger, "Conexion de CPU ID: %d", cpu_id);
                break;
            case PAQUETE:
                log_info(logger, "llego un paquete");
                //deserializar ()
                //recibir paquete -> deserializar
                break;
            case SYSCALL_IO:
                llamar_a_io(socket_cliente);
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


void handshake_memoria(int socket){
    enviar_handshake(socket);
    log_info(logger, "envié el handshake a memoria");

    int respuesta;
    if(0 >= recv(socket, &respuesta, sizeof(int), MSG_WAITALL)){
        log_error(logger, "Fallo al recibir OK de memoria");
        return;
    }
    if(respuesta == OK){
        log_info(logger, "Recibi el OK de memoria");
        return;
    }else {
        log_error(logger, "Fallo en el handshake de memoria, recibí %d", respuesta);
        return;
    }

    return;
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
                //close(socket_cliente);
                //la conexion sigue abierta pero sin mensajes(?
                log_info(logger, "termino la conexion con exito");
                break;
            case HANDSHAKE:
                handshake_io(socket_cliente);
                break;
            case PAQUETE:
                log_info(logger, "llego un paquete");
                //deserializar ()
                //recibir paquete -> deserializar
                break;
            case FINALIZA_IO:
                t_list* recibido = recibir_paquete(socket_cliente);
                int* pid = list_get(recibido, 0);
                log_trace(logger, "Recibi finalizacion de io - pid %d", *pid);
                t_pcb* pcb = obtener_pcb(*pid);

                if (pcb == NULL) {
                        log_error(logger, "FINALIZA_IO: No se encontró el PCB del PID %d", *pid);
                        list_destroy_and_destroy_elements(recibido, free);
                        break;
                    }

                if(pcb->estado_actual==SUSP_BLOCKED){
                    cambiar_estado(pcb, SUSP_READY);
                    queue_push(cola_susp_ready, pcb);
                }else if(pcb->estado_actual == BLOCKED){
                    cambiar_estado(pcb, READY);
                    queue_push(cola_ready, pcb);
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
    //envio y recibo un handhsake a memoria
    handshake_memoria(fd_socket);
    

    /*t_paquete  paquete_pid_pc = crear_paquete();
    agregar_a_paquete(paquete_pid_pc, pid, sizeof(int));
    agregar_a_paquete(paquete_pid_pc, pc, sizeof(int));
    enviar_paquete(paquete_pid_pc, fd_socket);
    borrar_paquete(paquete_pid_pc);*/

    return fd_socket;
}




//por cada cpu(o conexion) queremos un accept nuevo, por lo tanto hay que tirar un hilo por cada accept