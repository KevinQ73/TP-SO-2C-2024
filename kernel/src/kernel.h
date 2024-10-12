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

    t_list* proceso_creados;
    t_list* hilo_exec;
    t_list* hilos_block;

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

    pthread_t hiloNew;
    pthread_t hiloPlanifCortoPlazo;

    /*----------------------- FUNCIONES DE INICIALIZACIÓN -----------------------*/

    void iniciar_primer_proceso();
    
    void iniciar_colas();

    void iniciar_planificacion();

    // -------------------- FUNCIONES DE PLANIFICACIÓN --------------------

    void* planificador_largo_plazo();

    void* planificador_corto_plazo();

    void eliminar_listas();

    void* poner_en_ready(t_hilo_planificacion* hilo_del_proceso);
    // --------------------- Creacion de procesos ----------------------

    void poner_en_ready_procesos();

    void* inicializar_pcb_en_espera();

    void reintentar_inicializar_pcb_en_espera(t_pcb* pcb);

    void* syscall_process_create(uint32_t pid_solicitante, uint32_t tid_solicitante);

    void* peticion_iniciar_proceso(t_pcb* pcb, t_hilo_planificacion* primer_hilo_asociado);

    void* peticion_finalizar_proceso(t_pcb* pcb);

    int aplicar_tid(t_pcb* pcb);

    void* peticion_crear_hilo(void);

    void* finalizar_hilo(t_tcb* tcb);

    void* syscalls_a_atender();

    void* mover_a_block(void);

    void syscall_thread_join();
//-----------------------------FUNCIONES DE EJECUCUION---------------------------------------
    void* ejecutar_hilo(t_hilo_planificacion* hilo_a_ejecutar);
    void* esperar_tid_cpu(void);
    void* desalojar_hilo(t_hilo_planificacion* hilo_a_desalojar);

//---------------------------FUNCIONES DE FINALIZACION--------------------------------------

    void t_hilo_planificacion_destroy(void* ptr);
    void pcb_destroy(t_pcb* pcb);
    void eliminar_listas();
    void eliminar_colas();
    
#endif /* KERNEL_H_ */