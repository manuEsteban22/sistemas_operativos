#include <utils/utils.h>
#include <kernel.h>
#include <conexion_kernel.h>

void* manejar_servidor(void* arg){
    t_args_hilo* argumentos = (t_args_hilo*) arg;
    int socket = argumentos->socket;
    char* nombre_cliente = strdup(argumentos->nombre);
    free(arg);

    int socket_cliente = esperar_cliente(socket);

    //falta agregar recv
    int codigo_operacion = recibir_operacion(socket_cliente);
    switch (codigo_operacion){
        case CERRADO:
            log_info(logger, "cerro la conexion");
            //enviar otro
            break;
        case HANDSHAKE:
            log_info(logger, "llego un handshake");
            //recibir paquete
            break;
        case PAQUETE:
            log_info(logger, "llego un paquete");
            break;
        default:
            log_info(logger, "error en el recv");
            break;
    }

    //devolver el handshake
}
//por cada cpu(o conexion) queremos un accept nuevo, por lo tanto hay que tirar un hilo por cada accept