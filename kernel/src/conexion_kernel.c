#include <utils/utils.h>
#include <kernel.h>
#include <conexion_kernel.h>

void* manejar_servidor_cpu(void* arg){
    
    t_args_hilo* argumentos = (t_args_hilo*) arg;
    int socket_cliente = argumentos->socket;
    char* nombre_cliente = strdup(argumentos->nombre);
    free(arg);

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
                log_trace(logger, "Me llego un Dump Memory");  
                dump_memory(socket_cliente);
                break;
            case SYSCALL_INIT:
                log_info(logger, "Me llego syscall INIT_PROC");
                iniciar_proceso(socket_cliente);
                break;
            case SYSCALL_EXIT:
                log_info(logger, "Recibi un EXIT");
                ejecutar_exit(socket_cliente);
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

    op_code op = recibir_operacion(socket_dispositivo);


    if(op != MENSAJE){
        log_error(logger, "No llegó el mensaje de IO");
        return;
    }

    char* nombre_dispositivo = recibir_mensaje(socket_dispositivo);
    log_info(logger, "Conexion de IO: %s", nombre_dispositivo);

    t_dispositivo_io* io = dictionary_get(dispositivos_io, nombre_dispositivo);
    if(io == NULL){
        io = malloc(sizeof(t_dispositivo_io));
        io->sockets_io = list_create();
        io->cola_bloqueados = queue_create();
        //io->ocupado = false;
        //io->pid_ocupado = -1;
        dictionary_put(dispositivos_io, nombre_dispositivo, io);
    }
    t_instancia_io* nueva_instancia = malloc(sizeof(t_instancia_io));
    nueva_instancia->socket = socket_dispositivo;
    nueva_instancia->ocupado = false;
    nueva_instancia->pid_ocupado = -1;
    list_add(io->sockets_io, nueva_instancia);

    log_info(logger, "Se registró el socket (%d) para dispositivo IO [%s]", nueva_instancia->socket, nombre_dispositivo);
    return;
}

char* buscar_io_por_socket(int socket_io){
    t_list* keys = dictionary_keys(dispositivos_io);
    char* nombre = NULL;

    for(int i = 0; i < list_size(keys); i++){
        char* key = list_get(keys, i);
        t_dispositivo_io* dispositivo = dictionary_get(dispositivos_io, key);
        for(int j = 0; j < list_size(dispositivo->sockets_io); j++){
            t_instancia_io* instancia = list_get(dispositivo->sockets_io, j);
            if(instancia->socket == socket_io){
                nombre = strdup(key);
                break;
            }
        }
        if(nombre != NULL)break;
    }
    list_destroy(keys);
    return nombre;
}

t_instancia_io* obtener_instancia_disponible(t_dispositivo_io* dispositivo){
    for (int i = 0; i < list_size(dispositivo->sockets_io); i++){
        t_instancia_io* instancia = list_get(dispositivo->sockets_io, i);
        if (!instancia->ocupado){
            return instancia;
        }
    }
    return NULL;
}

void borrar_socket_io(t_dispositivo_io* dispositivo, int socket_a_borrar){
    for(int i = 0; i < list_size(dispositivo->sockets_io); i++){
        t_instancia_io* instancia = list_get(dispositivo->sockets_io, i);
        if(instancia->socket == socket_a_borrar){
            list_remove_and_destroy_element(dispositivo->sockets_io, i, free);
            return;
        }
    }
}


void matar_io (int socket_cliente){
    char* nombre = buscar_io_por_socket(socket_cliente);
    if(nombre != NULL){
        log_info(logger, "Se desconecto un dispositivo [%s] de socket (%d)", nombre, socket_cliente);
        t_dispositivo_io* dispositivo = dictionary_get(dispositivos_io, nombre);
        borrar_socket_io(dispositivo, socket_cliente);


        if(list_is_empty(dispositivo->sockets_io)){
            log_info(logger, "No quedan instancias para el dispositivo [%s]", nombre);

            while(!queue_is_empty(dispositivo->cola_bloqueados)){
                t_pcb_io* pcb_io = queue_pop(dispositivo->cola_bloqueados);
                t_pcb* pcb = obtener_pcb(pcb_io->pid);
                if(pcb != NULL){
                    int estado_anterior = pcb->estado_actual;
                    cambiar_estado(pcb, EXIT);
                    log_info(logger, "(%d) Pasa del estado %s al estado %s",pcb->pid, parsear_estado(estado_anterior), parsear_estado(pcb->estado_actual));
                    borrar_pcb(pcb);
                }
                free(pcb_io);
            }
            list_destroy_and_destroy_elements(dispositivo->sockets_io, free);
            queue_destroy(dispositivo->cola_bloqueados);
            dictionary_remove(dispositivos_io, nombre);
            free(dispositivo);
        }
        
        free(nombre);
    } else{
        log_error(logger, "No se encontro un IO con socket %d", socket_cliente);
    }
}

void* manejar_servidor_io(void* arg){
    int socket_cliente = *((int*) arg);
    free(arg);
    while(1){
        int codigo_operacion = recibir_operacion(socket_cliente);

        if(codigo_operacion <= 0){
            log_info(logger, "Se cerró la conexion de IO, Socket %d", socket_cliente);
            matar_io(socket_cliente);
            break;
        }

        log_info(logger, "IO Código de operación recibido: %d", codigo_operacion);

        switch (codigo_operacion){
            case HANDSHAKE:
                handshake_io(socket_cliente);
                break;

            case PAQUETE:
                log_info(logger, "llego un paquete");
                break;

            case FINALIZA_IO:
                manejar_finaliza_io(socket_cliente);
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

void* hilo_main_cpu(void* args){

    pthread_t thread_dispatch;
    pthread_t thread_interrupt;

    while(1){
        
        int socket_cliente_dispatch = esperar_cliente(socket_dispatch, logger);
        sem_post(&cpus_disponibles);
        t_args_hilo* args_dispatch = malloc(sizeof(t_args_hilo));
        args_dispatch->socket = socket_cliente_dispatch;
        args_dispatch->nombre = "DISPATCH";

        pthread_create(&thread_dispatch, NULL, manejar_servidor_cpu, (void*)args_dispatch);
        pthread_detach(thread_dispatch);
        log_trace(logger, "Nueva conexión DISPATCH aceptada");

        int socket_cliente_interrupt = esperar_cliente(socket_interrupt, logger);
        t_args_hilo* args_interrupt = malloc(sizeof(t_args_hilo));
        args_interrupt->socket = socket_cliente_interrupt;
        args_interrupt->nombre = "INTERRUPT";

        pthread_create(&thread_interrupt, NULL, manejar_servidor_cpu, (void*)args_interrupt);
        pthread_detach(thread_interrupt);
        log_trace(logger, "Nueva conexión INTERRUPT aceptada");
    }
}
void* hilo_main_io(void* args){
    pthread_t thread_io;

    while(1){
        int socket_cliente = esperar_cliente(socket_io, logger);
        int* socket_cliente_ptr = malloc(sizeof(int));
        *socket_cliente_ptr = socket_cliente;
        pthread_create(&thread_io, NULL, manejar_servidor_io, (void*)socket_cliente_ptr);
        pthread_detach(thread_io);
    }
}