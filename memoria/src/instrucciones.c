
#include <instrucciones.h>
#define MAX_LINEA 256

static t_list* lista_instrucciones = NULL;

void cargar_instrucciones(int pid) 
{
    char* Archivo = string_from_format("%sinstrucciones_pid%i.txt", campos_config.path_instrucciones, pid);
    FILE* archivo = fopen( Archivo, "r");
    if (!archivo) 
    {
        perror("No se pudo abrir el archivo de instrucciones");
        exit(EXIT_FAILURE);
    }

    lista_instrucciones = list_create();

    char linea[MAX_LINEA];
    while (fgets(linea, sizeof(linea), archivo)) 
    {
        linea[strcspn(linea, "\n")] = 0; // eliminar \n
        list_add(lista_instrucciones, strdup(linea)); // guardar copia
    }

    fclose(archivo);
}

char* obtener_instruccion(int pc) 
{
    if (pc >= list_size(lista_instrucciones)) return NULL;
    return list_get(lista_instrucciones, pc);
}

int cantidad_instrucciones() 
{
    return list_size(lista_instrucciones);
}

int espacio_libre()
{
    const int tam_mem = campos_config.tam_memoria;
    return tam_mem;
}

void liberar_memoria() 
{
    list_destroy_and_destroy_elements(lista_instrucciones, free);
}


int mandar_instruccion(int socket_cliente) 
{

    t_list* lista_paquete = list_create();
    lista_paquete = recibir_paquete(socket_cliente);
    int pid = *((int*) list_get(lista_paquete, 0));
    int pc = *((int*) list_get(lista_paquete, 1));

    cargar_instrucciones(pid);

    int cant = cantidad_instrucciones();

    
    t_paquete* paquete = crear_paquete();

    /*if(pc > cant)
    {
        cambiar_opcode_paquete(paquete, CERRADO);
        enviar_paquete(paquete, socket_cliente, logger);
    }*/
    
    char* instruccion = obtener_instruccion(pc);

    if (instruccion == NULL) {
    log_error(logger, "No se encontró la instrucción en la posición PC=%d", pc);
    liberar_memoria();
    list_destroy_and_destroy_elements(lista_paquete, free);
    return -1;
    }

    int tamanio = strlen(instruccion) + 1;
    log_trace(logger, "PID: %d - Obtener instrucción: %d - Instrucción: %s", pid, pc, instruccion);
    agregar_a_paquete(paquete, instruccion, tamanio);
    enviar_paquete(paquete, socket_cliente, logger);
    borrar_paquete(paquete);

    printf("Espacio libre: %d\n", espacio_libre());

    liberar_memoria();
    list_destroy_and_destroy_elements(lista_paquete, free); 
    return 0;
}

int mandar_frame(int socket_cliente){//recibo nro_pagina y pid y le mando el frame
    t_list* lista_paquete = list_create();
    lista_paquete = recibir_paquete(socket_cliente);

    int pid = *((int*) list_get(lista_paquete, 0));
    int nro_pagina = *((int*) list_get(lista_paquete, 1));

    //Aca tuca y manu tienen que hacer algo para ver a que frame corresponde esa pagina CHUPAME LAS BOLAS PABLO
    int marco = 2;//esto borrenlo claramente
    t_paquete* paquete = crear_paquete();
    cambiar_opcode_paquete(paquete, OC_FRAME);
    agregar_a_paquete(paquete,&marco,sizeof(int));
    enviar_paquete(paquete, socket_cliente, logger);
    borrar_paquete(paquete);
}

// int mandar_instrucciones(int socket_cliente) 
// {
//     while(1)
//     {
//         mandar_instruccion(socket_cliente);    
//     }
// }
