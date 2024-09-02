#ifndef CPU_H_
#define CPU_H_

    #include <utils-cpu.h>

    t_log* cpu_log;
    t_cpu cpu_registro;
    t_config* cpu_config;

    int fd_escucha_dispatch;
    int fd_escucha_interrupt;

    int conexion_memoria;
    
    int fd_conexion_dispatch;
    int fd_conexion_interrupt;

#endif /* CPU_H_ */