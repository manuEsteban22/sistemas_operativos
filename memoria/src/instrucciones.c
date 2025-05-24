#include <utils/utils.h>
#include <instrucciones.h>
#include <memoria.h>
#define MAX_LINEA 256

static t_list* lista_instrucciones = NULL;

void cargar_instrucciones(int pid) {
    char* rutaArchivo = string_from_format("/home/utnso/Desktop/SISTEMAS OPERATIVOS/tp-2025-1c-power-rangers/memoria/src/instrucciones_pid%i.txt", pid);
    FILE* archivo = fopen(rutaArchivo, "r");
    if (!archivo) {
        perror("No se pudo abrir el archivo de instrucciones");
        exit(EXIT_FAILURE);
    }

    lista_instrucciones = list_create();

    char linea[MAX_LINEA];
    while (fgets(linea, sizeof(linea), archivo)) {
        linea[strcspn(linea, "\n")] = 0; // eliminar \n
        list_add(lista_instrucciones, strdup(linea)); // guardar copia
    }

    fclose(archivo);
}

char* obtener_instruccion(int pc) {
    if (pc >= list_size(lista_instrucciones)) return NULL;
    return list_get(lista_instrucciones, pc);
}

int cantidad_instrucciones() {
    return list_size(lista_instrucciones);
}

int espacio_libre() {
    return 1024;
}

void liberar_memoria() {
    list_destroy_and_destroy_elements(lista_instrucciones, free);
}

int mandar_instrucciones() {
    int pc = 2;
    int pid = 0;
    cargar_instrucciones(pid);

    int cant = cantidad_instrucciones();
    for (int a = pc; a < cant; a++) {
        char* instruccion = obtener_instruccion(a);
        printf("Instrucción %d: %s\n", a, instruccion);
    }

    printf("Espacio libre: %d\n", espacio_libre());

    liberar_memoria();
    return 0;
}