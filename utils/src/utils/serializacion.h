#ifndef SERIALIZACION_H_
#define SERIALIZACION_H_

    #include <stdio.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
    #include <semaphore.h>
    #include <commons/collections/list.h>
    #include <commons/log.h>
    #include <sys/socket.h>

    typedef enum{
        MENSAJE,
    } cod_inst;

    typedef enum{
        SET,
        READ_MEM,
        WRITE_MEM,
        SUM,
        SUB,
        JNZ,
        LOG,
    } inst_cpu;

    typedef enum{
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
    } inst_syscalls;

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
        uint32_t size;
        uint32_t offset;
        void* stream;
    } t_buffer;

    typedef struct{
        cod_inst codigo_operacion;
        t_buffer* buffer;
    } t_paquete;

    typedef struct{
        uint32_t pid;
        t_list* tids;
        t_list* mutex_asociados;
        int program_counter;
        estado_proceso estado;
    } t_pcb;

    typedef struct{
        uint32_t tid;
        orden_prioridad prioridad;
    } t_tid;

    typedef struct {
        uint8_t valor;
        uint8_t tamanio;
    } t_registro;

    /*-------------------TADs de Buffer para serializar más fácil-------------*/

    /**
    * @fn    buffer_create
    * @brief Crea un buffer vacío de tamaño size y offset 0
    * 
    * @param size          Recibe un uint32_t size que indica el tamaño requerido para el buffer
    */
    t_buffer* buffer_create(uint32_t size);

    /**
    * @fn    buffer_destroy
    * @brief Libera la memoria asociada al buffer
    * 
    * @param buffer          Recibe un buffer para ser liberado
    */
    void buffer_destroy(t_buffer* buffer);

    /**
    * @fn    buffer_add
    * @brief Agrega un stream al buffer en la posición actual y avanza el offset
    * 
    * @param buffer          Buffer a utilizar
    * @param data            Dirección de memoria del dato a agregar al buffer
    * @param size            Tamaño necesario del dato a agregar al buffer
    */
    void buffer_add(t_buffer* buffer, void* data, uint32_t size);

    /**
    * @fn    buffer_read
    * @brief Guarda size bytes del principio del buffer en la dirección data y avanza el offset
    * 
    * @param buffer          Buffer a utilizar
    * @param data            Dirección de memoria donde guardar lo leído del buffer
    * @param size            Tamaño a leer del buffer
    */
    void buffer_read(t_buffer* buffer, void* data, uint32_t size);

    /**
    * @fn    buffer_add_uint32
    * @brief Agrega un uint32_t al buffer
    * 
    * @param buffer          Buffer a utilizar
    * @param data            Número a añadir en el buffer
    */
    void buffer_add_uint32(t_buffer* buffer, uint32_t data);

    /**
    * @fn    buffer_read_uint32
    * @brief Lee un uint32_t del buffer y avanza el offset
    * 
    * @param buffer          Buffer a utilizar
    */
    uint32_t buffer_read_uint32(t_buffer* buffer);

    /**
    * @fn    buffer_add_string
    * @brief Agrega string al buffer con un uint32_t adelante indicando su longitud
    * 
    * @param buffer          Buffer a utilizar
    * @param length          Largo del string a agregar
    * @param data            String a agregar en el buffer
    */
    void buffer_add_string(t_buffer* buffer, uint32_t length, char* string);

    /**
    * @fn    buffer_read_string
    * @brief Lee un string y su longitud del buffer y avanza el offset
    * 
    * @param buffer          Buffer a utilizar
    * @param data            Dirección de memoria donde almacenar el largo del string
    */
    char* buffer_read_string(t_buffer* buffer, uint32_t* length);

    void buffer_add_tlist(t_buffer* buffer, void* data, t_list* list);

    void buffer_add_tids(t_buffer* buffer, t_tid* tid, t_list* list);

    void buffer_add_mutex(t_buffer* buffer, sem_t* mutex, t_list* list);

    t_list* buffer_read_tlist(t_buffer* buffer, void* data, t_list* list, uint32_t* length_lista, uint32_t size_data);

    t_list* buffer_read_tids(t_buffer* buffer);

    t_list* buffer_read_mutex(t_buffer* buffer);

    void* serializar_pcb(t_pcb* data);

    t_pcb* deserializar_pcb(t_buffer* buffer);

    /*-------------------------------------------------------------------------*/

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

    void agregar_a_paquete_pcb(t_paquete* paquete_a_enviar, t_pcb* pcb);

    /**
    * @fn    serializar_paquete
    * @brief Serializa en un stream el paquete a enviar al servidor.
    * 
    * @param paquete          Una instancia t_paquete a ser enviado.
    * @param bytes            Tamaño en bytes que tendrá el stream a enviar.
    */
    void* serializar_paquete(t_paquete* paquete, int bytes);

    void enviar_pcb(t_pcb* pcb_a_enviar, estado_proceso estado_pcb, cod_inst codigo_instruccion, int socket_destino);
    
    t_pcb* recibir_pcb();

#endif /* SERIALIZACION_H_ */