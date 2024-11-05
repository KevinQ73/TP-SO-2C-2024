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
    
    pthread_t hilo_kernel_dispatch;
    pthread_t hilo_kernel_interrupt;

    bool flag_disconect_dispatch = true;
    bool flag_disconect_interrupt = true;
    bool interrupt_is_called = false;

    /*--------------------- Atender conexiones a la CPU ---------------------*/

    void atender_puerto_dispatch();

    void atender_puerto_interrupt();

    bool check_tid_interrupt(int fd_kernel);

    void interrupt_results(uint32_t* tid, char* motivo);

    /*-----------------------------------------------------------------------*/

    /*-------------------------- Ciclo de ejecución -------------------------*/
    
    void ejecutar_hilo(t_pid_tid pid_tid_recibido);

#endif /* CPU_H_ */