#ifndef MEMORIA_SRC_MEMORIA_H_
#define MEMORIA_SRC_MEMORIA_H_

    #include <utils-memoria.h>

    t_log* memoria_log;
    t_config* memoria_config;
    t_memoria memoria_registro;

    t_dictionary* contexto_ejecucion;

    char** lista_particiones;

    int fd_conexiones;
    int conexion_filesystem;
    int fd_conexion_cpu;
    int fd_conexion_kernel;
    
    pthread_t hiloMemoriaCpu;
    void atender_cpu();
    void enviar_contexto_solicitado(t_dictionary* registro_solicitado);
    char* obtenerInstruccion(char* pathInstrucciones, uint32_t program_counter);

    pthread_t hiloCrearProceso;
    pthread_t hiloFinalizarProceso;
    void atender_kernel();
    void* crear_proceso();
    void* finalizar_proceso();

#endif /* MEMORIA_SRC_MEMORIA_H_ */
