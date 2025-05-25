#include <conexion_memoria.h>
#include <instrucciones.h>
#include <utils/utils.h>
#include <pthread.h>  // Agregado para hilos

extern t_log* logger;  // Para usar el logger global

void* atender_clientes(void* socket_ptr) {
    int socket_cliente = *((int*)socket_ptr);
    free(socket_ptr);

    while(1) {
        int codigo_operacion = recibir_operacion(socket_cliente);

        if (codigo_operacion <= 0) {
            log_info(logger, "Se cerró la conexión o error");
            break;
        }

        log_info(logger, "Código de operación recibido: %d", codigo_operacion);

        switch (codigo_operacion) {
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

void manejar_servidor(int socket_servidor) {
    while(1) {
        int* socket_cliente = malloc(sizeof(int));
        *socket_cliente = esperar_cliente(socket_servidor);
        if (*socket_cliente < 0) {
            log_info(logger, "Error aceptando cliente");
            free(socket_cliente);
            continue;
        }

        pthread_t hilo;
        pthread_create(&hilo, NULL, atender_clientes, socket_cliente);
        pthread_detach(hilo);
    }
}