#ifndef UTILS_CPU_H_
#define UTILS_CPU_H_

    #include <pthread.h>
    #include <stdlib.h>
    #include <stdio.h>
    #include <utils/serializacion.h>
    #include <utils/cliente.h>
    #include <utils/files.h>
    #include <utils/servidor.h>
    #include <commons/string.h>

    typedef struct{
        char* ip_memoria;
        char* puerto_memoria;
        char* puerto_cpu_dispatch;
        char* puerto_cpu_interrupt;
        char* log_level;
    } t_cpu;

    t_cpu levantar_datos(t_config* config);

    t_registro* make_registers();

    t_dictionary* inicializar_registros();

    void recibir_paquete_kernel(int socket_kernel, t_log* kernel_log);
    
    void fetch(t_pcb* pcb_recibido);

#endif /* UTILS_CPU_H_ */