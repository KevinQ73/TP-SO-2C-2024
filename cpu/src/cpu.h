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
    t_dictionary* registros_cpu;
    
    pthread_t* hilo_kernel_dispatch;
    pthread_t* hilo_kernel_interrupt;

    bool flag_dispatch = true;

    void atender_puerto_dispatch();

    void atender_puerto_interrupt();

    void ejecutar_proceso(t_pid_tid pid_tid_recibido);

#endif /* CPU_H_ */