#include <procesos.h>

t_pcb* esperar_procesos(){
    int opcode = recibir_operacion(socket_kernel_dispatch);
    if(opcode != OC_EXEC){
        log_error(logger, "No recibi exec: %d", opcode);
        return;
    }
    t_list* recibido = recibir_paquete(socket_kernel_dispatch);
    int* pid = list_get(recibido, 0);
    int* pc = list_get(recibido, 1);

    log_trace(logger, "Recibí PCB - PID=%d, PC=%d", *pid, *pc);

    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->pid = *pid;
    pcb->pc = *pc;

    list_destroy_and_destroy_elements(recibido, free);
    return pcb;
}
