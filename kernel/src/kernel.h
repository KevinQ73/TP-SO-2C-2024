#ifndef KERNEL_H_
#define KERNEL_H_

    #include <utils-kernel.h>
    #include <utils/serializacion.h>
    t_log* kernel_log;
    t_config* kernel_config;
    t_kernel kernel_registro;

    int conexion_cpu;
    int conexion_memoria;
    int fd_conexion_dispatch;
    int fd_conexion_interrupt;
    int proceso_ejecutando;

    t_pcb* primer_proceso;
    
	char* ip_cpu;
	char* ip_memoria;

    char* puerto_memoria;
	char* puerto_cpu_dispatch;
    char* puerto_cpu_interrupt;

    t_list* procesos_creados;
    t_list* hilo_exec;
    t_list* hilos_block;

    pthread_t hiloNew;
    pthread_t hiloPlanifCortoPlazo;

    pthread_mutex_t mutex_siguiente_id;
    pthread_mutex_t mutex_cola_new;
    pthread_mutex_t mutex_cola_ready;
    pthread_mutex_t mutex_uso_fd_memoria;
    pthread_mutex_t mutex_lista_procesos_ready;

    sem_t contador_procesos_en_new;
    sem_t aviso_exit_proceso;

    t_queue* cola_new;
    t_queue* cola_new_procesos;
    t_queue* cola_ready;
    t_queue* cola_ready_procesos;
    t_queue* cola_ready_fifo;
    t_queue* cola_blocked;
    t_queue* cola_execute;
    t_queue* cola_exit;
    t_queue* cola_prioridad_maxima;
    t_queue* cola_prioridad_1;
    t_queue* cola_prioridad_2;
    t_queue* cola_prioridad_3;
    t_queue* cola_prioridad_4;
    t_queue* cola_prioridad_5;
    t_queue* cola_prioridad_6;
    t_queue* cola_prioridad_7;

    /*----------------------- FUNCIONES DE INICIALIZACIÓN -----------------------*/

    void iniciar_semaforos();

    void iniciar_colas();

    void iniciar_primer_proceso(char* path, char* size);
    
    void iniciar_planificacion();

    /*------------------------ FUNCIONES DE PLANIFICACIÓN -----------------------*/

    int aplicar_tid(t_pcb* pcb);

    void* planificador_largo_plazo();

    void* planificador_corto_plazo();

    void poner_en_new(t_pcb* pcb);

    void* inicializar_pcb_en_espera();

    void reintentar_inicializar_pcb_en_espera(t_pcb* pcb);

    void* poner_en_ready(t_hilo_planificacion* hilo_del_proceso);

    void poner_en_ready_procesos();

    void* poner_en_block();

    /*---------------------------- FUNCIONES EXECUTE ----------------------------*/

    void* ejecutar_hilo(t_hilo_planificacion* hilo_a_ejecutar);

    void* esperar_tid_cpu();

    void* desalojar_hilo(t_hilo_planificacion* hilo_a_desalojar);

    void liberar_hilos_bloqueados(t_hilo_planificacion* hilo);

    /*----------------------- FUNCIONES KERNEL - MEMORIA ------------------------*/

    char* avisar_creacion_proceso_memoria(char* path, int* size_process, int* prioridad, int socket_memoria, t_log* kernel_log);

    char* avisar_creacion_hilo_memoria(char* path, int* prioridad, int socket_memoria, t_log* kernel_log);

    /*--------------------------------- SYSCALLS --------------------------------*/

    void* syscalls_a_atender();

    void* syscall_process_create(uint32_t pid_solicitante, uint32_t tid_solicitante);

    void* syscall_process_exit(t_pcb* pcb);

    void* syscall_thread_create();

    void* syscall_thread_join();

    void* syscall_thread_exit(t_hilo_planificacion* hilo);

    /*--------------------------- FINALIZACIÓN DE TADS --------------------------*/

    void t_hilo_planificacion_destroy(t_hilo_planificacion* hilo);

    void pcb_destroy(t_pcb* pcb);

    /*------------------------- FINALIZACIÓN DEL MODULO -------------------------*/

    void eliminar_listas();

    void eliminar_colas();

    /*------------------------------- MISCELANEO --------------------------------*/

    t_pcb* obtener_pcb_from_hilo(t_hilo_planificacion* hilo);
    
#endif /* KERNEL_H_ */