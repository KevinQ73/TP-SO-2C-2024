#ifndef KERNEL_H_
#define KERNEL_H_

    #include <utils-kernel.h>
    #include <utils/serializacion.h>
    #include <stdbool.h>

    typedef struct {
        uint32_t milisegundos;
        uint32_t pid;
        uint32_t tid;
    } hilo_args_io;

    t_log* kernel_log;
    t_config* kernel_config;
    t_kernel kernel_registro;

    t_pcb* primer_proceso;
    t_hilo_planificacion* hilo_en_ejecucion;
    t_dictionary* thread_states;

    pthread_t hiloNew;
    pthread_t hiloPlanifCortoPlazo;
    pthread_t hilo_io;
    pthread_t hilo_quantum;

    pthread_mutex_t mutex_cola_new;
    pthread_mutex_t mutex_cola_ready;
    pthread_mutex_t mutex_cola_block;
    pthread_mutex_t mutex_hilo_exec;
    pthread_mutex_t mutex_uso_fd_memoria;
    pthread_mutex_t mutex_uso_estado_hilos;
    pthread_mutex_t mutex_lista_procesos_ready;
    pthread_mutex_t mutex_colas_multinivel_existentes;
    pthread_mutex_t mutex_siguiente_id;
    pthread_mutex_t mutex_io;
    pthread_mutex_t mutex_interrupt;

    sem_t contador_procesos_en_new;
    sem_t aviso_exit_proceso;
    sem_t kernel_activo;

    t_queue* cola_new;
    t_queue* cola_exit;

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

    bool termino_proceso = false;
    bool hilo_desalojado = false;

    t_list* procesos_creados;
    t_list* cola_ready;
    t_list* lista_hilo_en_ejecucion;
    t_list* lista_t_hilos_bloqueados;
    t_list* lista_prioridades;
    t_list* lista_colas_multinivel;

    /*----------------------- FUNCIONES DE INICIALIZACIÓN -----------------------*/

    void iniciar_listas();

    void iniciar_semaforos();

    void iniciar_colas();

    void iniciar_primer_proceso(char* path, char* size);
    
    void iniciar_planificacion();

    /*------------------------- PLANIFICADOR LARGO PLAZO ------------------------*/

    void* planificador_largo_plazo();

    void* inicializar_pcb_en_espera();

    void reintentar_inicializar_pcb_en_espera(t_pcb* pcb);

    void poner_en_new(t_pcb* pcb);

    void poner_en_exit(uint32_t pid, uint32_t tid);

    void eliminar_tcb_de_pcb(t_hilo_planificacion* hilo);

    void finalizar_hilos_de_proceso(uint32_t pid, uint32_t tid);

    /*------------------------- PLANIFICADOR CORTO PLAZO ------------------------*/

    void* planificador_corto_plazo();

    t_hilo_planificacion* obtener_hilo_segun_algoritmo(char* planificacion);

    void* poner_en_ready(t_hilo_planificacion* hilo_del_proceso);

    void* poner_en_block(t_hilo_planificacion* hilo_del_proceso);

    void agregar_proceso_activo(t_pcb* pcb);

    t_hilo_planificacion* remover_de_ready(uint32_t pid, uint32_t tid, uint32_t prioridad);

    t_hilo_planificacion* remover_de_block(uint32_t pid, uint32_t tid);

    /*--------------------------- FUNCIONES DE COLAS ----------------------------*/

    void queue_push_by_priority(t_hilo_planificacion* hilo_del_proceso);

    bool queue_find_by_priority(t_list* lista_colas_multinivel, int prioridad);

    t_cola_prioridades* queue_get_by_priority(t_list* lista_colas_multinivel, int prioridad);

    int maximum_priority_multilevel_queues_schedule(t_list* lista_colas_multinivel);

    /*-------------------------- FUNCIONES DE LISTAS ----------------------------*/

    /*--------------------------- FUNCIONES DE HILOS ----------------------------*/

    t_hilo_planificacion* thread_find_by_maximum_priority(t_list* lista_prioridades);

    t_hilo_planificacion* thread_find_by_priority_schedule(t_list* lista_prioridades);

    t_hilo_planificacion* thread_find_by_multilevel_queues_schedule(t_list* lista_colas_multinivel);

    t_hilo_planificacion* thread_find_by_tid(t_list* lista, t_pid_tid pid_tid);

    t_hilo_planificacion* thread_remove_by_tid(t_list* lista, uint32_t pid, uint32_t tid);

    void liberar_hilos_bloqueados_por_tid(t_hilo_planificacion* hilo);

    bool is_thread_exist(uint32_t pid, uint32_t tid);

    t_tcb* tid_find(t_list* lista_tcb, uint32_t tid);

    t_tcb* tcb_remove_by_tid(t_pcb* pcb, uint32_t tid);

    /*---------------------------- FUNCIONES DE PCB -----------------------------*/

    t_pcb* active_process_find_by_pid(uint32_t pid);

    t_pcb* active_process_remove_by_pid(uint32_t pid);

    /*----------------------- FUNCIONES DE ESTADO DE HILOS ----------------------*/

    void create_process_state(uint32_t pid);

    void create_thread_state(uint32_t pid, uint32_t tid, uint32_t prioridad);

    t_list* get_list_thread_state(uint32_t pid);

    t_thread_state* get_thread_state(uint32_t pid, uint32_t tid);

    t_thread_state* find_thread_state_by_tid(t_list* lista, uint32_t tid);

    void change_thread_state(uint32_t pid, uint32_t tid, estado_proceso estado);

    void add_thread_state_tid_blocked(uint32_t pid, uint32_t tid_bloqueado, uint32_t tid_bloqueante);

    void remove_thread_state_tid_blocked(uint32_t pid, uint32_t tid_bloqueado);

    bool exist_thread_blocked_by_tid(uint32_t pid, uint32_t tid_bloqueado);

    /*--------------------------- FUNCIONES DE MUTEX ----------------------------*/

    t_mutex* find_mutex_by_name(t_list* lista_de_mutex, char* name);

    bool mutex_any_satisfy_by_name(t_list* lista_de_mutex, char* name);

    bool mutex_taken_by_tid(t_list* lista_de_mutex, uint32_t tid, char* name);

    bool is_mutex_exist(uint32_t pid, char* nombre);

    bool is_mutex_taken_by_tid(uint32_t pid, uint32_t tid, char* nombre);

    int is_mutex_taken(uint32_t pid, char* nombre);

    void block_thread_mutex(uint32_t pid, uint32_t tid, char* nombre);

    void unblock_thread_mutex(uint32_t pid, char* nombre);

    void lock_mutex_to_thread(uint32_t pid, uint32_t tid, char* nombre);

    void unlock_mutex_to_thread(uint32_t pid, char* nombre);

    void add_mutex_to_process(t_mutex* mutex, uint32_t pid);

    void block_tid_by_mutex(t_list* lista_de_mutex, char* nombre, uint32_t tid);

    uint32_t unblock_tid_by_mutex(t_list* lista_de_mutex, char* nombre);

    void add_tid_to_mutex(t_list* lista_de_mutex, char* nombre, uint32_t tid);

    void left_tid_to_mutex(t_list* lista_de_mutex, char* nombre);

    uint32_t mutex_value(t_list* lista_de_mutex, char* name);

    /*---------------------------- FUNCIONES EXECUTE ----------------------------*/

    void* ejecutar_hilo(t_hilo_planificacion* hilo_a_ejecutar);

    //void* esperar_tid_cpu();

    t_hilo_planificacion* desalojar_hilo();

    void* ejecutar_io(void* ptr);

    /*----------------------- FUNCIONES KERNEL - MEMORIA ------------------------*/

    char* avisar_creacion_proceso_memoria(int* pid, int* size_process, t_log* kernel_log);

    char* avisar_creacion_hilo_memoria(int* pid, int* tid, char* path, int* prioridad, t_log* kernel_log);

    char* avisar_fin_proceso_memoria(uint32_t* pid);

    char* avisar_fin_hilo_memoria(uint32_t* pid, uint32_t* tid);

    /*------------------------- FUNCIONES KERNEL - CPU --------------------------*/

    void enviar_aviso_syscall(char* mensaje, cod_inst* codigo_instruccion);

    /*--------------------------------- SYSCALLS --------------------------------*/

    void* operacion_a_atender();

    void* syscall_process_create(t_buffer* buffer, t_pid_tid pid_tid);

    void* syscall_process_exit(t_pid_tid pid_tid_recibido);

    void* syscall_thread_create(t_buffer* buffer, t_pid_tid pid_tid);

    void* syscall_thread_join(t_buffer* buffer, t_pid_tid pid_tid);

    void* syscall_thread_exit(t_pid_tid pid_tid);

    void* syscall_thread_cancel(t_buffer* buffer, t_pid_tid pid_tid);

    void* syscall_mutex_create(t_buffer* buffer, t_pid_tid pid_tid);

    void* syscall_mutex_lock(t_buffer* buffer, t_pid_tid pid_tid);

    void* syscall_mutex_unlock(t_buffer* buffer, t_pid_tid pid_tid);

    void* syscall_dump_memory(t_pid_tid pid_tid);

    void* syscall_io(t_buffer* buffer, t_pid_tid pid_tid);

    /*--------------------------- FINALIZACIÓN DE TADS --------------------------*/

    void process_and_threads_destroy(uint32_t pid);

    void t_hilo_planificacion_destroy(t_hilo_planificacion* hilo);

    void pcb_destroy(t_pcb* pcb);

    void process_mutex_destroy(t_mutex* mutex);

    /*------------------------- FINALIZACIÓN DEL MODULO -------------------------*/

    void eliminar_listas();

    void eliminar_colas();

    /*------------------------------- MISCELANEO --------------------------------*/

    uint32_t siguiente_pid();

    void signal_handler(int sig);

    void iniciar_quantum();
    
#endif /* KERNEL_H_ */