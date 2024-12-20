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

/*----------------------- FUNCIONES CREATE DE STRUCTS -----------------------*/

t_pcb* create_pcb(char* path_instrucciones, int size_process){
    t_pcb* pcb = malloc(sizeof(t_pcb));

    pcb->pid = siguiente_pid();
    pcb->tid_siguiente = 0;
    pcb->program_counter = 0;
    pcb->path_instrucciones_hilo_main = path_instrucciones;
    pcb->size_process = size_process;
    pcb->mutex_asociados = list_create();
    pcb->lista_tcb = list_create();

    return pcb;
}

t_tcb* create_tcb(t_pcb* pcb_padre, int prioridad){
    t_tcb* tcb = malloc(sizeof(t_tcb));

    tcb->tid = siguiente_tid(pcb_padre->tid_siguiente);
    tcb->prioridad = prioridad;

    pcb_padre->tid_siguiente++;
    return tcb;
}

t_hilo_planificacion* create_hilo_planificacion(t_pcb* pcb_padre, t_tcb* tcb_asociado){
    t_hilo_planificacion* hilo = malloc(sizeof(t_hilo_planificacion));

    hilo->pid_padre = pcb_padre->pid;
    hilo->tid_asociado = tcb_asociado->tid;
    hilo->prioridad = tcb_asociado->prioridad;

    list_add(pcb_padre->lista_tcb, tcb_asociado);

    return hilo;
}

t_cola_prioridades* create_priority_queue(int prioridad){
    t_cola_prioridades* cola_nueva = malloc(sizeof(t_cola_prioridades));

    cola_nueva->cola = list_create();
    cola_nueva->prioridad = prioridad;

    return cola_nueva;
}

int crear_conexion_con_memoria(t_log* kernel_log, char* ip, char* puerto){
    int conexion_memoria = crear_conexion(kernel_log, ip, puerto);
    log_debug(kernel_log, "ME CONECTÉ A MEMORIA");

    return conexion_memoria;
}


t_mutex* create_mutex(char* nombreMutex){
    t_mutex* mutex = malloc(sizeof(t_mutex));

    mutex->nombre = nombreMutex;
    mutex->tid_tomado = -1;
    mutex->cola_bloqueados= list_create();

    return mutex;
}
/*------------------------------ MISCELANEO --------------------------------*/

bool compare_pid(uint32_t* pid_1, uint32_t* pid_2){
    if (pid_1== NULL || pid_2 == NULL) {
        return false;  // Manejar el caso en que alguno de los punteros sea NULL
    }   
    
    return (*pid_1 == *pid_2);  // Comparar los valores apuntados por a y b
    
}

uint32_t siguiente_tid(int siguiente_tid){
    int tid = 0 + siguiente_tid;
    return tid;
}

void multilevel_queue_destroy(t_cola_prioridades* cola){
    list_destroy_and_destroy_elements(cola->cola, free);
    free(cola);
}