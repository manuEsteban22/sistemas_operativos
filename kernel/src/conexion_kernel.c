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

void* manejar_servidor(void* arg){
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
                log_info(logger, "se termino la conexion con exito");
                break;
            case HANDSHAKE:
                log_info(logger, "recibi un handshake");
                op_code respuesta = OK;
                send(socket_cliente, &respuesta, sizeof(int),0);
                log_info(logger, "Envie OK a %s", nombre_cliente);
                // chequear que los otros modulos 
                break;
            case PAQUETE:
                log_info(logger, "llego un paquete");
                recibir_paquete(socket_cliente);
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
//por cada cpu(o conexion) queremos un accept nuevo, por lo tanto hay que tirar un hilo por cada accept