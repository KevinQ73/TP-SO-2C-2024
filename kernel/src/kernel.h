#ifndef KERNEL_H_
#define KERNEL_H_

    #include <utils-kernel.h>
    #include <utils/serializacion.h>
    t_log* kernel_log;
    t_config* kernel_config;
    t_kernel kernel_registro;

    int conexion_cpu;
    int conexion_memoria;
    int fd_conexion_dispatch;
    int fd_conexion_interrupt;
    int proceso_ejecutando;

	char* ip_cpu;
	char* ip_memoria;

    char* puerto_memoria;
	char* puerto_cpu_dispatch;
    char* puerto_cpu_interrupt;

    t_queue* cola_new;
    t_queue* cola_new_procesos;
    t_queue* cola_ready;
    t_queue* cola_ready_procesos;
    t_queue* cola_blocked;
    t_queue* cola_prioridad_maxima;
    t_queue* cola_prioridad_1;
    t_queue* cola_prioridad_2;
    t_queue* cola_prioridad_3;
    t_queue* cola_prioridad_4;
    t_queue* cola_prioridad_5;
    t_queue* cola_prioridad_6;
    t_queue* cola_prioridad_7;



    pthread_t hiloNew;
    pthread_t hiloPlanifCortoPlazo;
    // ----------------------- FUNCIONES DE INICIO ------------------------
    /**
    * @fn    levantar_datos
    * @brief Levanta los datos desde el config a el registro propio del módulo.
    * 
    * @return        Retorna una instancia de registro t_kernel
    */
    
    void inicializar_colas();

    // -------------------- FUNCIONES DE PLANIFICACIÓN --------------------

    void iniciar_planificacion();
    void* planificador_corto_plazo();

    void poner_en_new(t_pcb* pcb);

    void poner_en_ready();


    // --------------------- Creacion de procesos ----------------------
    void* peticion_crear_proceso(char* path);
#endif /* KERNEL_H_ */