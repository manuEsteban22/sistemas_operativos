#include <utils/utils.h>
#include <kernel.h>

void manejar_servidor(void* arg){
    t_args_hilo* argumentos = (t_args_hilo*) arg

    int socket = t_args_hilo->socket;
    char* nombre_cliente = t_args_hilo->nombre;
    free(arg);

    esperar_cliente()
}