#ifndef MEMORIA_SRC_MEMORIA_H_
#define MEMORIA_SRC_MEMORIA_H_

    #include <utils-memoria.h>

    t_log* memoria_log;
    t_config* memoria_config;
    t_memoria memoria_registro;

    int conexion_filesystem;
    int fd_conexion_cpu;
    int conexion_cpu;
    int fd_conexion_kernel;
    int conexion_kernel;

#endif /* MEMORIA_SRC_MEMORIA_H_ */
