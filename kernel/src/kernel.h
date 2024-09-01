#ifndef KERNEL_H_
#define KERNEL_H_

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
}t_kernel;

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

    t_kernel levantar_datos();

#endif /* KERNEL_H_ */