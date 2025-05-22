#ifndef KERNEL_H_
#define KERNEL_H_
#include<utils/utils.h>

extern t_log* logger;

typedef struct
{
    int socket;
    char* nombre;
} t_args_hilo;




#endif