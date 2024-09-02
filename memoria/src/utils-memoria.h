#ifndef MEMORIA_SRC_UTILS_MEMORIA_H_
#define MEMORIA_SRC_UTILS_MEMORIA_H_

    #include <pthread.h>
    #include <stdlib.h>
    #include <stdio.h>
    #include <utils/cliente.h>
    #include <utils/files.h>
    #include <utils/servidor.h>
    #include <utils/serializacion.h>
    #include <commons/collections/list.h>
    #include <commons/collections/queue.h>
    #include <commons/string.h>
    #include <readline/readline.h>

    typedef struct{
        char* puerto_escucha;
        char* ip_filesystem;
        char* puerto_filesystem;
        int tam_memoria;
        char* path_instrucciones;
        int retardo_respuesta;
        char* esquema;
        char* algoritmo_busqueda;
        char** particiones;
        char* log_level;
    } t_memoria;

    t_memoria levantar_datos_memoria(t_config* config);

#endif /* MEMORIA_SRC_UTILS_MEMORIA_H_ */
