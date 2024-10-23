#ifndef MEMORIA_SRC_MEMORIA_H_
#define MEMORIA_SRC_MEMORIA_H_

    #include <utils-memoria.h>

    t_log* memoria_log;
    t_config* memoria_config;
    t_memoria memoria_registro;

    t_dictionary* contexto_ejecucion;

    char** lista_particiones;

    void* memoria;

    int fd_conexiones;
    int conexion_filesystem;
    int fd_conexion_cpu;
    int fd_conexion_kernel;
    
    pthread_t hiloMemoriaCpu;
    t_list* lista_pseudocodigos;

    int pid_busqueda;
    int tid_busqueda;

    void atender_cpu();
    void enviar_contexto_solicitado(t_dictionary* registro_solicitado);
    char* buscar_path(int pid, int tid);
    char* obtenerInstruccion(char* pathInstrucciones, uint32_t program_counter);
    bool busqueda_pid_tid(t_pseudocodigo* pseudocodigo);
    void* leer_de_memoria(uint32_t tamanio_lectura, uint32_t inicio_lectura);
    void enviar_datos_memoria(void* buffer, uint32_t tamanio);
    void escribir_en_memoria(void* buffer_escritura, uint32_t tamanio_buffer, uint32_t inicio_escritura);

    pthread_t hiloCrearProceso;
    pthread_t hiloFinalizarProceso;
    void iniciar_memoria();
    void atender_kernel();
    void* crear_proceso();
    void* finalizar_proceso();

#endif /* MEMORIA_SRC_MEMORIA_H_ */
