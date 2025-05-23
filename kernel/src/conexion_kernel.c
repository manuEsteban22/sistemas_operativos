#include <utils/utils.h>
#include <kernel.h>
#include <conexion_kernel.h>

void* manejar_servidor(void* arg){
    t_args_hilo* argumentos = (t_args_hilo*) arg;
    int socket = argumentos->socket;
    char* nombre_cliente = strdup(argumentos->nombre);
    free(arg);

    int socket_cliente = esperar_cliente(socket);

    int codigo_operacion = recibir_operacion(socket_cliente);
    switch (codigo_operacion){
        case CERRADO:
            close(socket_cliente);
            log_info(logger, "cerro la conexion");
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
            //deserealizar ()
            //recibir paquete -> deserializar
            break;
        
        default:
            log_info(logger, "error en el recv");
            break;
    }

}
//por cada cpu(o conexion) queremos un accept nuevo, por lo tanto hay que tirar un hilo por cada accept