#ifndef UTILS_CPU_H_
#define UTILS_CPU_H_

    #include <pthread.h>
    #include <stdlib.h>
    #include <stdio.h>
    #include <utils/serializacion.h>
    #include <utils/cliente.h>
    #include <utils/files.h>
    #include <utils/servidor.h>
    #include <commons/string.h>

    typedef struct{
        char* ip_memoria;
        char* puerto_memoria;
        char* puerto_cpu_dispatch;
        char* puerto_cpu_interrupt;
        char* log_level;
    } t_cpu;

    t_cpu levantar_datos(t_config* config);

#endif /* UTILS_CPU_H_ */