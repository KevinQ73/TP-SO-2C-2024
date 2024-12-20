#ifndef MEMORIA_SRC_MEMORIA_H_
#define MEMORIA_SRC_MEMORIA_H_

    #include <utils-memoria.h>

    t_log* memoria_log;
    t_config* memoria_config;
    t_memoria memoria_registro;
    t_list* lista_pseudocodigos;

    t_dictionary* contextos_de_ejecucion; // Se guarda una t_list de contextos de ejecución de los hilos de un proceso. 
                                          // La key es el PID del proceso.

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

    pthread_mutex_t kernel_operando;
    pthread_mutex_t contexto_ejecucion_procesos;
    pthread_mutex_t mutex_fd_filesystem;
    pthread_mutex_t mutex_huecos_disponibles;
    pthread_mutex_t mutex_procesos_activos;

    sem_t memoria_activo;
    sem_t actualizar_contexto;

    // Particiones fijas

    char** lista_particiones; // Vienen del config
    t_bitarray* bitmap_particion_fija; // Bitmap de particiones, verifica si una partición se usa o no.

    typedef struct{
        int base;
        int limite;
    }t_particion;

    t_list* particiones_memoria;
    int ultima_asignacion = -1; //Para usarla en NEXT_FIST
    // Particiones dinámicas

    t_dictionary* lista_procesos_activos;
    t_list* lista_huecos_disponibles;

    /*----------------------- FUNCIONES DE INICIALIZACIÓN -----------------------*/

    void iniciar_memoria();

    void iniciar_semaforos();

    void atender_solicitudes();

    /*---------------------------- FUNCIONES DE KERNEL --------------------------*/

    void* atender_kernel();

    void* atender_solicitudes_kernel(void* fd_conexion);

    bool crear_proceso(uint32_t pid, uint32_t size);

    void crear_hilo(uint32_t pid, uint32_t tid, uint32_t prioridad, char* path);

    void* finalizar_proceso(uint32_t pid);

    void* finalizar_hilo(uint32_t pid, uint32_t tid);

    void* memory_dump();

    /*----------------------------- FUNCIONES DE CPU ----------------------------*/

    void* atender_cpu();

    /*-------------------- FUNCIONES DE CONTEXTO DE EJECUCION -------------------*/

    t_contexto_proceso* crear_contexto_proceso(uint32_t pid, uint32_t base, uint32_t limite);

    t_contexto_hilo* crear_contexto_hilo(uint32_t tid, uint32_t prioridad, char* path);

    t_contexto* buscar_contexto(uint32_t pid, uint32_t tid);

    t_contexto_proceso* buscar_contexto_proceso(uint32_t pid);

    t_contexto_hilo* buscar_contexto_hilo(t_contexto_proceso* contexto_proceso, uint32_t tid);

    t_contexto* recibir_contexto(t_buffer* buffer);

    //t_contexto* remover_contexto_hilo(uint32_t pid, uint32_t tid);

    int hay_particion_disponible(uint32_t pid, uint32_t size, char* esquema);

    uint32_t obtener_espacio_desocupado();

    t_contexto_hilo* thread_get_by_tid(t_list* lista_hilos, int tid);

    t_contexto_hilo* thread_remove_by_tid(t_list* lista_hilos, int tid);

    void process_remove_and_destroy_by_pid(int tid);

    t_contexto_proceso* process_remove_by_pid(int pid);

    void thread_context_destroy(t_contexto_hilo* contexto_hilo);

    void process_context_destroy(t_contexto_proceso* contexto_proceso);

    /*-------------------------------- MISCELANEO -------------------------------*/

    void enviar_contexto_solicitado(t_contexto* contexto);

    void* leer_de_memoria(uint32_t tamanio_lectura, uint32_t inicio_lectura);

    char** buscar_instruccion(uint32_t pid, uint32_t tid, uint32_t program_counter);

    bool busqueda_pid_tid(t_pseudocodigo* pseudocodigo);

    void enviar_datos_memoria(void* buffer, uint32_t tamanio);

    void actualizar_contexto_ejecucion(t_contexto* contexto_recibido, uint32_t pid, uint32_t tid);

    int get_size_partition(uint32_t base);

    /*--------------------------- PARTICIONES FIJAS --------------------------*/
    int buscar_peor_bloque(int tamanio);
    int buscar_mejor_bloque(int tamanio);
    int buscar_siguiente_bloque(int tamanio);
    int buscar_primer_bloque(int tamanio);
    int asignacion_fija(int pid,int tamanio);

    /*--------------------------- PARTICIONES DINAMICAS --------------------------*/

    int particion_dinamica(uint32_t pid, uint32_t size);

    t_hueco* crear_hueco(uint32_t inicio, uint32_t size);

    void agregar_hueco(t_hueco* hueco);

    t_hueco* first_fit(uint32_t size);

    t_hueco* best_fit(uint32_t size);

    t_hueco* worst_fit(uint32_t size);

    t_hueco* obtener_hueco(uint32_t size);

    void liberar_espacio_en_memoria(uint32_t pid);

    void liberar_hueco_bitmap_fijas(uint32_t base);

    void liberar_hueco_dinamico(t_contexto_proceso* proceso);

    void consolidar_huecos(uint32_t inicio, uint32_t size);

    bool byte_en_hueco(uint32_t byte);

    t_hueco* remover_hueco_que_contiene_byte(uint32_t byte);

    void escribir_en_memoria(void* buffer_escritura, uint32_t tamanio_buffer, uint32_t inicio_escritura);

// WORKING ZONE

void imprimir_estado_huecos();

void imprimir_estado_procesos_activos();

t_proceso* get_process(uint32_t pid);

int crear_conexion_con_filesystem(t_log* memoria_log, char* ip, char* puerto);

#endif /* MEMORIA_SRC_MEMORIA_H_ */
