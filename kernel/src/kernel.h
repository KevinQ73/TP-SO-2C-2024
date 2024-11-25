#ifndef KERNEL_H_
#define KERNEL_H_

    #include <utils-kernel.h>
    #include <utils/serializacion.h>
    #include <stdbool.h>

    t_log* kernel_log;
    t_config* kernel_config;
    t_kernel kernel_registro;

    t_pcb* primer_proceso;
    t_hilo_planificacion* hilo_en_ejecucion;

    int conexion_cpu;
    int conexion_memoria;
    int fd_conexion_dispatch;
    int fd_conexion_interrupt;
    int proceso_ejecutando;
    int pid_siguiente;
    int pid_actual = 0;

	char* ip_cpu;
	char* ip_memoria;
    char* puerto_memoria;
	char* puerto_cpu_dispatch;
    char* puerto_cpu_interrupt;

    bool termino_proceso;

    t_list* procesos_creados;
    t_list* hilos_block;
    t_list* lista_prioridades;
    t_list* lista_colas_multinivel;

    pthread_t hiloNew;
    pthread_t hiloPlanifCortoPlazo;
    pthread_t hilo_io;
    pthread_t hilo_quantum;

    pthread_mutex_t mutex_cola_new;
    pthread_mutex_t mutex_cola_ready;
    pthread_mutex_t mutex_uso_fd_memoria;
    pthread_mutex_t mutex_lista_procesos_ready;
    pthread_mutex_t mutex_colas_multinivel_existentes;
    pthread_mutex_t mutex_siguiente_id;

    sem_t contador_procesos_en_new;
    sem_t aviso_exit_proceso;
    sem_t kernel_activo;

    t_queue* cola_new;
    t_queue* cola_ready_fifo;
    t_queue* cola_exit;

    /*----------------------- FUNCIONES DE INICIALIZACIÓN -----------------------*/

    void iniciar_semaforos();

    void iniciar_colas();

    void iniciar_primer_proceso(char* path, char* size);
    
    void iniciar_planificacion();

    /*------------------------ FUNCIONES DE PLANIFICACIÓN -----------------------*/

    void* planificador_largo_plazo();

    void* inicializar_pcb_en_espera();

    void reintentar_inicializar_pcb_en_espera(t_pcb* pcb);

    void* planificador_corto_plazo();

    t_hilo_planificacion* obtener_hilo_segun_algoritmo(char* planificacion);

    void poner_en_new(t_pcb* pcb);

    void* poner_en_ready(t_hilo_planificacion* hilo_del_proceso);

    void poner_en_ready_procesos();

    void* poner_en_block();

    void liberar_hilos_bloqueados(t_hilo_planificacion* hilo);

    /*--------------------------- FUNCIONES DE COLAS ----------------------------*/

    void queue_push_by_priority(t_hilo_planificacion* hilo_del_proceso);

    bool queue_find_by_priority(t_list* lista_colas_prioridades, int prioridad);

    t_cola_prioridades* queue_get_by_priority(t_list* lista_colas_prioridades, int prioridad);

    t_hilo_planificacion* list_find_by_maximum_priority(t_list* lista_prioridades);

    t_hilo_planificacion* thread_find_by_priority_schedule(t_list* lista_prioridades);

    t_hilo_planificacion* thread_find_by_multilevel_queues_schedule(t_list* lista_colas_multinivel);

    t_hilo_planificacion* thread_find_by_tid(t_list* lista, uint32_t tid);

    pthread_mutex_t* find_by_name(t_list* lista_de_mutex, char* name);

    t_pcb* list_find_by_pid(uint32_t pid);

    t_tcb* tid_find(t_list* lista_tcb, uint32_t tid);

    int maximum_priority_multilevel_queues_schedule(t_list* lista_colas_prioridades);

    /*---------------------------- FUNCIONES EXECUTE ----------------------------*/

    void* ejecutar_hilo(t_hilo_planificacion* hilo_a_ejecutar);

    void* esperar_tid_cpu();

    void* desalojar_hilo(t_hilo_planificacion* hilo_a_desalojar);

    void* ejecutar_io (uint32_t milisegundos);

    /*----------------------- FUNCIONES KERNEL - MEMORIA ------------------------*/

    char* avisar_creacion_proceso_memoria(int* pid, int* size_process, t_log* kernel_log);

    char* avisar_creacion_hilo_memoria(int* pid, int* tid, char* path, int* prioridad, t_log* kernel_log);

    char* avisar_fin_proceso_memoria(uint32_t pid);

    char* avisar_fin_hilo_memoria(uint32_t pid, uint32_t tid);

    /*------------------------- FUNCIONES KERNEL - CPU --------------------------*/

    void enviar_aviso_syscall(char* mensaje, cod_inst* codigo_instruccion);

    /*--------------------------------- SYSCALLS --------------------------------*/

    void* operacion_a_atender();

    void* syscall_process_create(uint32_t pid_solicitante, uint32_t tid_solicitante);

    void* syscall_process_exit();

    void* syscall_thread_create(t_buffer* buffer, uint32_t pid_solicitante, uint32_t tid_solicitante);

    void* syscall_thread_join();

    void* syscall_thread_exit(t_hilo_planificacion* hilo);

    void* syscall_thread_cancel(uint32_t tid);

    void* syscall_mutex_create(char* nombreMutex, uint32_t pid);

    void* syscall_mutex_lock(char* nombreMutex ,t_pid_tid pid_tid_recibido);

    void* syscall_mutex_unlock(char* nombreMutex ,t_pid_tid pid_tid_recibido);

    void* syscall_dump_memory();

    void* syscall_io(uint32_t milisegundos_de_trabajo);

    /*--------------------------- FINALIZACIÓN DE TADS --------------------------*/

    void t_hilo_planificacion_destroy(t_hilo_planificacion* hilo);

    void pcb_destroy(t_pcb* pcb);

    /*------------------------- FINALIZACIÓN DEL MODULO -------------------------*/

    void eliminar_listas();

    void eliminar_colas();

    /*------------------------------- MISCELANEO --------------------------------*/

    uint32_t siguiente_pid();

    void signal_handler(int sig);

    void iniciar_quantum();
    
#endif /* KERNEL_H_ */