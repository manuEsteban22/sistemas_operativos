#include <conexion_memoria_kernel.h>
#include <utils/utils.h>
#include <pthread.h> 
extern t_log* logger;

void* atender_kernel(void* socket_ptr) 
{
    int socket_cliente = *((int*)socket_ptr);
    free(socket_ptr);

    while(1) 
    {
        int codigo_operacion = recibir_operacion(socket_cliente);

        if (codigo_operacion <= 0) 
        {
            log_info(logger, "Se cerró la conexión o error");
            break;
        }

        log_info(logger, "Código de operación recibido: %d", codigo_operacion);

        switch (codigo_operacion) 
        {
            case CERRADO:
                log_info(logger, "Se terminó la conexión con éxito");
                break;
            case HANDSHAKE:
                log_info(logger, "Recibí un handshake");
                op_code respuesta = OK;
                send(socket_cliente, &respuesta, sizeof(int), 0);
                log_info(logger, "Envié OK");
                break;
            case PAQUETE:
                log_info(logger, "Llegó un paquete");
                recibir_paquete(socket_cliente);
                break;
            case OC_READ:
                log_info(logger, "recibi un read");
                break;
            case OC_WRITE:
                log_info(logger, "recibi un write");
                break;
            case ERROR:
                log_info(logger, "Recibí un error");
                break;
            default:
                log_info(logger, "Error en el recv: operación desconocida");
                break;
        }
    }  
    close(socket_cliente); 
    return NULL;
}

void* manejar_servidor_kernel(int socket_servidor) 
{
    while (1) 
    {
        int socket_cliente = esperar_cliente(socket_servidor, logger);

        int* socket_cliente_ptr = malloc(sizeof(int));
        *socket_cliente_ptr = socket_cliente;

        pthread_t hilo_cliente;
        pthread_create(&hilo_cliente, NULL, atender_kernel, socket_cliente_ptr);
        pthread_detach(hilo_cliente);
    }
    return NULL;
}

void* inicializar_proceso(int tam_proceso, int pid+)
{

int pags_necesarias = tam_proceso / campos_config.tam_pagina;


}

void* suspender_proceso()
{

}

void* des_suspender_proceso()
{

}

void* finalizar_proceso()
{

}
