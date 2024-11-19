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

    t_pid_tid pid_tid_recibido;
    t_contexto* registros_cpu;
    t_dictionary* direccion_fisica;
    
    pthread_t hilo_kernel_dispatch;
    pthread_t hilo_kernel_interrupt;

    sem_t aviso_syscall;

    bool flag_disconect_dispatch = true;
    bool flag_disconect_interrupt = true;
    bool interrupt_is_called = false;
    bool segmentation_fault = false;
    
    /*--------------------- Atender conexiones a la CPU ---------------------*/

    void atender_puerto_dispatch();

    void atender_puerto_interrupt();

    bool check_tid_interrupt(int fd_kernel);

    void interrupt_results(uint32_t* tid, char* motivo);

    /*-----------------------------------------------------------------------*/

    /*-------------------------- Ciclo de ejecución -------------------------*/
    
    void ejecutar_hilo(t_pid_tid pid_tid_recibido);

    /*-------------------------- Funciones execute --------------------------*/

    void execute(t_contexto* registros_cpu, t_pid_tid pid_tid_recibido, char** instruccion_parseada);

    void execute_set(t_contexto* registro_cpu, char* registro, char* valor);

    void execute_read_mem();

    void execute_write_mem(t_contexto* registro_cpu, char* registro, char* valor);

    void execute_sum();

    void execute_sub();

    void execute_jnz();

    void execute_log();

    void execute_dump_memory();

    void execute_io();

    void execute_process_create();

    void execute_thread_create(t_contexto* registro_cpu, char* path, char* prioridad);

    void execute_thread_join();

    void execute_thread_cancel();

    void execute_mutex_create();

    void execute_mutex_lock();

    void execute_mutex_unlock();

    void execute_thread_exit();

    void execute_process_exit();

    /*-----------------------------------------------------------------------*/

#endif /* CPU_H_ */