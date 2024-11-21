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

    void execute_read_mem(t_contexto* registro_cpu, t_pid_tid pid_tid_recibido, char* registro_datos, char* registro_direccion);

    void execute_write_mem(t_contexto* registro_cpu, t_pid_tid pid_tid_recibido, char* registro_direccion, char* registro_datos);

    void execute_sum(t_contexto* registro_cpu, char* registro_destino, char* registro_origen);

    void execute_sub(t_contexto* registro_cpu, char* registro_destino, char* registro_origen);

    void execute_jnz(t_contexto* registro_cpu, char* registro, char* instruccion);

    void execute_log(t_contexto* registro_cpu, char* registro);

    void execute_dump_memory(t_contexto* registro_cpu);

    void execute_io(t_contexto* registro_cpu, char* tiempo);

    void execute_process_create(t_contexto* registro_cpu, char* path, char* size_process, char* prioridad_hilo_main);

    void execute_thread_create(t_contexto* registro_cpu, char* path, char* prioridad);

    void execute_thread_join(t_contexto* registro_cpu, char* tid_join);

    void execute_thread_cancel(t_contexto* registro_cpu, char* tid_cancel);

    void execute_mutex_create(t_contexto* registro_cpu, char* recurso);

    void execute_mutex_lock(t_contexto* registro_cpu, char* recurso);

    void execute_mutex_unlock(t_contexto* registro_cpu, char* recurso);

    void execute_thread_exit(t_contexto* registro_cpu);

    void execute_process_exit(t_contexto* registro_cpu);

    /*-----------------------------------------------------------------------*/

#endif /* CPU_H_ */