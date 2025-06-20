#include <conexion_memoria_cpu.h>
#include <pthread.h>  // Agregado para hilos

void* atender_cpu(void* socket_ptr) 
{
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
                mandar_instruccion(socket_cliente);         
                break;
            case OC_READ:
                log_info(logger, "recibi un read");
                break;
            case OC_WRITE:
                log_info(logger, "recibi un write");
                break;
            
//-------------- ESTO PERTENECE A KERNEL -------------------------------
            case OC_INIT:
                log_info(logger, "llego una peticion de crear un nuevo proceso");
                t_list* recibido = recibir_paquete(socket_cliente);
                int* pid = list_get(recibido, 0);
                int* tamanio = list_get(recibido, 1);
                log_info(logger, "Proceso PID=%d - Tamanio=%d", *pid, *tamanio);
                if(*tamanio <= campos_config.tam_memoria){//esto se va a tener que cambiar por una funcion de memoria disponible creo
                send(socket_cliente, OK, sizeof(int), 0);
                }
                list_destroy_and_destroy_elements(recibido, free);
                break;
// --------------ESTO PERTENECE A KERNEL------------------------------
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

void* manejar_servidor(void* socket_ptr) 
{
    int socket_servidor = *((int*)socket_ptr);
    free(socket_ptr);
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

void* lanzar_servidor(int socket_servidor)
{
    pthread_t hilo_conexion;

    int* socket_ptr = malloc(sizeof(int));
    *socket_ptr = socket_servidor;

    pthread_create(&hilo_conexion, NULL, manejar_servidor, socket_ptr);
    pthread_detach(hilo_conexion);

    return NULL;
}

