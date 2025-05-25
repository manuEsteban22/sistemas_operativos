#include <utils/utils.h>
#include <kernel.h>
#include <conexion_kernel.h>

void* manejar_servidor_cpu(void* arg){
    t_args_hilo* argumentos = (t_args_hilo*) arg;
    int socket = argumentos->socket;
    char* nombre_cliente = strdup(argumentos->nombre);
    free(arg);

    int socket_cliente = esperar_cliente(socket);
    
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

void* manejar_servidor_io(int socket_io){

    int socket_cliente = esperar_cliente(socket_io);

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
                int io_id;
                log_info(logger, "recibi un handshake de io");
                op_code respuesta = OK;
                send(socket_cliente, &respuesta, sizeof(int),0);
                log_info(logger, "Envie OK a IO");
                char* nombre_dispositivo;
                nombre_dispositivo = recibir_mensaje(socket_cliente);
                log_info(logger, "Conexion de IO: %s", nombre_dispositivo);
                break;
            case PAQUETE:
                log_info(logger, "llego un paquete");
                //deserializar ()
                //recibir paquete -> deserializar
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
    
    int pid = 0
    int pc = 2

    t_paquete  paquete_pid_pc = crear_paquete();
    agregar_a_paquete(paquete_pid_pc, pid, sizeof(int));
    agregar_a_paquete(paquete_pid_pc, pc, sizeof(int));
    enviar_paquete(paquete_pid_pc, fd_socket);
    borrar_paquete(paquete_pid_pc);

    return fd_socket;
}









//por cada cpu(o conexion) queremos un accept nuevo, por lo tanto hay que tirar un hilo por cada accept