#include <syscalls.h>


void llamar_a_io(socket_cpu){
    int pid;
    int size_dispositivo;
    char* dispositivo;
    int tiempo;
    recv(socket_cpu, &pid, sizeof(int), MSG_WAITALL);
    recv(socket_cpu, &size_dispositivo, sizeof(int), MSG_WAITALL);
    dispositivo = malloc(size_dispositivo);
    recv(socket_cpu, dispositivo, size_dispositivo, MSG_WAITALL);
    recv(socket_cpu, &tiempo, sizeof(int), MSG_WAITALL);

    //ahora mando la peticion

    


}