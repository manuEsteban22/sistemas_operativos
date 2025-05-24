#include<conexion_memoria.h>


void* manejar_servidor(int socket){

    int socket_cliente = esperar_cliente(socket);

    while(1){
        int codigo_operacion = recibir_operacion(socket_cliente);

        if(codigo_operacion == -1){
            log_info(logger, "Se cerró la conexion");
            break;
        }

        log_info(logger, "Código de operación recibido: %d", codigo_operacion);

        switch (codigo_operacion){
            case CERRADO:
                log_info(logger, "se termino la conexion con exito");
                break;
            case HANDSHAKE:
                log_info(logger, "recibi un handshake");
                op_code respuesta = OK;
                send(socket_cliente, &respuesta, sizeof(int),0);
                log_info(logger, "Envie OK");
                break;
            case PAQUETE:
                log_info(logger, "llego un paquete");
                recibir_paquete(socket_cliente);
                break;
            case READ:
                log_info(logger, "recibi un read");
                break;
            case WRITE:
                log_info(logger, "recibi un write");
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