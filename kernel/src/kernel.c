#include <kernel.h>

int main(int argc, char* argv[]) {

    //---------------------------- Iniciar archivos ----------------------------

    kernel_log = iniciar_logger("./files/kernel.log", "KERNEL", 1, LOG_LEVEL_DEBUG);

    kernel_config = iniciar_config("./files/kernel.config");

    kernel_registro = levantar_datos(kernel_config);

    // ----------------------- FUNCIONES DE INICIO ------------------------

    inicializar_colas();
    proceso_creados = list_create();

    // --------------------- Conexión como cliente de CPU ----------------------

    fd_conexion_dispatch = crear_conexion(kernel_log, kernel_registro.ip_cpu, kernel_registro.puerto_cpu_dispatch);
    log_debug(kernel_log, "ME CONECTÉ A CPU DISPATCH");

    fd_conexion_interrupt = crear_conexion(kernel_log, kernel_registro.ip_cpu, kernel_registro.puerto_cpu_interrupt);
    log_debug(kernel_log, "ME CONECTÉ A CPU INTERRUPT");

    // --------------------- Conexión como cliente de MEMORIA ----------------------

    conexion_memoria = crear_conexion(kernel_log, kernel_registro.ip_memoria, kernel_registro.puerto_memoria);
    log_debug(kernel_log, "ME CONECTÉ A MEMORIA");

    // --------------------- Inicio del modulo ----------------------

    iniciar_planificacion();

    // --------------------- Finalizacion del modulo----------------------
    
    //eliminar_paquete(paquete_proceso);
    //eliminar_paquete(paquete_fin_proceso);

    log_debug(kernel_log, "TERMINANDO KERNEL");
    eliminar_logger(kernel_log);
    eliminar_listas();
    //LIBERAR COLAS Y ELEMENTOS QUE CONTIENE
}

// ------------------------- CREAR LISTAS -------------------------

void iniciar_planificacion(){
    proceso_ejecutando = 0;

    pthread_create(&hiloNew, NULL, planificador_largo_plazo, NULL);
    pthread_detach(hiloNew);

    pthread_create(&hiloPlanifCortoPlazo, NULL, planificador_corto_plazo, NULL);
    pthread_detach(hiloPlanifCortoPlazo);
}

void inicializar_colas(){
    cola_new = queue_create();
    cola_new_procesos = queue_create();
    cola_ready = queue_create();
    cola_ready_procesos = queue_create();
    cola_blocked = queue_create();
    cola_exit = queue_create();

    cola_prioridad_maxima= queue_create();
    cola_prioridad_1 = queue_create();
    cola_prioridad_2 = queue_create();
    cola_prioridad_3 = queue_create();
    cola_prioridad_4 = queue_create();
    cola_prioridad_5 = queue_create();
    cola_prioridad_6 = queue_create();
    cola_prioridad_7 = queue_create();
}

void eliminar_listas(){

}

// ------------------------- FUNCIONES DE PCB -------------------------

// -------------------- FUNCIONES DE PLANIFICACIÓN --------------------

void* planificador_largo_plazo(){
        
}

void* planificador_corto_plazo(){
    char* planificacion = kernel_registro.algoritmo_planificacion;

    if(strcmp(planificacion, "FIFO")){
        log_debug(kernel_log, "FIFO");
    }else if(strcmp(planificacion, "PRIORIDADES")){
        log_debug(kernel_log, "PRIORIDADES");
    }else if(strcmp(planificacion, "COLAS_MULTINIVEL")){
        log_debug(kernel_log, "COLAS_MULTINIVEL");
    }else{
        log_error(kernel_log,"NO SE RECONOCE LA PLANIFICACION");
    }
}

void* inicializar_pcb_en_espera(){
    t_pcb* pcb;
        
    if(queue_size(cola_new) != 0){
        pcb = queue_pop(cola_new);
        t_paquete* paquete_proceso = crear_paquete(CREAR_PROCESO);

        agregar_a_paquete(paquete_proceso, &pcb->pid, sizeof(uint32_t));

        //ENVIA EL PEDIDO A MEMORIA PARA INICIALIZAR EL PROCESO
        enviar_paquete(paquete_proceso, conexion_memoria);
        log_debug(kernel_log, "SE ENVIO PAQUETE");

        //RECIBO EL PAQUETE CON LA RESPUESTA DE MEMORIA 
        char* respuesta_memoria = recibir_mensaje(conexion_memoria, kernel_log);

        if(strcmp(respuesta_memoria, "ESPACIO_ASIGNADO")){

            //list_add(pcb->pid,); FALTA TID DEL HILO ASOCIADO AL PROCESO
            poner_en_ready_procesos();
                
        }else if(strcmp(respuesta_memoria, "ESPACIO_NO_ASIGNADO")){
            //Ver que hace
            log_debug(kernel_log, "ESPACIO_NO_ASIGNADO");
        }
    }
}

