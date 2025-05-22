#include <utils/utils.h>
#include <instrucciones.h>
#include <memoria.h>
//t_list* instrucciones = list_create();

t_list* cargar_instrucciones(const char* rutaArchivo) {
    FILE* archivo = fopen(rutaArchivo, "r");
    if (!archivo) {
        perror("Error al abrir archivo");
        return NULL;
    }

    t_list* lista = list_create();
    char* linea = NULL;
    size_t len = 0;
    ssize_t read;
    int pc = 0;

    while ((read = getline(&linea, &len, archivo)) != -1) {
        // Quitar salto de línea
        string_trim(&linea);

        t_instruccion* instr = malloc(sizeof(t_instruccion));
        instr->pc = pc;
        instr->instruccion = strdup(linea);
        list_add(lista, instr);

        pc++;
    }

    free(linea);
    fclose(archivo);
    return lista;
}

t_instruccion* get_instruccion(t_list* lista, int pc) {
    if (pc < 0 || pc >= list_size(lista)) return NULL;
    return list_get(lista, pc);
}

void memoria_liberar(t_list* lista) {
    for (int i = 0; i < list_size(lista); i++) {
        t_instruccion* instr = list_get(lista, i);
        free(instr->instruccion);
        free(instr);
    }
    list_destroy(lista);
}

int devolver_instrucciones() {
    
    int pid = 0;

    char* archivo_instrucciones = string_from_format("instrucciones_pid%i.txt", pid);

    t_list* memoria = cargar_instrucciones(archivo_instrucciones);
    //if (!memoria) return 1;

    int pc;
    printf("Ingrese PC para obtener instruccion: ");
    scanf("%d", &pc);

    t_instruccion* instr = get_instruccion(memoria, pc);
    if (instr) {
        printf("PC %d: %s\n", instr->pc, instr->instruccion);
    } else {
        printf("PC fuera de rango\n");
    }

    memoria_liberar(memoria);
    return 0;
}