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

    /*-------------------------------- TADS ---------------------------------*/

    typedef enum{
        MENSAJE,
        PAQUETE,
        PID_TID,
        DESCONEXION,
        CREAR_PROCESO,
        FINALIZAR_PROCESO,
        PETICION_INSTRUCCION,
        CONTEXTO_EJECUCION,
        INTERRUPCION_QUANTUM,
        INTERRUPCION_USUARIO,
        ENVIO_TID,
    } cod_inst;

    typedef enum{
        SET,
        READ_MEM,
        WRITE_MEM,
        SUM,
        SUB,
        JNZ,
        LOG,
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
        ERROR_CODE,
    } inst_cpu;

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
        uint32_t program_counter;
        estado_proceso estado;
    } t_pcb;

    typedef struct{
        uint32_t tid;
        orden_prioridad prioridad;
    } t_tcb;

    typedef struct{
        uint32_t pid;
        uint32_t tid;
    } t_pid_tid;

    typedef struct{
        uint32_t pid;
        t_tcb* tcb_asociado;
    } t_hilo_planificacion;
    
    extern char* nombres_registros[11];

    /*-----------------------------------------------------------------------*/

    /*--------------------------- TADs de Buffer ----------------------------*/

    /**
    * @fn    buffer_create
    * @brief Crea un buffer vacío de tamaño size y offset 0
    * 
    * @param size          Recibe un uint32_t size que indica el tamaño requerido para el buffer
    * @return              Una instancia t_buffer* de tamaño size
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
    * @fn    buffer_size
    * @brief Indica el tamaño que tiene el payload del buffer
    * 
    * @param buffer          Recibe una instancia t_buffer*
    * @return                Un entero uint32_t que indica el tamaño del payload
    */
    uint32_t buffer_size(t_buffer* buffer);

    /**
    * @fn    buffer_add
    * @brief Agrega un stream al buffer en la posición actual y avanza el offset
    * 
    * @param buffer          Instancia t_buffer* a utilizar
    * @param data            Dirección de memoria del dato a agregar al buffer
    * @param size            Tamaño necesario del dato a agregar al buffer
    */
    void buffer_add(t_buffer* buffer, void* data, uint32_t size);

    /**
    * @fn    buffer_read
    * @brief Guarda size bytes del principio del buffer en la dirección data y avanza el offset
    * 
    * @param buffer          Instancia t_buffer* a utilizar
    * @param data            Dirección de memoria donde guardar lo leído del buffer
    * @param size            Tamaño a leer del buffer
    */
    void buffer_read(t_buffer* buffer, void* data, uint32_t size);

    /**
    * @fn    buffer_add_uint32
    * @brief Agrega un uint32_t al buffer
    * 
    * @param buffer          Instancia t_buffer* a utilizar
    * @param data            Número a añadir en el buffer
    * @param log             **PARA DEBUGGEAR** Una instancia t_log* del módulo que utilice la función
    */
    void buffer_add_uint32(t_buffer* buffer, uint32_t* data, t_log* log);

    /**
    * @fn    buffer_add_string
    * @brief Agrega string al buffer con un uint32_t adelante indicando su longitud
    * 
    * @param buffer          Instancia t_buffer* a utilizar
    * @param length          Largo del string a agregar
    * @param data            String a agregar en el buffer
    * @param log             **PARA DEBUGGEAR** Una instancia t_log* del módulo que utilice la función
    */
    void buffer_add_string(t_buffer* buffer, uint32_t length, char* string, t_log* log);

    /**
    * @fn    buffer_read_uint32
    * @brief Lee un uint32_t del buffer y avanza el offset
    * 
    * @param buffer          Instancia t_buffer* a utilizar
    * @return                Un entero uint32_t leído del buffer.
    */
    uint32_t buffer_read_uint32(t_buffer* buffer);

    /**
    * @fn    buffer_read_string
    * @brief Lee un string y su longitud del buffer y avanza el offset
    * 
    * @param buffer          Buffer a utilizar
    * @param length          Dirección de memoria donde almacenar el largo del string
    */
    char* buffer_read_string(t_buffer* buffer, uint32_t* length);

        /**
    * @fn    buffer_recieve
    * @brief Obtiene el buffer enviado desde el cliente.
    * 
    * @param buffer             Tamaño en bytes del buffer
    * @param socket_cliente   Socket del cliente que envió el paquete con el buffer dentro.
    * @return                 Stream de tipo void* con los datos del buffer.
    */
    t_buffer* buffer_recieve(int socket_cliente);

    /*
    void buffer_add_tlist(t_buffer* buffer, t_list* list, t_log* log);

    void buffer_add_tcbs(t_buffer* buffer, t_list* list, t_log* log);

    void buffer_add_mutex(t_buffer* buffer, t_list* list, t_log* log);

    void buffer_add_tids(t_buffer* buffer, t_list* list, t_log* log);
    
    t_list* buffer_read_tlist(t_buffer* buffer, t_list* list);

    t_list* buffer_read_tcbs(t_buffer* buffer);

    t_list* buffer_read_mutex(t_buffer* buffer);

    t_list* buffer_read_tids(t_buffer* buffer);
    */

    /*-------------------------------------------------------------------------*/

    /*--------------------------- TADs de Paquete ---------------------------*/

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

    /*
    void agregar_a_paquete_pcb(t_paquete* paquete_a_enviar, t_pcb* pcb, t_log* log);
    */

    /*-----------------------------------------------------------------------*/

    /*-------------- Funciones de envio de datos entre módulos --------------*/

    /**
    * @fn    recibir_operacion
    * @brief Recibe el socket del cliente y obtiene el código de operación.
    * 
    * @param socket_cliente   Socket del cual obtiene el código de operación.
    * @return                 Un Integer con el código de operación.
    */
    int recibir_operacion(int socket_cliente);

    /**
    * @fn    enviar_mensaje
    * @brief Envía un mensaje desde el cliente al servidor
    * 
    * @param mensaje          Mensaje a ser enviado.
    * @param socket_cliente   Socket del cliente para realizar la conexión.
    */
    void enviar_mensaje(char* mensaje, int socket_server, t_log* log);

    char* recibir_mensaje(int socket_cliente, t_log* log);

    void enviar_pid_tid(uint32_t* pid, uint32_t* tid, int socket_servidor, t_log* log);

    t_pid_tid recibir_pid_tid(int socket_cliente, t_log* log);

    cod_inst obtener_codigo_instruccion(char* operacion);
    
    /*
    void* serializar_pcb(t_pcb* data, t_log* log);

    t_pcb* deserializar_pcb(t_buffer* buffer);

    void enviar_pcb(t_pcb* pcb_a_enviar, estado_proceso estado_pcb, cod_inst codigo_instruccion, int socket_destino, t_log* log);
    
    t_pcb* recibir_pcb(int socket_cliente);
    */


#endif /* SERIALIZACION_H_ */