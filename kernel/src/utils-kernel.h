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

    pthread_mutex_t mutex_siguiente_id;
    pthread_mutex_t mutex_cola_new;
    pthread_mutex_t mutex_cola_ready;
    pthread_mutex_t mutex_uso_fd_memoria;
    pthread_mutex_t mutex_lista_procesos_ready;

    sem_t contador_procesos_en_new;

    t_kernel levantar_datos(t_config* config);

    void iniciar_semaforos();

    t_pcb* create_pcb(char* path_instrucciones, int size_process);

    t_tcb* create_tcb(t_pcb* pcb, int prioridad);

    t_hilo_planificacion* create_hilo_planificacion(t_pcb* pcb_asociado, t_tcb* tcb_asociado);

    void poner_en_new(t_pcb* pcb);

    void poner_en_new_procesos(t_queue* cola_planificador, t_pcb* pcb);

    bool compare_pid(uint32_t* pid_1, uint32_t* pid_2);

    /*----------------------- FUNCIONES KERNEL - MEMORIA ------------------------*/

    char* avisar_creacion_proceso_memoria(char* path, int size_process, int prioridad, t_log* kernel_log);

    char* avisar_creacion_hilo_memoria(char* path, int prioridad, t_log* kernel_log);

#endif /* UTILS_KERNEL_H_ */