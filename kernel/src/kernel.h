#ifndef KERNEL_H_
#define KERNEL_H_

    #include <utils-kernel.h>

    t_log* kernel_log;
    t_config* kernel_config;
    t_kernel kernel_registro;

    int conexion_cpu;
    int conexion_memoria;
    int conexion_cpu_interrupt;
    int conexion_cpu_dispatch;

	char* ip_cpu;
	char* ip_memoria;

    char* puerto_memoria;
	char* puerto_cpu_dispatch;
    char* puerto_cpu_interrupt;

#endif /* KERNEL_H_ */