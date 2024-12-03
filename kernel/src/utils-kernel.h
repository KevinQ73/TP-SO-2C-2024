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

    typedef struct{
        t_list* cola;
        int prioridad;
    } t_cola_prioridades;

    /*----------------------- FUNCIONES DE INICIALIZACIÓN -----------------------*/

    t_kernel levantar_datos(t_config* config);

    /*----------------------- FUNCIONES CREATE DE STRUCTS -----------------------*/

    t_pcb* create_pcb(char* path_instrucciones, int size_process);

    t_tcb* create_tcb(t_pcb* pcb_padre, int prioridad);

    t_hilo_planificacion* create_hilo_planificacion(t_pcb* pcb_asociado, t_tcb* tcb_asociado);

    t_cola_prioridades* create_priority_queue(int prioridad);

    int crear_conexion_con_memoria(t_log* kernel_log, char* ip, char* puerto);

    t_mutex* create_mutex(char* nombreMutex);

    /*------------------------------ MISCELANEO --------------------------------*/

    bool compare_pid(uint32_t* pid_1, uint32_t* pid_2);

    uint32_t siguiente_tid(int siguiente_tid);

    void multilevel_queue_destroy(t_cola_prioridades* cola);

#endif /* UTILS_KERNEL_H_ */