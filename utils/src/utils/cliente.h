#ifndef CONEXION_H_
#define CONEXION_H_

    #include <stdlib.h>
    #include <stdio.h>
    #include <signal.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <string.h>
    #include <sys/socket.h>
    #include <commons/log.h>

    typedef enum {
        HANDSHAKE_KERNEL = 1,
        HANDSHAKE_CPU,
        HANDSHAKE_IO,
    } opcode;

    /**
    * @fn    crear_conexion
    * @brief Inicia la conexion de un cliente a un servidor
    * 
    * @param ip          IP a la cual dirigirse para conectarse al servidor.
    * @param puerto      Puerto por el cual se conecta al servidor.
    * @return            Retorna un nuevo socket con el cual se realizaran las conexiones cliente-servidor.
    **/
    int crear_conexion(t_log* logger, char* ip, char* puerto);

#endif /* CONEXION_H_ */