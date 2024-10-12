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


/*------------------------------ MISCELANEO --------------------------------*/

bool compare_pid(uint32_t* pid_1, uint32_t* pid_2){
    if (pid_1== NULL || pid_2 == NULL) {
        return false;  // Manejar el caso en que alguno de los punteros sea NULL
    }   
    
    return (*pid_1 == *pid_2);  // Comparar los valores apuntados por a y b
    
}