/*No se si es con * o sin en t_hilo_planificacion*/
void* poner_en_ready_segun_prioridad(int prioridad_hilo, t_hilo_planificacion* hilo_del_proceso){ 
    /*PONGO EN READY AL HILO SEGUN SU NIVEL DE PRIORIDAD */
    if(prioridad_hilo == 0){
        queue_push(cola_prioridad_maxima, hilo_del_proceso);
    }else if(prioridad_hilo == 1){
        queue_push(cola_prioridad_1, hilo_del_proceso);
    }else if(prioridad_hilo == 2){
        queue_push(cola_prioridad_2, hilo_del_proceso);
    }else if(prioridad_hilo == 3){
        queue_push(cola_prioridad_3, hilo_del_proceso);
    }else if(prioridad_hilo == 4){
        queue_push(cola_prioridad_4, hilo_del_proceso);
    }else if(prioridad_hilo == 5){
        queue_push(cola_prioridad_5, hilo_del_proceso);
    }else if(prioridad_hilo == 6){
        queue_push(cola_prioridad_6,hilo_del_proceso);
    }else if(prioridad_hilo == 7){
        queue_push(cola_prioridad_7,hilo_del_proceso);
    }else{
        log_error(kernel_log,"NO SE RECONOCE LA PLANIFICACION");
    }
}

void poner_en_ready_procesos(){
    //int size_cola_new = queue_size(cola_new);
    t_pcb* pcb;
    pcb = queue_pop(cola_new_procesos);
    list_add(proceso_creados,pcb);
    pcb->estado = READY_STATE;
    list_add(proceso_creados,pcb);
}

    // --------------------- Creacion de procesos ----------------------

void* peticion_crear_proceso(){
    uint32_t largo_string = 0;
    t_buffer* buffer;
    buffer = buffer_recieve(fd_conexion_dispatch);
    char* path = buffer_read_string(buffer, &largo_string);

    uint32_t tam_proceso = buffer_read_uint32(buffer);

    int prioridad_proceso = buffer_read_uint32(buffer);

    t_pcb* pcb = create_pcb();
    //poner_en_new_procesos(pcb);
    log_debug(kernel_log,"PID: %d- Se crea el proceso- Estado: NEW", pcb->pid);

    t_tcb* tcb= create_tcb();

    tcb->tid = aplicar_tid();
    tcb->prioridad = prioridad_proceso;
    /*DONDE LO LIBERO EL TCB?*/

    t_hilo_planificacion* primer_hilo_planificacion;

    primer_hilo_planificacion->pid = pcb->pid;
    primer_hilo_planificacion->tcb_asociado = tcb;
    primer_hilo_planificacion->estado = NEW_STATE;

    list_add(pcb->tids, primer_hilo_planificacion->tcb_asociado->tid);

    poner_en_new(cola_new, primer_hilo_planificacion);

    // Porqué? -> peticion_iniciar_proceso(pcb, primer_hilo_planificacion);
        
}

void* peticion_iniciar_proceso(t_pcb* pcb, t_hilo_planificacion* primer_hilo_asociado){
    t_paquete* paquete_proceso = crear_paquete(CREAR_PROCESO);

    agregar_a_paquete(paquete_proceso, &pcb->pid, sizeof(uint32_t));

    //ENVIA EL PEDIDO A MEMORIA PARA INICIALIZAR EL PROCESO
    enviar_paquete(paquete_proceso, conexion_memoria);
    log_debug(kernel_log, "SE ENVIO PAQUETE");

    //RECIBO EL PAQUETE CON LA RESPUESTA DE MEMORIA 
    char* respuesta_memoria = recibir_mensaje(conexion_memoria,kernel_log);

    if(respuesta_memoria == "ESPACIO_ASIGNADO"){
        poner_en_ready_procesos(pcb);
        int prioridad = 7;
        poner_en_ready_segun_prioridad(prioridad, primer_hilo_asociado);

    }else if(respuesta_memoria == "ESPACIO_NO_ASIGNADO"){
        pcb= queue_pop(cola_new_procesos);
        poner_en_new_procesos(cola_new_procesos, pcb);
    }
}
        
// --------------------- Finalizacion de procesos ----------------------

