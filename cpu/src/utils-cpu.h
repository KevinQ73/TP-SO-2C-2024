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

    typedef enum{
        BASE,
        LIMITE,
        PC,
        AX,
        BX,
        CX,
        DX,
        EX,
        FX,
        GX,
        HX,
    } t_registro_handler;

    /*------------------------- Funciones de inicio -------------------------*/

    t_cpu levantar_datos(t_config* config);

    t_dictionary* inicializar_registros();

    /*-----------------------------------------------------------------------*/
    /*----------------------- Conexiones con módulos ------------------------*/

    t_pid_tid recibir_paquete_kernel(int socket_kernel, t_log* kernel_log);

    void enviar_paquete_kernel(t_buffer* buffer, int socket_memoria, cod_inst codigo_instruccion);

    void recibir_contexto_memoria(t_contexto* registros_cpu, int fd_memoria, t_log* cpu_log);

    void enviar_registros_memoria(t_contexto* registro_cpu, t_pid_tid pid_tid_recibido, int conexion_memoria, t_log* log);

    void solicitar_contexto_ejecucion(t_contexto* registros_cpu, t_pid_tid pid_tid, int fd_memoria, t_log* log_cpu);

    /*-----------------------------------------------------------------------*/
    /*-------------------- Funciones de registro de CPU ---------------------*/

    t_registro_handler get_register_id(char* registro);

    void actualizar_registros_cpu(t_contexto* registros_cpu, t_buffer* registro_destino, t_log* log);

    void modificar_registro(t_contexto* registro_destino, char* registro, int nuevo_valor, t_log* log);

    void program_counter_update(t_contexto* registro_cpu, t_log* log);

    /*-----------------------------------------------------------------------*/
    /*----------------------- Ciclo de instrucciones ------------------------*/

    char* fetch(t_pid_tid pid_tid_recibido, uint32_t* program_counter, int fd_memoria, t_log* log);

    char** decode(char* instruccion);

    /*-----------------------------------------------------------------------*/


#endif /* UTILS_CPU_H_ */