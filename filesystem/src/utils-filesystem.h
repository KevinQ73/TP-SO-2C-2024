#ifndef UTILS_FILESYSTEM_H_
#define UTILS_FILESYSTEM_H_

    #include <stdlib.h>
    #include <stdio.h>
    #include <pthread.h>
    #include <utils/serializacion.h>
    #include <utils/files.h>
    #include <utils/servidor.h>
    #include <commons/string.h>
    #include <string.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <math.h>
    #include <commons/bitarray.h>
    #include <sys/mman.h>
    #include <commons/config.h>
    #include <commons/collections/list.h>
    #include <dirent.h>
    #include <stdbool.h>
    #include <memory.h>

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