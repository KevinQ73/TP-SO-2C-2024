#ifndef UTILS_FILESYSTEM_H_
#define UTILS_FILESYSTEM_H_

    #include <stdlib.h>
    #include <stdio.h>
    #include <pthread.h>
    #include <utils/serializacion.h>
    #include <utils/files.h>
    #include <utils/servidor.h>
    #include <commons/string.h>

    typedef struct{
        char* puerto_escucha;
        char* path_mount_dir;
        uint32_t block_size;
        uint32_t block_count;
        uint32_t retardo_acceso_bloque;
        char* log_level;
    } t_filesystem;

    t_filesystem levantar_datos(t_config* config);

#endif /* UTILS_FILESYSTEM_H_ */