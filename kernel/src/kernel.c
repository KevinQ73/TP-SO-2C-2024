#include <kernel.h>

int main(int argc, char* argv[]) {

    //---------------------------- Iniciar archivos ----------------------------

    kernel_log = iniciar_logger("./files/kernel.log", "KERNEL", 1, LOG_LEVEL_DEBUG);

    kernel_config = iniciar_config("./files/kernel.config");

    kernel_registro = levantar_datos(kernel_config);

    // --------------------- Conexión como cliente de CPU ----------------------

    fd_conexion_dispatch = crear_conexion(kernel_log, kernel_registro.ip_cpu, kernel_registro.puerto_cpu_dispatch);
    log_debug(kernel_log, "ME CONECTÉ A CPU DISPATCH");

    fd_conexion_interrupt = crear_conexion(kernel_log, kernel_registro.ip_cpu, kernel_registro.puerto_cpu_interrupt);
    log_debug(kernel_log, "ME CONECTÉ A CPU INTERRUPT");

    // --------------------- Conexión como cliente de MEMORIA ----------------------

    conexion_memoria = crear_conexion(kernel_log, kernel_registro.ip_memoria, kernel_registro.puerto_memoria);
    log_debug(kernel_log, "ME CONECTÉ A MEMORIA");

    // --------------------- Inicio del modulo ----------------------

    iniciar_semaforos();
    iniciar_primer_proceso();
    iniciar_colas();
    iniciar_planificacion();

    proceso_creados = list_create();
    hilo_exec = list_create();
    hilos_block = list_create();

    // --------------------- Finalizacion del modulo----------------------
    
    //eliminar_paquete(paquete_proceso);
    //eliminar_paquete(paquete_fin_proceso);

    log_debug(kernel_log, "TERMINANDO KERNEL");
    eliminar_logger(kernel_log);
    eliminar_listas();
    eliminar_colas();
    //LIBERAR COLAS Y ELEMENTOS QUE CONTIENE
}

/*----------------------- FUNCIONES DE INICIALIZACIÓN -----------------------*/

void iniciar_primer_proceso(){
    int size_process = atoi(argv[1]);

    char* response_memoria = avisar_creacion_proceso_memoria(argv[0], size_process, PRIORIDAD_MAXIMA, kernel_log);

    if (strcmp(response_memoria, "OK"))
    {
        primer_proceso = create_pcb(argv[0], size_process);
        char* respuesta_creacion_hilo = avisar_creacion_hilo_memoria(argv[0], PRIORIDAD_MAXIMA, kernel_log);

        if (strcmp(respuesta_creacion_hilo, "OK"))
        {
            t_tcb* primer_hilo = create_tcb(PRIORIDAD_MAXIMA);
            list_add(primer_proceso->tids, primer_hilo->tid);

            t_hilo_planificacion* hilo = create_hilo_planificacion(primer_proceso, primer_hilo);
            poner_en_ready(hilo);
            poner_en_ready_procesos(primer_proceso);
            log_debug(kernel_log, "SE CREÓ EL PRIMER PROCESO CON PID: %d, Y PRIMER TID: %d", primer_proceso->pid, primer_hilo->tid);
        } else {
            log_error(kernel_log, "ERROR AL INICIAR PRIMER HILO");
        }
    } else {
        log_error(kernel_log, "ERROR AL INICIAR PRIMER PROCESO");
    }
}

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

// -------------------- FUNCIONES DE PLANIFICACIÓN --------------------

void* planificador_largo_plazo(){
    while (1)
    {
        sem_wait(&contador_procesos_en_new);
        inicializar_pcb_en_espera();
    }
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
    if(queue_size(cola_new) != 0){
        pthread_mutex_lock(&mutex_cola_new);
        t_pcb* pcb = queue_pop(cola_new);
        pthread_mutex_unlock(&mutex_cola_new);

        t_tcb* tcb_asociado = list_get(pcb->tids, 0);

        char* respuesta_memoria = avisar_creacion_proceso_memoria(pcb->path_instrucciones_hilo_main, pcb->size_process, tcb_asociado->prioridad, kernel_log);

        if (strcmp(respuesta_memoria, "OK"))
        {
            char* respuesta_memoria_hilo = avisar_creacion_hilo_memoria(pcb->path_instrucciones_hilo_main, tcb_asociado->prioridad, kernel_log);
            t_hilo_planificacion* hilo = create_hilo_planificacion(pcb, tcb_asociado);

            poner_en_ready(hilo);
        } else {
            sem_wait(&aviso_exit_proceso);
            reintentar_inicializar_pcb_en_espera(pcb);
        }
    } else {
        log_debug(kernel_log, "NO HAY PCB EN COLA NEW");
    }
}

