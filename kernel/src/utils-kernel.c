#include <utils-kernel.h>

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

/*----------------------- FUNCIONES DE INICIALIZACIÓN -----------------------*/

t_pcb* create_pcb(){
    t_pcb* pcb = malloc(sizeof(t_pcb));
        
    pcb->pid = 0; // Crear función incremental
    pcb->tids = list_create();
    pcb->mutex_asociados = list_create();
    pcb->program_counter = 0;
    pcb->estado = NEW_STATE;
    
    return pcb;
}

t_tcb* create_tcb(){
    t_tcb* tcb = malloc(sizeof(t_tcb));
    char* archivo = string_new();

    tcb->tid = 0; // Crear función incremental
    tcb->prioridad = PRORIDAD_7;
    tcb->archivo_asociado = archivo;

    return tcb;
}

void poner_en_new(t_queue* cola_planificador, t_hilo_planificacion* hilo_del_proceso){

    queue_push(cola_planificador, hilo_del_proceso);
}

void poner_en_new_procesos(t_queue* cola_planificador, t_pcb* pcb){

    queue_push(cola_planificador, (void*)pcb);
}

/*-------------- Funcion de comparacion de pids --------------*/
    
bool compare_pid(uint32_t* pid_1, uint32_t* pid_2){
    if (pid_1== NULL || pid_2 == NULL) {
        return false;  // Manejar el caso en que alguno de los punteros sea NULL
    }   
    
    return (*pid_1 == *pid_2);  // Comparar los valores apuntados por a y b
    
}