void* peticion_finalizar_proceso(t_pcb* pcb){

    t_paquete* paquete_fin_proceso = crear_paquete(FINALIZAR_PROCESO);
    agregar_a_paquete(paquete_fin_proceso, &pcb->pid, sizeof(uint32_t));

    //ENVIA EL PEDIDO A MEMORIA PARA FINALIZAR EL PROCESO
    enviar_paquete(paquete_fin_proceso, conexion_memoria);
    log_debug(kernel_log, "SE ENVIO AVISO DE FINALIZACION DE PROCESO");

    //RECIBO EL PAQUETE CON LA RESPUESTA DE MEMORIA 
    char* respuesta_memoria = recibir_mensaje(conexion_memoria,kernel_log);

    if(respuesta_memoria == "FINALIZACION_ACEPTADA"){
        free(pcb);
        //FALTA LIBERAR HILOS DE ESE PCB??
        log_debug(kernel_log,"Find de proceso:“## Finaliza el proceso <PID> %d", pcb->pid);
        inicializar_pcb_en_espera();

    }else if(respuesta_memoria == "FINALIZACION_RECHAZADA"){
        pcb= queue_pop(cola_new);
        // Porque? -> poner_en_new(cola_new, pcb);
    }
}

// --------------------- Creacion de hilo----------------------
int aplicar_tid(){
    //TODO
}

void* peticion_crear_hilo(){
    uint32_t largo_string = 0;
    t_buffer* buffer;
    buffer = buffer_recieve( fd_conexion_dispatch );
    char* path = buffer_read_string(buffer, &largo_string);

    int prioridad_hilo = buffer_read_uint32(buffer);

    t_paquete* paquete_proceso = crear_paquete(CREAR_HILO);

    /*COMO LE MANDO A MEMORIA EL PATH A EJECUTAR*/
    t_pcb* pcb; // Auxiliar para que compile
    agregar_a_paquete(paquete_proceso, &pcb->pid, sizeof(uint32_t));
       
    //ENVIA EL PEDIDO A MEMORIA PARA INICIALIZAR EL PROCESO
    enviar_paquete(paquete_proceso, conexion_memoria);
    log_debug(kernel_log, "SE ENVIO PAQUETE");
        
    t_tcb* tcb = create_tcb();
    //RECIBO EL PAQUETE CON LA RESPUESTA DE MEMORIA 
    char* respuesta_memoria = recibir_mensaje(conexion_memoria, kernel_log);

    /*TODO la funcion aplicar_tid*/
    tcb->tid = aplicar_tid();
    poner_en_ready_segun_prioridad(tcb->prioridad, tcb);
}

// --------------------- Finalizacion de hilo ----------------------

void* finalizar_hilo(int tid){
    t_paquete* paquete_fin_hilo = crear_paquete(FINALIZAR_HILO);
    agregar_a_paquete(paquete_fin_hilo, &tid, sizeof(uint32_t));

    //ENVIA EL PEDIDO A MEMORIA PARA FINALIZAR EL PROCESO
    enviar_paquete(paquete_fin_hilo, conexion_memoria);
    log_debug(kernel_log, "SE ENVIO AVISO DE FINALIZACION DE PROCESO");

    //RECIBO EL PAQUETE CON LA RESPUESTA DE MEMORIA 
    char* respuesta_memoria = recibir_mensaje(conexion_memoria, kernel_log);

    if(respuesta_memoria == "FINALIZACION_ACEPTADA"){
        //TODO ver como buscar segun que planificacion sea
        /*
        if(planificacion == "FIFO"){

        }else if(planificacion == "PRIORIDADES"){

        }else if(planificacion == "COLAS_MULTINIVEL"){

        }

        }else if(respuesta_memoria == "FINALIZACION_RECHAZADA"){
        */ 
    }
}

// --------------------- Syscalls a atender  ----------------------

void* syscalls_a_atender(){

/*LEER LA OP QUE TIENE EL PAQUETE QUE ENVIA CPU (recibir_op)*/

    int syscall = recibir_operacion(fd_conexion_interrupt);
        
    switch (syscall)
    {
    case PROCESS_CREATE:
        peticion_crear_proceso();
        break;

    case PROCESS_EXIT:
        //peticion_finalizar_proceso();
        break;

    case THREAD_CREATE:
        peticion_crear_hilo();
        break;

    case THREAD_JOIN:
            
        break;

    case THREAD_CANCEL:
            
        break;

    case THREAD_EXIT:
            
        break;
        
    case MUTEX_UNLOCK:
            
        break;

    case DUMP_MEMORY:
            
        break;
        
    default:
        break;
    }
}

    