void reintentar_inicializar_pcb_en_espera(t_pcb* pcb){
    t_tcb* tcb_asociado = list_get(pcb->tids, 0);
    char* respuesta_memoria = avisar_creacion_proceso_memoria(pcb->path_instrucciones_hilo_main, pcb->size_process, tcb_asociado->prioridad, kernel_log);

        if (strcmp(respuesta_memoria, "OK"))
        {
            char* respuesta_memoria_hilo = avisar_creacion_hilo_memoria(pcb->path_instrucciones_hilo_main, tcb_asociado->prioridad, kernel_log);
            t_hilo_planificacion* hilo = create_hilo_planificacion(pcb, tcb_asociado);

            poner_en_ready(hilo);
        } else {
            sem_wait(&aviso_exit_proceso);
            reintentar_inicializar_pcb_en_espera(pcb);
        }
}

void* poner_en_ready(t_hilo_planificacion* hilo_del_proceso){
    pthread_mutex_lock(&mutex_cola_ready);
    hilo_del_proceso->estado = READY_STATE;
    
    if(strcmp(kernel_registro.algoritmo_planificacion, "FIFO")){
        queue_push(cola_prioridad_maxima, hilo_del_proceso);

    } else if(strcmp(kernel_registro.algoritmo_planificacion, "PRIORIDADES")){
    
        if(hilo_del_proceso->tcb_asociado->prioridad == 0){
            queue_push(cola_prioridad_maxima, hilo_del_proceso);
            
        }else if(hilo_del_proceso->tcb_asociado->prioridad  == 1){
            queue_push(cola_prioridad_1, hilo_del_proceso);
        }else if(hilo_del_proceso->tcb_asociado->prioridad  == 2){
            queue_push(cola_prioridad_2, hilo_del_proceso);
        }else if(hilo_del_proceso->tcb_asociado->prioridad  == 3){
            queue_push(cola_prioridad_3, hilo_del_proceso);
        }else if(hilo_del_proceso->tcb_asociado->prioridad  == 4){
            queue_push(cola_prioridad_4, hilo_del_proceso);
        }else if(hilo_del_proceso->tcb_asociado->prioridad  == 5){
            queue_push(cola_prioridad_5, hilo_del_proceso);
        }else if(hilo_del_proceso->tcb_asociado->prioridad  == 6){
            queue_push(cola_prioridad_6,hilo_del_proceso);
        }else if(hilo_del_proceso->tcb_asociado->prioridad  == 7){
            queue_push(cola_prioridad_7,hilo_del_proceso);
        }else{
            log_error(kernel_log,"NO SE RECONOCE LA prioridad");
        }

    }else if(strcmp(kernel_registro.algoritmo_planificacion, "COLAS_MULTINIVEL")){
        if(hilo_del_proceso->tcb_asociado->prioridad  == 0){
            queue_push(cola_prioridad_maxima, hilo_del_proceso);
        }else if(hilo_del_proceso->tcb_asociado->prioridad  == 1){
            queue_push(cola_prioridad_1, hilo_del_proceso);
        }else if(hilo_del_proceso->tcb_asociado->prioridad  == 2){
            queue_push(cola_prioridad_2, hilo_del_proceso);
        }else if(hilo_del_proceso->tcb_asociado->prioridad  == 3){
            queue_push(cola_prioridad_3, hilo_del_proceso);
        }else if(hilo_del_proceso->tcb_asociado->prioridad  == 4){
            queue_push(cola_prioridad_4, hilo_del_proceso);
        }else if(hilo_del_proceso->tcb_asociado->prioridad  == 5){
            queue_push(cola_prioridad_5, hilo_del_proceso);
        }else if(hilo_del_proceso->tcb_asociado->prioridad  == 6){
            queue_push(cola_prioridad_6,hilo_del_proceso);
        }else if(hilo_del_proceso->tcb_asociado->prioridad  == 7){
            queue_push(cola_prioridad_7,hilo_del_proceso);
        }else{
            log_error(kernel_log,"NO SE RECONOCE LA prioridad");
        }
    }else{
        log_error(kernel_log,"NO SE RECONOCE LA PLANIFICACION");
    }
    pthread_mutex_unlock(&mutex_cola_ready);
}

void poner_en_ready_procesos(t_pcb* pcb){
    pthread_mutex_lock(&mutex_lista_procesos_ready);
    pcb->estado = READY_STATE;
    list_add(proceso_creados, pcb);
    pthread_mutex_unlock(&mutex_lista_procesos_ready);
}

// --------------------- Creacion de procesos ----------------------

