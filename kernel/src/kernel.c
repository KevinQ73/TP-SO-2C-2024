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
    iniciar_colas();
    iniciar_primer_proceso(argv[1], argv[2]);
    iniciar_planificacion();

    procesos_creados = list_create();
    hilo_exec = list_create();
    hilos_block = list_create();
    lista_colas_multinivel = list_create();
    lista_prioridades = list_create();

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

void iniciar_semaforos(){
    pthread_mutex_init(&mutex_siguiente_id, NULL);
    pthread_mutex_init(&mutex_cola_new, NULL);
    pthread_mutex_init(&mutex_uso_fd_memoria, NULL);
    pthread_mutex_init(&mutex_cola_ready, NULL);
    pthread_mutex_init(&mutex_lista_procesos_ready, NULL);

    sem_init(&contador_procesos_en_new, 0, 0);
}

void iniciar_colas(){
    cola_new = queue_create();
    cola_ready_fifo = queue_create();
    cola_exit = queue_create();
}

void iniciar_primer_proceso(char* path, char* size){
    int size_process = atoi(size);
    int prioridad = PRIORIDAD_MAXIMA;
    char* response_memoria = avisar_creacion_proceso_memoria(path, &size_process, &prioridad, conexion_memoria, kernel_log);

    if (strcmp(response_memoria, "OK"))
    {
        primer_proceso = create_pcb(path, size_process);
        char* respuesta_creacion_hilo = avisar_creacion_hilo_memoria(path, &prioridad, conexion_memoria, kernel_log);

        if (strcmp(respuesta_creacion_hilo, "OK"))
        {
            t_tcb* primer_hilo = create_tcb(PRIORIDAD_MAXIMA);
            t_hilo_planificacion* hilo = create_hilo_planificacion(primer_proceso, primer_hilo);

            list_add(primer_proceso->tids, hilo);
            poner_en_ready(hilo);
            poner_en_ready_procesos(primer_proceso);
            log_debug(kernel_log, "SE CREÓ EL PRIMER PROCESO CON PID: %d, Y PRIMER TID: %d", primer_proceso->pid, primer_hilo->tid);
        } else {
            log_error(kernel_log, "ERROR AL INICIAR PRIMER HILO");
            abort();
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

/*------------------------ FUNCIONES DE PLANIFICACIÓN -----------------------*/

int aplicar_tid(t_pcb* pcb){
    int valor = pcb->tidSig;
    pcb->tidSig++;
    return valor; //crea un tid en base al contador, aumenta dicho contador y devuelve el tid correspondiente
}

void* planificador_largo_plazo(){
    while (1)
    {
        sem_wait(&contador_procesos_en_new);
        inicializar_pcb_en_espera();
    }
}

void* inicializar_pcb_en_espera(){        
    if(queue_size(cola_new) != 0){
        pthread_mutex_lock(&mutex_cola_new);
        t_pcb* pcb = queue_pop(cola_new);
        pthread_mutex_unlock(&mutex_cola_new);

        t_tcb* tcb_asociado = list_remove(pcb->lista_tcbs_new, 0);
        int prioridad = tcb_asociado->prioridad;

        char* respuesta_memoria = avisar_creacion_proceso_memoria(pcb->path_instrucciones_hilo_main, &(pcb->size_process), &prioridad, conexion_memoria, kernel_log);

        if (strcmp(respuesta_memoria, "OK"))
        {
            char* respuesta_memoria_hilo = avisar_creacion_hilo_memoria(pcb->path_instrucciones_hilo_main, &(tcb_asociado->prioridad), conexion_memoria, kernel_log);
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
    int prioridad = tcb_asociado->prioridad;
    char* respuesta_memoria = avisar_creacion_proceso_memoria(pcb->path_instrucciones_hilo_main, &(pcb->size_process), &prioridad, conexion_memoria, kernel_log);

        if (strcmp(respuesta_memoria, "OK"))
        {
            char* respuesta_memoria_hilo = avisar_creacion_hilo_memoria(pcb->path_instrucciones_hilo_main, &prioridad, conexion_memoria, kernel_log);
            t_hilo_planificacion* hilo = create_hilo_planificacion(pcb, tcb_asociado);

            poner_en_ready(hilo);
        } else {
            sem_wait(&aviso_exit_proceso);
            reintentar_inicializar_pcb_en_espera(pcb);
        }
}

void* planificador_corto_plazo(){
    char* planificacion = kernel_registro.algoritmo_planificacion;

    while (1)
    {
        hilo_en_ejecucion = obtener_hilo_segun_algoritmo(planificacion);

        ejecutar_hilo(hilo_en_ejecucion);
    }
}

t_hilo_planificacion* obtener_hilo_segun_algoritmo(char* planificacion){
    t_hilo_planificacion* hilo;

    if (strcmp(planificacion, "FIFO"))
    {
        pthread_mutex_lock(&mutex_cola_ready);
        hilo = (t_hilo_planificacion*)queue_pop(cola_ready_fifo);
        pthread_mutex_unlock(&mutex_cola_ready);
        log_info(kernel_log, "## [FIFO] Se quitó el hilo (%d:%d) de la COLA READY", hilo->pid, hilo->tcb_asociado->tid);

    } else if (strcmp(planificacion, "PRIORIDADES"))
    {
        pthread_mutex_lock(&mutex_cola_ready);
        hilo = thread_find_by_priority_schedule(lista_prioridades);
        pthread_mutex_unlock(&mutex_cola_ready);

    } else if (strcmp(planificacion, "COLAS_MULTINIVEL"))
    {
        pthread_mutex_lock(&mutex_cola_ready);
        hilo = thread_find_by_multilevel_queues_schedule(lista_colas_multinivel);
        pthread_mutex_unlock(&mutex_cola_ready);
    }
    return hilo;
}

void poner_en_new(t_pcb* pcb){
    pthread_mutex_lock(&mutex_cola_new);
    queue_push(cola_new, pcb);
    pthread_mutex_unlock(&mutex_cola_new);
    sem_post(&contador_procesos_en_new);
}

void* poner_en_ready(t_hilo_planificacion* hilo_del_proceso){
    pthread_mutex_lock(&mutex_cola_ready);
    hilo_del_proceso->estado = READY_STATE;
    
    if(strcmp(kernel_registro.algoritmo_planificacion, "FIFO")){
        queue_push(cola_ready_fifo, hilo_del_proceso);

    } else if(strcmp(kernel_registro.algoritmo_planificacion, "PRIORIDADES")){
        list_add(lista_prioridades, hilo_del_proceso);

    }else if(strcmp(kernel_registro.algoritmo_planificacion, "COLAS_MULTINIVEL")){
        queue_push_by_priority(hilo_del_proceso);
    }else{
        log_error(kernel_log,"NO SE RECONOCE LA PLANIFICACION");
        abort();
    }
    pthread_mutex_unlock(&mutex_cola_ready);
}

void poner_en_ready_procesos(t_pcb* pcb){
    pthread_mutex_lock(&mutex_lista_procesos_ready);
    pcb->estado = READY_STATE;
    list_add(procesos_creados, pcb);
    pthread_mutex_unlock(&mutex_lista_procesos_ready);
}

void* poner_en_block(){
    //WAIT
    t_hilo_planificacion* hilo_a_bloquear = list_remove(hilo_exec, 0);
    //SIGNAL
    hilo_a_bloquear->estado = BLOCKED_STATE;

    list_add(hilos_block,hilo_a_bloquear);
   
}
void liberar_hilos_bloqueados(t_hilo_planificacion* hilo){
    for (int i = 0; i < list_size(hilo->lista_hilos_block); i++)
    {
        int get_thread_by_id(t_tcb* tcb){
            return tcb->tid == hilo->tcb_asociado->tid;
        }
        t_hilo_planificacion* hilo_obtenido = list_find(hilo->lista_hilos_block, (void*)get_thread_by_id);
        poner_en_ready(hilo);
    }
}

/*--------------------------- FUNCIONES DE COLAS ----------------------------*/

void queue_push_by_priority(t_hilo_planificacion* hilo_del_proceso){
    if (queue_find_by_priority(lista_colas_multinivel, hilo_del_proceso->tcb_asociado->prioridad))
    {
        t_cola_prioridades* cola_prioridad = queue_get_by_priority(lista_colas_multinivel, hilo_del_proceso->tcb_asociado->prioridad);
        queue_push(cola_prioridad->cola, hilo_del_proceso);
        log_info(kernel_log, "## [COLAS MULTINIVEL] Se agregó el hilo (%d:%d) a la cola de prioridad %d en estado READY", hilo_del_proceso->pid, hilo_del_proceso->tcb_asociado->tid, hilo_del_proceso->tcb_asociado->prioridad);
    } else {
        t_cola_prioridades* cola_prioridad_nueva = create_priority_queue(hilo_del_proceso->tcb_asociado->prioridad);
        queue_push(cola_prioridad_nueva->cola, hilo_del_proceso);
        list_add(lista_colas_multinivel, cola_prioridad_nueva);
        log_info(kernel_log, "## [COLAS MULTINIVEL] Se agregó el hilo (%d:%d) a la NUEVA cola de prioridad %d en estado READY", hilo_del_proceso->pid, hilo_del_proceso->tcb_asociado->tid, hilo_del_proceso->tcb_asociado->prioridad);
    }
}

t_cola_prioridades* queue_get_by_priority(t_list* lista_colas_prioridades, int prioridad){
    bool _queue_contains(void* ptr) {
	    t_cola_prioridades* cola = (t_cola_prioridades*) ptr;
	    return (prioridad == cola->prioridad);
	}
	return list_find(lista_colas_prioridades, _queue_contains);
}

bool queue_find_by_priority(t_list* lista_colas_prioridades, int prioridad){
    bool _queue_contains(void* ptr) {
	    t_cola_prioridades* cola = (t_cola_prioridades*) ptr;
	    return (prioridad == cola->prioridad);
	}
	return list_any_satisfy(lista_colas_prioridades, _queue_contains);
}

t_hilo_planificacion* list_find_by_maximum_priority(t_list* lista_prioridades){
    void* _min_priority(void* a, void* b) {
	    t_hilo_planificacion* hilo_a = (t_hilo_planificacion*) a;
	    t_hilo_planificacion* hilo_b = (t_hilo_planificacion*) b;
	    return hilo_a->tcb_asociado->prioridad <= hilo_b->tcb_asociado->prioridad ? hilo_a : hilo_b;
	}
	t_hilo_planificacion* hilo_encontrado = list_get_minimum(lista_prioridades, _min_priority);
    return hilo_encontrado;
}

t_hilo_planificacion* thread_find_by_priority_schedule(t_list* lista_prioridades){
    t_hilo_planificacion* hilo = list_find_by_maximum_priority(lista_prioridades);
    bool removed = list_remove_element(lista_prioridades, hilo);
    if (removed)
    {
        log_info(kernel_log, "## [PRIORIDADES] Se quitó el hilo (%d:%d) de la COLA READY CON PRIORIDAD %d", hilo->pid, hilo->tcb_asociado->tid, hilo->tcb_asociado->prioridad);
    } else {
        log_debug(kernel_log, "## [PRIORIDADES] ERROR EN thread_find_by_priority_schedule");
        abort();
    }
}

t_hilo_planificacion* thread_find_by_multilevel_queues_schedule(t_list* lista_colas_multinivel){
    t_hilo_planificacion* hilo_a_ejecutar;

    pthread_mutex_lock(&mutex_colas_multinivel_existentes);
    for (int i = 0; i < list_size(lista_colas_multinivel); i++)
    {
        t_cola_prioridades* cola_hallada = queue_get_by_priority(lista_colas_multinivel, i);

        if (queue_is_empty(cola_hallada->cola))
        {
            log_debug(kernel_log, "## [COLAS MULTINIVEL] COLA CON PRIORIDAD: %d VACÍA", cola_hallada->prioridad);
        } else {
            hilo_a_ejecutar = (t_hilo_planificacion*)queue_pop(cola_hallada->cola);
            log_info(kernel_log, "## [COLAS MULTINIVEL] Se quitó el hilo (%d:%d) de la COLA CON PRIORIDAD %d", hilo_a_ejecutar->pid, hilo_a_ejecutar->tcb_asociado->tid, cola_hallada->prioridad);
            i = list_size(lista_colas_multinivel);
        }
    }
    pthread_mutex_lock(&mutex_colas_multinivel_existentes);

    return hilo_a_ejecutar;
}

/*t_cola_prioridades* queue_find_by_minimum_priority(t_list* lista_colas_prioridades){
    bool _queue_min_priority(void* ptr) {
	    t_cola_prioridades* cola_a = (t_cola_prioridades*) a;
	    t_cola_prioridades* cola_b = (t_cola_prioridades*) b;
	    return cola_a->prioridad <= cola_b->prioridad ? cola_a : cola_b;
	}
	return list_any_satisfy(lista_colas_prioridades, _queue_min_priority);
}*/

/*---------------------------- FUNCIONES EXECUTE ----------------------------*/

void* ejecutar_hilo(t_hilo_planificacion* hilo_a_ejecutar){
    //mover_hilo_a_exec(hilo_a_ejecutar);
    t_paquete* paquete = crear_paquete(EJECUTAR_HILO);

    t_buffer* buffer_envio = buffer_create(
        2*(sizeof(uint32_t))
        );

    buffer_add_uint32(buffer_envio, hilo_a_ejecutar->pid, kernel_log);
    buffer_add_uint32(buffer_envio, hilo_a_ejecutar->tcb_asociado->tid, kernel_log);

    paquete->buffer = buffer_envio;

    enviar_paquete(paquete,fd_conexion_dispatch);
    log_debug(kernel_log, "SE ENVIO PAQUETE");
    eliminar_paquete(paquete);

    operacion_a_atender();
}

void* desalojar_hilo(t_hilo_planificacion* hilo_a_desalojar){
    //TODO
    /*
    CASOS POR LOS QUE QUERRIA DESALOJAR:
        PETICION_INSTRUCCION,
        INTERRUPCION_QUANTUM, 
        OTRO CASO PUEDE SER QUE HAYA UN HILO QUE EJECUTE UNA OPERACION QUE LO BLOQUEE Y ESA OPERACION FALLA POR LO QUE HAY QUE MANDAR TODO EL PROCESO A EXIT Y SI HAY ALGUN HILO DE ESE PROCESO
        QUE ESTA EN "EXEC", HAY QUE INTERRMPIRLO"
    */
    

    //enviar_mensaje("DESALOJAR_HILO_POR_QUANTUM",fd_conexion_interrupt);

}

/*----------------------- FUNCIONES KERNEL - MEMORIA ------------------------*/

char* avisar_creacion_proceso_memoria(char* path, int* size_process, int* prioridad, int socket_memoria, t_log* kernel_log){
    int length = strlen(path) + 1;
    t_paquete* paquete = crear_paquete(CREAR_PROCESO);
    t_buffer* buffer = buffer_create(
        length + sizeof(int) + sizeof(int)
    );

    buffer_add_string(buffer, length, path, kernel_log);
    buffer_add_uint32(buffer, size_process, kernel_log);
    buffer_add_uint32(buffer, prioridad, kernel_log);

    paquete->buffer = buffer;

    pthread_mutex_lock(&mutex_uso_fd_memoria);
    enviar_paquete(paquete, socket_memoria);
    char* response_memoria = recibir_mensaje(socket_memoria, kernel_log);
    pthread_mutex_unlock(&mutex_uso_fd_memoria);

    return response_memoria;
}

char* avisar_creacion_hilo_memoria(char* path, int* prioridad, int socket_memoria, t_log* kernel_log){
    int length = strlen(path) + 1;
    t_paquete* paquete = crear_paquete(CREAR_HILO);
    t_buffer* buffer = buffer_create(
        length + sizeof(int)
    );

    buffer_add_string(buffer, length, path, kernel_log);
    buffer_add_uint32(buffer, (uint32_t*)prioridad, kernel_log);

    paquete->buffer = buffer;

    pthread_mutex_lock(&mutex_uso_fd_memoria);
    enviar_paquete(paquete, socket_memoria);
    char* response_memoria = recibir_mensaje(socket_memoria, kernel_log);
    pthread_mutex_unlock(&mutex_uso_fd_memoria);

    return response_memoria;
}

char* avisar_fin_proceso_memoria(uint32_t pid, int socket_memoria){
    t_paquete* paquete = crear_paquete(FINALIZAR_PROCESO);
    t_buffer* buffer = buffer_create(
        sizeof(int)
    );

    buffer_add_uint32(buffer, (uint32_t*)pid, kernel_log);

    paquete->buffer = buffer;

    pthread_mutex_lock(&mutex_uso_fd_memoria);
    enviar_paquete(paquete, socket_memoria);
    char* response_memoria = recibir_mensaje(socket_memoria, kernel_log);
    pthread_mutex_unlock(&mutex_uso_fd_memoria);

    return response_memoria;
}

char* avisar_fin_hilo_memoria(uint32_t pid, uint32_t tid, int socket_memoria){
    t_paquete* paquete = crear_paquete(FINALIZAR_HILO);
    t_buffer* buffer = buffer_create(
        2*sizeof(int)
    );

    buffer_add_uint32(buffer, (uint32_t*)pid, kernel_log);
    buffer_add_uint32(buffer, (uint32_t*)tid, kernel_log);

    paquete->buffer = buffer;

    pthread_mutex_lock(&mutex_uso_fd_memoria);
    enviar_paquete(paquete, socket_memoria);
    char* response_memoria = recibir_mensaje(socket_memoria, kernel_log);
    pthread_mutex_unlock(&mutex_uso_fd_memoria);

    buffer_destroy(buffer);
    eliminar_paquete(paquete);

    return response_memoria;
}
/*--------------------------------- SYSCALLS --------------------------------*/

void* operacion_a_atender(){
    int operacion = recibir_operacion(fd_conexion_interrupt);
    t_pid_tid pid_tid_recibido = recibir_pid_tid(fd_conexion_interrupt, kernel_log);
   
    switch (operacion)
    {
    case PROCESS_CREATE:
        
        syscall_process_create(pid_tid_recibido.pid, pid_tid_recibido.tid);
        break;

    case PROCESS_EXIT:
        syscall_process_exit();
        break;

    case THREAD_CREATE:
        syscall_thread_create(pid_tid_recibido.pid, pid_tid_recibido.tid);
        break;

    case THREAD_JOIN:
        syscall_thread_join();
        break;

    case THREAD_CANCEL:
        syscall_thread_cancel(pid_tid_recibido.tid);
        break;

    case THREAD_EXIT:
        syscall_thread_exit(hilo_en_ejecucion);   
            
        break;

    case MUTEX_CREATE:
            
        break;

    case MUTEX_LOCK:
            
        break;
        
    case MUTEX_UNLOCK:
            
        break;

    case DUMP_MEMORY:
            syscall_dump_memory();
        break;
        
    default:
        break;
    }
}

void* syscall_process_create(uint32_t pid_solicitante, uint32_t tid_solicitante){
    uint32_t largo_string = 0;
    t_buffer* buffer;
    buffer = buffer_recieve(fd_conexion_dispatch);

    char* path = buffer_read_string(buffer, &largo_string);
    uint32_t tam_proceso = buffer_read_uint32(buffer);
    int prioridad_proceso = buffer_read_uint32(buffer);

    log_info(kernel_log,"(%d:%d) - Solicitó syscall: PROCESS CREATE", pid_solicitante, tid_solicitante);

    t_pcb* pcb = create_pcb(path, tam_proceso);
    t_tcb* tcb_asociado = create_tcb(prioridad_proceso);
    list_add(pcb->lista_tcbs_new, tcb_asociado);

    poner_en_new(pcb);
    log_info(kernel_log,"## PID: %d- Se crea el proceso- Estado: NEW", pcb->pid);
}   

/*void* syscall_process_exit(t_pcb* pcb){
    log_info(kernel_log,"(%d:%d) - Solicitó syscall: PROCESS_EXIT", hilo_en_ejecucion->pid, hilo_en_ejecucion->tcb_asociado->tid);
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
}*/

// Función de comparación con valor objetivo como parámetro
bool es_igual(void* elemento, void* valor_objetivo) {
    int* valor_elemento = (int*)elemento;
    int* valor_a_buscar = (int*)valor_objetivo;
    return *valor_elemento == *valor_a_buscar;
}


void* syscall_dump_memory(){
    log_info(kernel_log,"(%d:%d) - Solicitó syscall: DUMP_MEMORY", hilo_en_ejecucion->pid, hilo_en_ejecucion->tcb_asociado->tid);
    t_paquete* paquete_aviso_dump_memory = crear_paquete(AVISO_DUMP_MEMORY);
    agregar_a_paquete(paquete_aviso_dump_memory,  hilo_en_ejecucion->pid, sizeof(uint32_t));
    agregar_a_paquete(paquete_aviso_dump_memory,  hilo_en_ejecucion->tcb_asociado->tid, sizeof(uint32_t));

    //ENVIA EL PEDIDO A MEMORIA PARA FINALIZAR EL PROCESO
    enviar_paquete(paquete_aviso_dump_memory, conexion_memoria);
    log_debug(kernel_log, "SE ENVIO AVISO DE DUMP_MEMORY");

    int tid_hilo_en_ejecucion =  hilo_en_ejecucion->tcb_asociado->tid;

    /*Esta syscall bloqueará al hilo que la invocó*/

    poner_en_block(); 
    
    /*hasta que el módulo memoria confirme la finalización de la operación*/

    char* respuesta_memoria = recibir_mensaje(conexion_memoria,kernel_log);

    if(strcmp(respuesta_memoria, "OK")){

        /*Caso contrario, el hilo se desbloquea normalmente pasando a READY.*/

        t_hilo_planificacion* hilo_a_desbloquear = list_remove_by_condition(hilos_block, es_igual);
        poner_en_ready(hilo_a_desbloquear);

    }else{
        /*en caso de error, el proceso se enviará a EXIT. */

        t_pcb* pcb_a_liberar =  list_find_by_pid(hilo_en_ejecucion->pid);
        free(pcb_a_liberar);
        
        log_debug(kernel_log,"Find de proceso:“## Finaliza el proceso <PID> %d", pcb_a_liberar->pid);
        inicializar_pcb_en_espera();

    }

 }

void* syscall_process_exit(){
    log_info(kernel_log,"(%d:%d) - Solicitó syscall: PROCESS_EXIT", hilo_en_ejecucion->pid, hilo_en_ejecucion->tcb_asociado->tid);
    t_paquete* paquete_fin_proceso = crear_paquete(FINALIZAR_PROCESO);
    agregar_a_paquete(paquete_fin_proceso,  hilo_en_ejecucion->pid, sizeof(uint32_t));

    //ENVIA EL PEDIDO A MEMORIA PARA FINALIZAR EL PROCESO
    enviar_paquete(paquete_fin_proceso, conexion_memoria);
    log_debug(kernel_log, "SE ENVIO AVISO DE FINALIZACION DE PROCESO");

    //RECIBO EL PAQUETE CON LA RESPUESTA DE MEMORIA 
    char* respuesta_memoria = recibir_mensaje(conexion_memoria,kernel_log);

    if(strcmp(respuesta_memoria, "FINALIZACION_ACEPTADA")){
       t_pcb* pcb_a_liberar =  list_find_by_pid(hilo_en_ejecucion->pid);
        free(pcb_a_liberar);
        
        log_debug(kernel_log,"Find de proceso:“## Finaliza el proceso <PID> %d", pcb_a_liberar->pid);
        inicializar_pcb_en_espera();
    }
}

void* syscall_thread_create(uint32_t pid_solicitante, uint32_t tid_solicitante){
    log_info(kernel_log,"(%d:%d) - Solicitó syscall: THREAD_CREATE", pid_solicitante, tid_solicitante);
    uint32_t largo_string = 0;
    t_buffer* buffer;
    buffer = buffer_recieve(fd_conexion_dispatch);
    
    char* path = buffer_read_string(buffer, &largo_string);
    int prioridad_hilo = buffer_read_uint32(buffer);
    
    uint32_t length = strlen(path) + 1;

    t_paquete* paquete = crear_paquete(CREAR_HILO);
    t_buffer* buffer_envio = buffer_create(
        length
        );
    buffer_add_string(buffer_envio, length, path, kernel_log);

    paquete->buffer = buffer_envio;

    enviar_paquete(paquete,conexion_memoria);
    log_debug(kernel_log, "SE ENVIO PAQUETE");
    eliminar_paquete(paquete);

    t_pcb* pcb = obtener_pcb_from_hilo(hilo_en_ejecucion);
    t_tcb* tcb = create_tcb(prioridad_hilo);

    t_hilo_planificacion* hilo_a_crear = create_hilo_planificacion(pcb, tcb);
    
    char* respuesta_memoria = avisar_creacion_hilo_memoria(path, &prioridad_hilo, conexion_memoria, kernel_log);

    if (strcmp(respuesta_memoria, "OK"))
    {
        poner_en_ready(hilo_a_crear);
    } else {
        log_debug(kernel_log, "Rompimos algo con peticion crear hilo");
    }
}

void* syscall_thread_join(){
    uint32_t largo_string = 0;
    t_buffer* buffer;
    buffer = buffer_recieve(fd_conexion_dispatch);
    uint32_t tid = buffer_read_uint32(buffer);
    t_hilo_planificacion* hilo_a_bloquear = list_get(hilo_exec,0);
    list_add(hilo_a_bloquear->lista_hilos_block, tid);

    hilo_a_bloquear->estado = BLOCKED_STATE;
    
    poner_en_block();
    buffer_destroy(buffer);
}

void* syscall_thread_exit(t_hilo_planificacion* hilo){
    log_info(kernel_log,"##(%d:%d) - Solicitó syscall: THREAD_EXIT", hilo->pid, hilo->tcb_asociado->tid);
    
    char* respuesta_memoria = avisar_fin_hilo_memoria(hilo->pid, hilo->tcb_asociado->tid, conexion_memoria);
    
    if (strcmp(respuesta_memoria, "OK"))
    {
        log_debug(kernel_log, "SE ENVIO AVISO DE FINALIZACION DE HILO PID: %d TID: %d", hilo->pid, hilo->tcb_asociado->tid);
        liberar_hilos_bloqueados(hilo);
        t_hilo_planificacion_destroy(hilo);
        free(respuesta_memoria);
    } else {
        log_debug(kernel_log, "Rompimos algo con finalizar hilo");
    }
}

t_pcb* list_find_by_pid(uint32_t pid){
    pthread_mutex_lock(&mutex_lista_procesos_ready);
    bool _list_contains(void* ptr) {
	    t_pcb* pcb_a_encontrar = (t_pcb*) ptr;
	    return (pcb_a_encontrar == pid);
	}
	return list_find(procesos_creados, _list_contains);
    pthread_mutex_lock(&mutex_lista_procesos_ready);
}

void* tid_find(t_list* lista_tids, uint32_t tid){
    bool _list_contains(void* ptr){
        t_tcb* tcb_a_encontrar = (t_tcb*) ptr;
	    return (tcb_a_encontrar->tid == tid);
	}
	return list_find(lista_tids, _list_contains);
}

void* syscall_thread_cancel(uint32_t tid){
    t_pcb* pcb = list_find_by_pid(hilo_en_ejecucion->pid);

    t_hilo_planificacion* resultado = tid_find(pcb->tids, tid);

    if (resultado != NULL)
    {
        liberar_hilos_bloqueados(resultado);
        t_hilo_planificacion_destroy(resultado);
    }
    
}

/*--------------------------- FINALIZACIÓN DE TADS --------------------------*/

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
    free(pcb);
}

/*------------------------- FINALIZACIÓN DEL MODULO -------------------------*/

void eliminar_listas(){
    /*list_clean_and_destroy_elements(hilo_exec,hilo_destroy);
    list_clean_and_destroy_elements(procesos_creados,pcb_destroy);
    list_clean_and_destroy_elements(hilos_block,hilo_destroy);*/
}

void eliminar_colas(){
    
    queue_destroy_and_destroy_elements(cola_new,t_hilo_planificacion_destroy);
    queue_destroy_and_destroy_elements(cola_exit,t_hilo_planificacion_destroy);
    queue_destroy_and_destroy_elements(cola_ready_fifo,t_hilo_planificacion_destroy);
}

/*------------------------------- MISCELANEO --------------------------------*/

t_pcb* obtener_pcb_from_hilo(t_hilo_planificacion* hilo){

}
    