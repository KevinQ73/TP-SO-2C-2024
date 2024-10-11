#ifndef UTILS_KERNEL_H_
#define UTILS_KERNEL_H_

    #include <pthread.h>
    #include <stdlib.h>
    #include <stdio.h>
    #include <utils/cliente.h>
    #include <utils/files.h>
    #include <utils/servidor.h>
    #include <utils/serializacion.h>
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

    t_pcb* create_pcb();

    t_tcb* create_tcb();

    void poner_en_ready();

    void poner_en_new(t_queue* cola_planificador, t_hilo_planificacion* hilo_del_proceso);

    void poner_en_new_procesos(t_queue* cola_planificador, t_pcb* pcb);

    bool compare_pid(uint32_t* pid_1, uint32_t* pid_2);
    
#endif /* UTILS_KERNEL_H_ */