void* syscall_process_create(uint32_t pid_solicitante, uint32_t tid_solicitante){
    uint32_t largo_string = 0;
    t_buffer* buffer;
    buffer = buffer_recieve(fd_conexion_dispatch);

    char* path = buffer_read_string(buffer, &largo_string);
    uint32_t tam_proceso = buffer_read_uint32(buffer);
    int prioridad_proceso = buffer_read_uint32(buffer);

    log_info(kernel_log,"(%d:%d) - Solicitó syscall: PROCESS CREATE", pid_solicitante, tid_solicitante);

    t_pcb* pcb = create_pcb(path, tam_proceso);
    t_tcb* tcb_asociado = create_tcb(pcb, prioridad_proceso);

    poner_en_new(pcb);
    log_info(kernel_log,"## PID: %d- Se crea el proceso- Estado: NEW", pcb->pid);
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
        
        log_debug(kernel_log,"Find de proceso:“## Finaliza el proceso <PID> %d", pcb->pid);
        inicializar_pcb_en_espera();

    }
}

// --------------------- Creacion de hilo----------------------
int aplicar_tid(t_pcb* pcb){
    valor = pcb->tidSig
    pcb->tidSig++
    return valor //crea un tid en base al contador, aumenta dicho contador y devuelve el tid correspondiente
}

void* peticion_crear_hilo(){
    uint32_t largo_string = 0;
    t_buffer* buffer;
    buffer = buffer_recieve(fd_conexion_dispatch);
    
    char* path = buffer_read_string(buffer, &largo_string);
    int prioridad_hilo = buffer_read_uint32(buffer);
    
    unit32_t length = strlen(path) + 1;

    t_paquete* paquete = crear_paquete(CREAR_HILO);
    t_buffer* buffer_envio = buffer_create(
        length
        );
    buffer_add_string(buffer_envio, length, path, kernel_log);

    paquete->buffer = buffer_envio;

    enviar_paquete(paquete,conexion_memoria);
    log_debug(kernel_log, "SE ENVIO PAQUETE");
    eliminar_paquete(paquete);
    
    t_tcb* tcb = create_tcb(prioridad_hilo);
    t_hilo_planificacion* hilo_ejecutando = list_get(hilo_exec,0);
    t_hilo_planificacion* hilo_a_crear;
    
    hilo_a_crear->pid = hilo_ejecutando->pid;
    hilo_a_crear->tcb_asociado = tcb;
    hilo_a_crear->estado = NEW_STATE;
    hilo_a_crear->lista_hilos_block=[];

    //RECIBO EL PAQUETE CON LA RESPUESTA DE MEMORIA 
    char* respuesta_memoria = recibir_mensaje(conexion_memoria, kernel_log);

    if (strcmp(respuesta_memoria, "OK"))
    {
        poner_en_ready(hilo_a_crear);
    } else {
        log_debug(kernel_log, "Rompimos algo con peticion crear hilo");
    }
}

// --------------------- Finalizacion de hilo ----------------------

void* finalizar_hilo(t_hilo_planificacion* hilo){
    t_paquete* paquete_fin_hilo = crear_paquete(FINALIZAR_HILO);

    t_buffer* buffer = buffer_create(
        2* sizeof(uint32_t);
    );
    buffer_add_uint32(buffer, hilo->pid);
    buffer_add_uint32(buffer, hilo->tcb_asociado->tid);

    paquete_fin_hilo->buffer = buffer;
    //ENVIA EL PEDIDO A MEMORIA PARA FINALIZAR EL PROCESO
    enviar_paquete(paquete_fin_hilo, conexion_memoria);
    log_debug(kernel_log, "SE ENVIO AVISO DE FINALIZACION DE HILO PID: %d TID: %d", hilo->pid, hilo->tcb_asociado->tid);

    eliminar_paquete(paquete_fin_hilo);
    //RECIBO EL PAQUETE CON LA RESPUESTA DE MEMORIA 
    char* respuesta_memoria = recibir_mensaje(conexion_memoria, kernel_log);

    if (strcmp(respuesta_memoria, "OK"))
    {
        liberar_hilos_bloqueados(hilo);
        t_hilo_planificacion_destroy(hilo);
        free(respuesta_memoria);
        buffer_destroy(buffer);
    } else {
        log_debug(kernel_log, "Rompimos algo con finalizar hilo");
    }
}

void liberar_hilos_bloqueados(t_hilo_planificacion* hilo){
    for (int i = 0; i < list_size(hilo->lista_hilos_block); i++)
    {
        int get_thread_by_id(t_tcb* tcb){
            return tcb->tid == hilo->tcb_asociado->tid;
        }
        t_hilo_planificacion* hilo_obtenido = (hilo->lista_hilos_block, void*get_thread_by_id);
        poner_en_ready(hilo_obtenido->tcb_asociado->prioridad, hilo);
    }
}


