#include <conexion_memoria_cpu.h>
#include <pthread.h>  // Agregado para hilos

void* atender_cpu(void* socket_ptr) {
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
                t_list* lista_paquete = list_create();
                lista_paquete = recibir_paquete(socket_cliente);
                int pid = *((int*) list_get(lista_paquete, 0));
                int pc = *((int*) list_get(lista_paquete, 1));
                log_info(logger, "pid:%d, pc:%d", pid, pc);
                mandar_instrucciones(socket_cliente, pid, pc);      
                list_destroy_and_destroy_elements(lista_paquete, free);    
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

void* manejar_servidor(int socket_servidor) {
    while (1) {
        int socket_cliente = esperar_cliente(socket_servidor, logger);

        int* socket_cliente_ptr = malloc(sizeof(int));
        *socket_cliente_ptr = socket_cliente;

        pthread_t hilo_cliente;
        pthread_create(&hilo_cliente, NULL, atender_cpu, socket_cliente_ptr);
        pthread_detach(hilo_cliente);
    }
    return NULL;
}


