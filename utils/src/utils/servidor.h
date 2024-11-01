#ifndef SERVIDOR_H_
#define SERVIDOR_H_

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <errno.h>
    #include <sys/socket.h>
    #include <commons/config.h>
    #include <commons/log.h>
    #include <commons/error.h>
    #include <utils/cliente.h>

    typedef enum{
        HANDSHAKE_OK = 400,
        HANDSHAKE_ERROR,
    } handshake_codes;

    /**
    * @fn    iniciar_servidor
    * @brief Crea el socket para permanecer escuchando clientes
    * 
    * @param puerto Ruta al puerto escucha del cliente
    * @return       Retorna el socket creado para poder realizar la conexión.
    */
    int iniciar_servidor(t_log* logger, const char* name, char* puerto);

    /**
    * @fn    esperar_cliente
    * @brief Espera la conexión de los distintos clientes del servidor
    * 
    * @param socket_servidor Socket del servidor para que se realice el accept.
    * @return                Retorna el socket con el cual tanto el servidor como
    *                        el cliente podrán conectarse.
    */
    int esperar_cliente(t_log* logger, const char* name, int socket_servidor);

    void manejar_error_accept(int errnum, t_log* log);

    /**
    * @fn    terminar_modulo
    * @brief Finaliza la sesion del modulo en particular
    * 
    * @param socket Socket a finalizar.
    * @param logger Logger a finalizar.
    * @param config Config a finalizar.
    */
    void terminar_modulo(int socket, t_log* logger, t_config* config);

#endif /* SERVIDOR_H_ */