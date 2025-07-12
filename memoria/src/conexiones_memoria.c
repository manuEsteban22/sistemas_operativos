#include <conexiones_memoria.h>
#include <pthread.h>

// Función para manejar conexiones PERSISTENTES de CPU
void manejar_conexion_cpu(int socket_cliente) {
    while(1) {
        int codigo_operacion = recibir_operacion(socket_cliente);
        if (codigo_operacion <= 0) {
            log_warning(logger, "CPU desconectada");
            break;
        }

        switch(codigo_operacion) {
            case PAQUETE:
                log_info(logger, "Llegó un paquete");
                mandar_instruccion(socket_cliente);         
                break;
            case OC_READ:
                log_info(logger, "recibi un read");
                ejecutar_read(socket_cliente);
                break;
            case OC_WRITE:
                log_info(logger, "recibi un write");
                ejecutar_write(socket_cliente);
                break;
            case OC_FRAME:
                mandar_frame(socket_cliente);
                log_info(logger, "mande el frame");
                break;
            default:
                log_error(logger, "Operación CPU desconocida: %d", codigo_operacion);
        }
    }
    //close(socket_cliente);
}

// Función para manejar conexiones EFÍMERAS de Kernel
void manejar_conexion_kernel(int socket_cliente) {
    int codigo_operacion = recibir_operacion(socket_cliente);
    
    switch(codigo_operacion) {
        case OC_INIT:
            log_trace(logger, "Llego una peticion de crear un nuevo proceso");
            t_list* recibido = recibir_paquete(socket_cliente);
            int* pid = list_get(recibido, 0);
            int* tamanio = list_get(recibido, 1);
            log_trace(logger, "Proceso PID=%d - Tamanio=%d", *pid, *tamanio);
            if(*tamanio <= campos_config.tam_memoria){//esto se va a tener que cambiar por una funcion de memoria disponible creo
                log_trace(logger, "Hay suficiente memoria, se manda el OK");
                t_paquete* paquete = crear_paquete();
                cambiar_opcode_paquete(paquete, OK);
                enviar_paquete(paquete, socket_cliente, logger);
                borrar_paquete(paquete);
            }
            list_destroy_and_destroy_elements(recibido, free);
            break;
        case SOLICITUD_DUMP_MEMORY:
            log_trace(logger, "Se recibio solicitud de hacer un memory dump");
            dumpear_memoria();
            break;
        default:
            log_error(logger, "Operación Kernel desconocida: %d", codigo_operacion);
            break;
    }
}

void* manejar_conexiones_memoria(void* socket_ptr) {
    int socket_cliente = *((int*)socket_ptr);
    //free(socket_ptr);

    int codigo_operacion = recibir_operacion(socket_cliente);
    
    if (codigo_operacion == HANDSHAKE_CPU_MEMORIA) {
        log_trace(logger, "Recibi el handshake de una CPU");
        int respuesta = OK;//temporal hasta que agregue lo de mandar tam_pag
        send(socket_cliente, &respuesta, sizeof(int), 0);
        sleep(5);
        
        int* socket_cpu = malloc(sizeof(int));
        *socket_cpu = socket_cliente;
        
        pthread_t hilo_cpu;
        pthread_create(&hilo_cpu, NULL, (void*)manejar_conexion_cpu, socket_cpu);
        pthread_join(hilo_cpu, NULL);
        return NULL;
    }
    else if (codigo_operacion == HANDSHAKE) {
        log_info(logger, "Conexión efimera de Kernel");
        op_code respuesta = OK;
        send(socket_cliente, &respuesta, sizeof(int), 0);
        
        manejar_conexion_kernel(socket_cliente);
        close(socket_cliente);
        return NULL;
    }
    else {
        log_error(logger, "Handshake invalido: %d", codigo_operacion);
        close(socket_cliente);
        return NULL;
    }
}

void* manejar_servidor(void* socket_ptr) 
{
    //por cada accept esta funcion tira un hilo
    int socket_servidor = *((int*)socket_ptr);
    free(socket_ptr);
    while (1) {
        int socket_cliente = esperar_cliente(socket_servidor, logger);

        int* socket_cliente_ptr = malloc(sizeof(int));
        *socket_cliente_ptr = socket_cliente;

        pthread_t hilo_cliente;
        pthread_create(&hilo_cliente, NULL, manejar_conexiones_memoria, socket_cliente_ptr);
        pthread_detach(hilo_cliente);
    }
    return NULL;
}

void* lanzar_servidor(int socket_servidor)
{
    //este es el hilo main que lanza todas las conexiones
    pthread_t hilo_conexion;

    int* socket_ptr = malloc(sizeof(int));
    *socket_ptr = socket_servidor;

    pthread_create(&hilo_conexion, NULL, manejar_servidor, socket_ptr);
    pthread_detach(hilo_conexion);

    return NULL;
}

