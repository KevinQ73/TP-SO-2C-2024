#ifndef UTILS_KERNEL_H_
#define UTILS_KERNEL_H_

    #include <pthread.h>
    #include <stdlib.h>
    #include <stdio.h>
    #include <utils/cliente.h>
    #include <utils/files.h>
    #include <utils/servidor.h>
    #include <commons/collections/list.h>
    #include <commons/collections/queue.h>
    #include <commons/string.h>
    #include <readline/readline.h>

    typedef struct{
        char* ip_memoria;
        char* puerto_memoria;
        char* ip_cpu;
        char* puerto_cpu_dispatch;
        char* puerto_cpu_interrupt;
        char* algoritmo_planificacion;
        int quantum;
        char* log_level;
    } t_kernel;

    t_kernel levantar_datos(t_config* config);

#endif /* UTILS_KERNEL_H_ */