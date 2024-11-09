#ifndef MEMORIA_SRC_MEMORIA_H_
#define MEMORIA_SRC_MEMORIA_H_

    #include <utils-memoria.h>

    t_log* memoria_log;
    t_config* memoria_config;
    t_memoria memoria_registro;

    t_bitarray* bitmap_particion_fija;
    t_dictionary* contexto_ejecucion;
    t_list* lista_pseudocodigos;

    char** lista_particiones;

    void* memoria;

    int fd_conexiones;
    int conexion_filesystem;
    int fd_conexion_cpu;
    int fd_conexion_kernel;
    int pid_busqueda;
    int tid_busqueda;
    
    pthread_t hiloMemoriaCpu;
    pthread_t hiloMemoriaKernel;
    pthread_t hilo_atender_kernel;
    pthread_t mutex_fd_filesystem;

    pthread_mutex_t kernel_operando;
    pthread_mutex_t contexto_ejecucion_procesos;
    sem_t memoria_activo;

    /*----------------------- FUNCIONES DE INICIALIZACIÓN -----------------------*/

    void enviar_solicitud_fs();

    void iniciar_memoria();

    void iniciar_semaforos();

    void atender_solicitudes();

    /*---------------------------- FUNCIONES DE KERNEL --------------------------*/

    void* atender_kernel();

    void* atender_solicitudes_kernel(void* fd_conexion);

    bool crear_proceso(uint32_t pid, uint32_t size);

    void crear_hilo(uint32_t pid, uint32_t tid, uint32_t prioridad, char* path);

    void* finalizar_proceso();

    void* finalizar_hilo();

    void* memory_dump();

    /*----------------------------- FUNCIONES DE CPU ----------------------------*/

    void* atender_cpu();

    /*-------------------- FUNCIONES DE CONTEXTO DE EJECUCION -------------------*/

    t_contexto_proceso* crear_contexto_proceso(uint32_t pid, uint32_t base, uint32_t limite);

    t_contexto_hilo* crear_contexto_hilo(uint32_t tid, uint32_t prioridad, char* path);

    t_contexto* buscar_contexto(uint32_t pid, uint32_t tid);

    t_contexto* recibir_contexto(t_buffer* buffer);

    uint32_t hay_particion_disponible(uint32_t pid, uint32_t size, char* esquema);

    uint32_t obtener_espacio_desocupado();

    /*-------------------------------- MISCELANEO -------------------------------*/

    void enviar_contexto_solicitado(t_contexto* contexto);

    void* leer_de_memoria(uint32_t tamanio_lectura, uint32_t inicio_lectura);

    t_contexto_hilo* thread_get_by_tid(t_list* lista_hilos, int tid);

    char** buscar_instruccion(uint32_t pid, uint32_t tid, uint32_t program_counter);

    bool busqueda_pid_tid(t_pseudocodigo* pseudocodigo);

    void enviar_datos_memoria(void* buffer, uint32_t tamanio);

    void actualizar_contexto_ejecucion(t_contexto* contexto_recibido, uint32_t pid, uint32_t tid);

    //void escribir_en_memoria(void* buffer_escritura, uint32_t tamanio_buffer, uint32_t inicio_escritura);

#endif /* MEMORIA_SRC_MEMORIA_H_ */
