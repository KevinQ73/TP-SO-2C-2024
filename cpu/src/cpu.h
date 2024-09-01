#ifndef CPU_H_
#define CPU_H_

    #include <utils-cpu.h>

    t_log* cpu_log;
    t_cpu cpu_registro;
    t_config* cpu_config;

    int fd_server_dispatch;
    int fd_server_interrupt;

    int fd_kernel_dispatch;
    int fd_kernel_interrupt;

#endif /* CPU_H_ */