#include <utils/utils.h>
#include <instrucciones.h>
#include <memoria.h>
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

int mandar_instrucciones(int socket_cliente) 
{
    int pc = 2;
    int pid = 0;
    cargar_instrucciones(pid);

    int cant = cantidad_instrucciones();
    for (; pc < cant; pc++) 
    {
        char* instruccion = obtener_instruccion(pc);
        t_paquete* paquete = crear_paquete();
        int tamanio = strlen(instruccion) + 1;
        agregar_a_paquete(paquete, instruccion, tamanio);
        enviar_paquete(paquete, socket_cliente);
        borrar_paquete(paquete);
    }
    

    printf("Espacio libre: %d\n", espacio_libre());

    liberar_memoria();
    return 0;
}

