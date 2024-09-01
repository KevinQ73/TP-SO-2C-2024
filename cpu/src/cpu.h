#ifndef CPU_H_
#define CPU_H_

    #include <pthread.h>
    #include <stdlib.h>
    #include <stdio.h>
    #include <utils/cliente.h>
    #include <utils/files.h>
    #include <utils/servidor.h>
    #include <utils/serializacion.h>
    #include <commons/string.h>

    typedef struct{
        char* ip_memoria;
        char* puerto_memoria;
        char* puerto_cpu_dispatch;
        char* puerto_cpu_interrupt;
        char* log_level;
    }t_cpu;

    t_log* cpu_log;
    t_cpu cpu_registro;

    t_config* cpu_config;
    
    t_cpu levantar_datos();


    int fd_server_dispatch;
    int fd_server_interrupt;

    int fd_kernel_dispatch;
    int fd_kernel_interrupt;

#endif /* CPU_H_ */