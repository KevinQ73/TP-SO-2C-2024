#ifndef SERIALIZACION_H_
#define SERIALIZACION_H_

    #include <stdio.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <string.h>
    #include <semaphore.h>
    #include <commons/collections/list.h>
    #include <commons/log.h>
    #include <sys/socket.h>

    typedef enum{
        // INSTRUCCIONES DE CPU
        SET,
        READ_MEM,
        WRITE_MEM,
        SUM,
        SUB,
        JNZ,
        LOG,
        // SYSCALLS
        DUMP_MEMORY,
        IO,
        PROCESS_CREATE,
        THREAD_CREATE,
        THREAD_JOIN,
        THREAD_CANCEL,
        MUTEX_CREATE,
        MUTEX_LOCK,
        MUTEX_UNLOCK,
        THREAD_EXIT,
        PROCESS_EXIT,
    } cod_inst;

    typedef enum{
        NEW_STATE = 200,
        READY_STATE,
        EXEC_STATE,
        BLOCKED_STATE,
        EXIT_STATE,
    } estado_proceso;
    
    typedef enum{
        PRIORIDAD_MAXIMA,
        PRIORIDAD_1,
        PRORIDAD_2,
        PRORIDAD_3,
        PRORIDAD_4,
        PRORIDAD_5,
        PRORIDAD_6,
        PRORIDAD_7,
    } orden_prioridad;

    typedef struct{
        int size;
        void* stream;
    } t_buffer;

    typedef struct{
        cod_inst codigo_operacion;
        t_buffer* buffer;
    } t_paquete;

    typedef struct{
        uint8_t ax;
        uint8_t bx;
        uint8_t cx;
        uint8_t dx;
        uint32_t eax;
        uint32_t ebx;
        uint32_t ecx;
        uint32_t edx;
        uint32_t si;
        uint32_t di;
    } t_registros;

    typedef struct{
        uint32_t pid;
        t_list* tid;
        t_list* mutex_asociados
        int program_counter;
        estado_proceso estado;
        t_registros registros_cpu;
    } t_pcb;

    typedef struct{
        uint32_t tid;
        orden_prioridad prioridad;
    } t_tid;

    /**
    * @fn    crear_buffer
    * @brief Arma una instancia t_buffer que almacena un stream void* y su tamaño.
    * 
    * @param paquete          Recibe un puntero a una instancia de t_paquete* que contendrá el buffer.
    */
    void crear_buffer(t_paquete* paquete);

    /**
    * @fn    enviar_mensaje
    * @brief Envía un mensaje desde el cliente al servidor
    * 
    * @param mensaje          Mensaje a ser enviado.
    * @param socket_cliente   Socket del cliente para realizar la conexión.
    */
    void enviar_mensaje(char* mensaje, int socket_cliente);

    /**
    * @fn    recibir_mensaje
    * @brief Recibe el mensaje en forma de stream desde el cliente.
    * 
    * @param socket_cliente   Socket del cliente que envió el mensaje.
    * @param logger_servidor  Instancia de logger del módulo correspondiente para poder informar
    *                         el resultado del mensaje.
    */
    void recibir_mensaje(int socket_cliente, t_log* logger_servidor);

    /**
    * @fn    recibir_buffer
    * @brief Obtiene el buffer enviado desde el cliente.
    * 
    * @param size             Tamaño en bytes del buffer
    * @param socket_cliente   Socket del cliente que envió el paquete con el buffer dentro.
    * @return                 Stream de tipo void* con los datos del buffer.
    */
    void* recibir_buffer(int* size, int socket_cliente);
    
    /**
    * @fn    recibir_operacion
    * @brief Recibe el socket del cliente y obtiene el código de operación.
    * 
    * @param socket_cliente   Socket del cual obtiene el código de operación.
    * @return                 Un Integer con el código de operación.
    */
    int recibir_operacion(int socket_cliente);

    /**
    * @fn    crear_paquete
    * @brief Arma un paquete de tipo t_paquete.
    * 
    * @param instruccion      Instrucción de tipo cod_inst que servirá para avisar al servidor de que manera
    *                         debe atender la petición.
    * @return                 Un puntero a una instancia t_paquete.
    */
    t_paquete* crear_paquete(cod_inst instruccion);

    /**
    * @fn    enviar_paquete
    * @brief Envío del paquete al servidor.
    * 
    * @param paquete          Una instancia t_paquete a ser enviado.
    * @param socket_cliente   Socket al cual se envia el paquete.
    */
    void enviar_paquete(t_paquete* paquete, int socket_cliente);

    /**
    * @fn    recibir_paquete
    * @brief Se encarga de recibir el paquete enviado desde el cliente.
    * 
    * @param socket_cliente   Socket del cual se recibe el paquete.
    * @return                 Un puntero a una lista t_list con los valores enviados por el cliente.
    */
    t_list* recibir_paquete(int socket_cliente);

    /**
    * @fn    eliminar_paquete
    * @brief Se encarga de liberar la memoria reservada para el paquete.
    * 
    * @param paquete          Una instancia t_paquete a ser liberado.
    */
    void eliminar_paquete(t_paquete* paquete);

    /**
    * @fn    agregar_a_paquete
    * @brief Agrega un bloque stream más al paquete.
    * 
    * @param paquete          Puntero a una instancia t_paquete que agregará el stream nuevo.
    * @param valor            Stream a ser agregado en el paquete.
    * @param tamanio          Tamaño total del stream a enviar. Ejemplo: &tamanio_stream donde tamanio_stream 
    *                         se declara previamente como Integer.
    */
    void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);

    /**
    * @fn    serializar_paquete
    * @brief Serializa en un stream el paquete a enviar al servidor.
    * 
    * @param paquete          Una instancia t_paquete a ser enviado.
    * @param bytes            Tamaño en bytes que tendrá el stream a enviar.
    */
    void* serializar_paquete(t_paquete* paquete, int bytes);

#endif /* SERIALIZACION_H_ */