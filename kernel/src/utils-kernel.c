#include <utils-kernel.h>

/*----------------------- FUNCIONES DE INICIALIZACIÓN -----------------------*/

t_kernel levantar_datos(t_config* config){

    t_kernel datos_config;

    datos_config.ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    datos_config.puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    datos_config.ip_cpu = config_get_string_value(config, "IP_CPU");
    datos_config.puerto_cpu_dispatch = config_get_string_value(config,"PUERTO_CPU_DISPATCH");
    datos_config.puerto_cpu_interrupt = config_get_string_value(config,"PUERTO_CPU_INTERRUPT");
    datos_config.algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    datos_config.quantum = config_get_int_value(config, "QUANTUM");
    datos_config.log_level = config_get_string_value(config, "LOG_LEVEL");
    
    return datos_config;
}

void iniciar_semaforos(){
    pthread_mutex_init(&mutex_siguiente_id, NULL);
    pthread_mutex_init(&mutex_cola_new, NULL);
    pthread_mutex_init(&mutex_uso_fd_memoria, NULL);
    pthread_mutex_init(&mutex_cola_ready, NULL);
    pthread_mutex_init(&mutex_lista_procesos_ready, NULL);

    sem_init(&contador_procesos_en_new, 0, 0);
}

/*----------------------- FUNCIONES CREATE DE STRUCTS -----------------------*/

t_pcb* create_pcb(char* path_instrucciones, int size_process){
    t_pcb* pcb = malloc(sizeof(t_pcb));
        
    pcb->pid = 0; // Crear función incremental
    pcb->tidSig = 0;
    pcb->tids = list_create();
    pcb->mutex_asociados = list_create();
    pcb->program_counter = 0;
    pcb->estado = NEW_STATE;
    pcb->path_instrucciones_hilo_main = path_instrucciones;
    pcb->size_process = size_process;
    
    return pcb;
}

t_tcb* create_tcb(t_pcb* pcb, int prioridad){
    t_tcb* tcb = malloc(sizeof(t_tcb));

    tcb->tid = aplicar_tid(pcb); // Crear función incremental
    tcb->prioridad = prioridad;

    list_add(pcb->tids, tcb);
    return tcb;
}

t_hilo_planificacion* create_hilo_planificacion(t_pcb* pcb_asociado, t_tcb* tcb_asociado){
    t_hilo_planificacion* hilo = malloc(sizeof(t_hilo_planificacion));

    hilo->estado = pcb_asociado->estado;
    hilo->pid = pcb_asociado->pid;
    hilo->tcb_asociado = tcb_asociado;
    hilo->lista_hilos_block = list_create();

    return hilo;
}

/*----------------------- FUNCIONES DE PLANIFICACIÓN ------------------------*/

void poner_en_new(t_pcb* pcb){
    pthread_mutex_lock(&mutex_cola_new);
    queue_push(cola_new, pcb);
    pthread_mutex_unlock(&mutex_cola_new);
    sem_post(&contador_procesos_en_new);
}

/*----------------------- FUNCIONES KERNEL - MEMORIA ------------------------*/

char* avisar_creacion_proceso_memoria(char* path, int size_process, int prioridad, t_log* kernel_log){
    uint32_t largo_string = 0;
    t_buffer* buffer;
    int length = strlen(path) + 1;
    t_paquete* paquete = crear_paquete(CREAR_PROCESO);
    t_buffer* buffer = buffer_create(
        length + sizeof(int) + sizeof(int)
    );

    buffer_add_string(buffer, length, path, kernel_log);
    buffer_add_uint32(buffer, (uint32_t)size_process, kernel_log);
    buffer_add_uint32(buffer, (uint32_t)prioridad, kernel_log);

    paquete->buffer = buffer;

    pthread_mutex_lock(&mutex_uso_fd_memoria);
    enviar_paquete(paquete, conexion_memoria);
    char* response_memoria = recibir_mensaje(conexion_memoria, kernel_log);
    pthread_mutex_unlock(&mutex_uso_fd_memoria);

    return response_memoria;
}

char* avisar_creacion_hilo_memoria(char* path, int prioridad, t_log* kernel_log){
    uint32_t largo_string = 0;
    t_buffer* buffer;
    int length = strlen(path) + 1;
    t_paquete* paquete = crear_paquete(CREAR_HILO);
    t_buffer* buffer = buffer_create(
        length + sizeof(int)
    );

    buffer_add_string(buffer, length, path, kernel_log);
    buffer_add_uint32(buffer, (uint32_t)prioridad, kernel_log);

    paquete->buffer = buffer;

    pthread_mutex_lock(&mutex_uso_fd_memoria);
    enviar_paquete(paquete, conexion_memoria);
    char* response_memoria = recibir_mensaje(conexion_memoria, kernel_log);
    pthread_mutex_unlock(&mutex_uso_fd_memoria);

    return response_memoria;
}

/*------------------------------ MISCELANEO --------------------------------*/

bool compare_pid(uint32_t* pid_1, uint32_t* pid_2){
    if (pid_1== NULL || pid_2 == NULL) {
        return false;  // Manejar el caso en que alguno de los punteros sea NULL
    }   
    
    return (*pid_1 == *pid_2);  // Comparar los valores apuntados por a y b
    
}