//-----------------------------FUNCION EJECUCION DE HILOS----------------------------------
    void* ejecutar_hilo(t_hilo_planificacion* hilo_a_ejecutar){
        
        mover_hilo_a_exec(hilo_a_ejecutar);

        t_paquete* paquete = crear_paquete(EJECUTAR_HILO);

        t_buffer* buffer_envio = buffer_create(
            2*(sizeof(unit_32));
            );

        buffer_add_uint32(buffer_envio, hilo_a_ejecutar->pid, kernel_log);
        buffer_add_uint32(buffer_envio, hilo_a_ejecutar->tcb_asociado->tid, kernel_log);

        paquete->buffer = buffer_envio;

        enviar_paquete(paquete,fd_conexion_dispatch);
        log_debug(kernel_log, "SE ENVIO PAQUETE");
        eliminar_paquete(paquete);

        //SEMAFORO QUE ESPERE HASTA QUE CPU DEVUELVA EL TID
        esperar_tid_cpu();

    }

    void* esperar_tid_cpu(void){
            //TODO
    }

    void* desalojar_hilo(t_hilo_planificacion* hilo_a_desalojar){
        /*
        PETICION_INSTRUCCION,
        CONTEXTO_EJECUCION,
        INTERRUPCION_QUANTUM,
        INTERRUPCION_USUARIO,
        */
        //TODO
    }


//-----------------------------THREAD_JOIN----------------------------------

void* mover_a_block(){
    //WAIT
    t_hilo_planificacion* hilo_a_bloquear = list_remove(hilo_exec, 0);
    //SIGNAL
    list_add(hilos_block,hilo_a_bloquear);

}

void* syscall_thread_join(){
    uint32_t largo_string = 0;
    t_buffer* buffer;
    buffer = buffer_recieve(fd_conexion_dispatch);
    uint32_t tid = buffer_read_uint32(buffer);
    t_hilo_planificacion* hilo_a_bloquear = list_get(lista_exec,0);
    list_add(hilo_a_bloquear->lista_hilos_block, tid);

    hilo_a_bloquear->estado = BLOCKED_STATE;
    
    
    mover_a_block();
    buffer_destroy(buffer);
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
        syscall_thread_join();
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



//---------------------------FUNCIONES DE FINALIZACION--------------------------------------

void t_hilo_planificacion_destroy(t_hilo_planificacion* hilo) {
    free(hilo->tcb_asociado);
    list_destroy(hilo->lista_hilos_block);
    free(hilo);
}

void pcb_destroy(t_pcb* pcb){
   
    free(pcb->tids);
    list_destroy(pcb->tids);
    free(pcb->mutex_asociados);
    free(pcb->program_counter);
    free(pcb->estado);
    free(pcb);
}

void eliminar_listas(){
    proceso_creados = list_create();
    hilo_exec = list_create();
    hilos_block = list_create();
    list_clean_and_destroy_elements(hilo_exec,hilo_destroy);
    list_clean_and_destroy_elements(procesos_creados,pcb_destroy);
    list_clean_and_destroy_elements(hilos_block,hilo_destroy);

}

void eliminar_colas(){
    
    queue_destroy_and_destroy_elements(cola_new,t_hilo_planificacion_destroy);
    queue_destroy_and_destroy_elements(cola_ready,t_hilo_planificacion_destroy);
    queue_destroy_and_destroy_elements(cola_blocked,t_hilo_planificacion_destroy);
    queue_destroy_and_destroy_elements(cola_exit,t_hilo_planificacion_destroy);
    queue_destroy_and_destroy_elements(cola_prioridad_maxima,t_hilo_planificacion_destroy);
    queue_destroy_and_destroy_elements(cola_prioridad_1,t_hilo_planificacion_destroy);
    queue_destroy_and_destroy_elements(cola_prioridad_2,t_hilo_planificacion_destroy);
    queue_destroy_and_destroy_elements(cola_prioridad_3,t_hilo_planificacion_destroy);
    queue_destroy_and_destroy_elements(cola_prioridad_4,t_hilo_planificacion_destroy);
    queue_destroy_and_destroy_elements(cola_prioridad_5,t_hilo_planificacion_destroy);
    queue_destroy_and_destroy_elements(cola_prioridad_6,t_hilo_planificacion_destroy);
    queue_destroy_and_destroy_elements(cola_prioridad_7,t_hilo_planificacion_destroy);


    queue_destroy_and_destroy_elements(cola_new_procesos,pcb_destroy);
    queue_destroy_and_destroy_elements(cola_ready_procesos,pcb_destroy);

}
    