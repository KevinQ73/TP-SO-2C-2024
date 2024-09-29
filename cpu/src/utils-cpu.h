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

    /*------------------------- Funciones de inicio -------------------------*/

    t_cpu levantar_datos(t_config* config);

    t_dictionary* inicializar_registros();

    /*-----------------------------------------------------------------------*/
    /*----------------------- Conexiones con módulos ------------------------*/

    t_pid_tid recibir_paquete_kernel(int socket_kernel, t_log* kernel_log);

    void recibir_paquete_memoria(t_dictionary* registros, int fd_memoria, t_log* cpu_log);

    t_dictionary* solicitar_contexto_ejecucion(t_pid_tid, int fd_memoria, t_log* log_cpu);

    /*-----------------------------------------------------------------------*/
    /*-------------------- Funciones de registro de CPU ---------------------*/

    void actualizar_registros_cpu(t_dictionary* registro_destino, t_dictionary* registro_inicial, t_log* log);

    void modificar_registro(t_dictionary* registro_destino, char* key, int nuevo_valor, t_log* log);

    void program_counter_update(t_dictionary* registro_cpu, t_log* log);

    /*-----------------------------------------------------------------------*/
    /*----------------------- Ciclo de instrucciones ------------------------*/

    char* fetch(t_pid_tid pid_tid_recibido, uint32_t* program_counter, int fd_memoria, t_log* log);

    char** decode(char* instruccion);

    void execute(t_dictionary* registro_a_modificar, char** instruccion_parseada, t_log* log);

    /*-----------------------------------------------------------------------*/
    /*-------------------------- Funciones execute --------------------------*/

    void execute_set(t_dictionary* registro_cpu, char* registro, char* valor, t_log* log);

    void execute_read_mem();

    void execute_write_mem();

    void execute_sum();

    void execute_sub();

    void execute_jnz();

    void execute_log();

    void execute_dump_memory();

    void execute_process_create();

    void execute_thread_create();

    void execute_thread_cancel();

    void execute_mutex_create();

    void execute_mutex_lock();

    void execute_mutex_unlock();

    void execute_thread_exit();

    void execute_process_exit();
    /*-----------------------------------------------------------------------*/

#endif /* UTILS_CPU_H_ */