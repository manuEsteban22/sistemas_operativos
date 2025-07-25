#include <procesos.h>

bool esperar_procesos(int cpu_id){
    int opcode = recibir_operacion(socket_kernel_dispatch);
    if(opcode == OC_EXEC){
        t_list* recibido = recibir_paquete(socket_kernel_dispatch);
        int* pid = list_get(recibido, 0);
        int* pc = list_get(recibido, 1);

        log_trace(logger, "Recibí PCB - PID=%d, PC=%d", *pid, *pc);

        t_pcb* pcb = malloc(sizeof(t_pcb));
        pcb->pid = *pid;
        pcb->pc = *pc;

        list_destroy_and_destroy_elements(recibido, free);

        iniciar_ciclo_de_instrucciones(pcb, cpu_id);
        free(pcb);
        return true;
    }else if(opcode < 0){
        log_error(logger, "Se cerro la conexion con Kernel, OPCODE: %d", opcode);
        return false;
    }else if(opcode == 0){
        log_trace(logger, "OPCODE: %d", opcode);
        return true;
    }
    else{
        log_error(logger, "No llego el opcode esperado: %d", opcode);
        return true;
    }
}

void bucle_esperar_procesos(int cpu_id){
    bool kernel_corriendo = true;
    while(kernel_corriendo){
        kernel_corriendo = esperar_procesos(cpu_id);
    }
}