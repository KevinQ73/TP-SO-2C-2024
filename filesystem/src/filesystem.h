#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

    #include <utils-filesystem.h>

    t_log* filesystem_log;
    t_config* filesystem_config;
    t_filesystem filesystem_registro;

    int fd_escucha_memoria;
    int fd_conexion_memoria;

#endif /* FILESYSTEM_H_ */