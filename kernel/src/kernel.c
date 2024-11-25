#include <kernel.h>

int main(int argc, char* argv[]) {

    //---------------------------- Iniciar archivos ----------------------------

    kernel_log = iniciar_logger("./files/kernel.log", "KERNEL", 1, LOG_LEVEL_DEBUG);

    kernel_config = iniciar_config(argv[3]);

    kernel_registro = levantar_datos(kernel_config);

    // --------------------- Conexión como cliente de CPU ----------------------

    fd_conexion_dispatch = crear_conexion(kernel_log, kernel_registro.ip_cpu, kernel_registro.puerto_cpu_dispatch);
    log_debug(kernel_log, "ME CONECTÉ A CPU DISPATCH");

    fd_conexion_interrupt = crear_conexion(kernel_log, kernel_registro.ip_cpu, kernel_registro.puerto_cpu_interrupt);
    log_debug(kernel_log, "ME CONECTÉ A CPU INTERRUPT");

    // --------------------- Inicio del modulo ----------------------

    procesos_creados = list_create();
    hilos_block = list_create();
    lista_colas_multinivel = list_create();
    lista_prioridades = list_create();

    signal(SIGTERM, signal_handler);  // Capturar la señal SIGTERM
    signal(SIGINT, signal_handler);   // Capturar la señal SIGINT (Ctrl+C)

    iniciar_semaforos();
    iniciar_colas();
    iniciar_primer_proceso(argv[1], argv[2]);
    iniciar_planificacion();

    sem_wait(&kernel_activo);

    // --------------------- Finalizacion del modulo----------------------

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
    sem_init(&kernel_activo, 0, 0);
}

void iniciar_colas(){
    cola_new = queue_create();
    cola_ready_fifo = queue_create();
    cola_exit = queue_create();
}

void iniciar_primer_proceso(char* path, char* size){
    int size_process = atoi(size);
    int prioridad = 0;

    primer_proceso = create_pcb(path, size_process);
    char* response_memoria = avisar_creacion_proceso_memoria(&(primer_proceso->pid), &size_process, kernel_log);

    if (strcmp(response_memoria, "OK") == 0)
    {   
        log_info(kernel_log, "## (<PID>:%d) Se crea el proceso - Estado: NEW", primer_proceso->pid);
        t_tcb* primer_hilo = create_tcb(primer_proceso, 0);
        char* respuesta_creacion_hilo = avisar_creacion_hilo_memoria(&primer_proceso->pid, &primer_hilo->tid, path, &prioridad, kernel_log);

        if (strcmp(respuesta_creacion_hilo, "OK") == 0)
        {
            t_hilo_planificacion* hilo = create_hilo_planificacion(primer_proceso, primer_hilo);

            poner_en_ready(hilo);
            log_info(kernel_log, "## (<%d>:<%d>) Se crea el Hilo - Estado: READY", primer_proceso->pid, primer_hilo->tid);
            poner_en_ready_procesos(primer_proceso);
        } else {
            log_error(kernel_log, "ERROR AL INICIAR PRIMER HILO");
            abort();
        }
    } else {
        log_error(kernel_log, "ERROR AL INICIAR PRIMER PROCESO");
        abort();
    }
}

void iniciar_planificacion(){
    pthread_create(&hiloPlanifCortoPlazo, NULL, (void*)planificador_corto_plazo, NULL);
    pthread_detach(hiloPlanifCortoPlazo);

    pthread_create(&hiloNew, NULL, planificador_largo_plazo, NULL);
    pthread_detach(hiloNew);
}

/*------------------------ FUNCIONES DE PLANIFICACIÓN -----------------------*/

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

        char* respuesta_memoria = avisar_creacion_proceso_memoria(&(pcb->pid), &pcb->size_process, kernel_log);

        if (strcmp(respuesta_memoria, "OK"))
        {
            t_tcb* tcb_main = list_get(pcb->lista_tcb, 0);
            char* respuesta_memoria_hilo = avisar_creacion_hilo_memoria(&pcb->pid, &tcb_main->tid, pcb->path_instrucciones_hilo_main, &(tcb_main->prioridad), kernel_log);
            if (strcmp(respuesta_memoria_hilo, "OK"))
            {
                t_hilo_planificacion* hilo = create_hilo_planificacion(pcb, tcb_main);
                poner_en_ready(hilo);
            } else {
                log_debug(kernel_log, "NO SE PUDO INICIALIZAR HILO EN MEMORIA");
            }
        } else {
            sem_wait(&aviso_exit_proceso);
            reintentar_inicializar_pcb_en_espera(pcb);
        }
    } else {
        log_debug(kernel_log, "NO HAY PCB EN COLA NEW");
    }
}

void reintentar_inicializar_pcb_en_espera(t_pcb* pcb){
    t_tcb* tcb_asociado = list_get(pcb->lista_tcb, 0);
    int prioridad = tcb_asociado->prioridad;
    char* respuesta_memoria = avisar_creacion_proceso_memoria(&(pcb->size_process), &prioridad, kernel_log);

        if (strcmp(respuesta_memoria, "OK"))
        {
            char* respuesta_memoria_hilo = avisar_creacion_hilo_memoria(&pcb->pid, &tcb_asociado->tid, pcb->path_instrucciones_hilo_main, &prioridad, kernel_log);
            if (strcmp(respuesta_memoria_hilo, "OK"))
            {
                t_hilo_planificacion* hilo = create_hilo_planificacion(pcb, tcb_asociado);
                poner_en_ready(hilo);
            } else {
                log_debug(kernel_log, "NO SE PUDO REINTENTAR INICIALIZAR HILO EN MEMORIA");
            }
        } else {
            sem_wait(&aviso_exit_proceso);
            reintentar_inicializar_pcb_en_espera(pcb);
        }
}

void* planificador_corto_plazo(){
    char* planificacion = kernel_registro.algoritmo_planificacion;
    hilo_en_ejecucion = malloc(sizeof(t_hilo_planificacion));
    while (1)
    {
        hilo_en_ejecucion = obtener_hilo_segun_algoritmo(planificacion);

        ejecutar_hilo(hilo_en_ejecucion);
    }
}

t_hilo_planificacion* obtener_hilo_segun_algoritmo(char* planificacion){
    t_hilo_planificacion* hilo;

    if (strcmp(planificacion, "FIFO") == 0)
    {
        pthread_mutex_lock(&mutex_cola_ready);
        hilo = (t_hilo_planificacion*)queue_pop(cola_ready_fifo);
        pthread_mutex_unlock(&mutex_cola_ready);
        log_info(kernel_log, "## [FIFO] Se quitó el hilo (%d:%d) de la COLA READY", hilo->pcb_padre->pid, hilo->tcb_asociado->tid);

    } else if (strcmp(planificacion, "PRIORIDADES") == 0)
    {
        pthread_mutex_lock(&mutex_cola_ready);
        hilo = thread_find_by_priority_schedule(lista_prioridades);
        pthread_mutex_unlock(&mutex_cola_ready);

    } else if (strcmp(planificacion, "CMP") == 0)
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
    
    if(strcmp(kernel_registro.algoritmo_planificacion, "FIFO") == 0){
        queue_push(cola_ready_fifo, hilo_del_proceso);

    } else if(strcmp(kernel_registro.algoritmo_planificacion, "PRIORIDADES") == 0){
        list_add(lista_prioridades, hilo_del_proceso);

    }else if(strcmp(kernel_registro.algoritmo_planificacion, "CMP") == 0){
        queue_push_by_priority(hilo_del_proceso);
    }else{
        log_error(kernel_log,"NO SE RECONOCE LA PLANIFICACION");
        abort();
    }
    pthread_mutex_unlock(&mutex_cola_ready);
}

void poner_en_ready_procesos(t_pcb* pcb){
    pthread_mutex_lock(&mutex_lista_procesos_ready);
    pcb->estado_proceso = READY_STATE;
    pthread_mutex_unlock(&mutex_lista_procesos_ready);
}

void* poner_en_block(){
    /*//WAIT
    t_hilo_planificacion* hilo_a_bloquear = list_remove(hilo_exec, 0);
    //SIGNAL
    hilo_a_bloquear->estado = BLOCKED_STATE;

    list_add(hilos_block,hilo_a_bloquear);*/
}

void liberar_hilos_bloqueados(t_hilo_planificacion* hilo){
    for (int i = 0; i < list_size(hilo->lista_hilos_block); i++)
    {
        int get_thread_by_id(t_tcb* tcb){
            return tcb->tid == hilo->tcb_asociado->tid;
        }
        t_hilo_planificacion* hilo_obtenido = list_find(hilo->lista_hilos_block, (void*)get_thread_by_id);
        poner_en_ready(hilo_obtenido);
    }
}

/*--------------------------- FUNCIONES DE COLAS ----------------------------*/

void queue_push_by_priority(t_hilo_planificacion* hilo_del_proceso){
    if (queue_find_by_priority(lista_colas_multinivel, hilo_del_proceso->tcb_asociado->prioridad))
    {
        t_cola_prioridades* cola_prioridad = queue_get_by_priority(lista_colas_multinivel, hilo_del_proceso->tcb_asociado->prioridad);
        queue_push(cola_prioridad->cola, hilo_del_proceso);
        log_info(kernel_log, "## [COLAS MULTINIVEL] Se agregó el hilo (%d:%d) a la cola de prioridad %d en estado READY", hilo_del_proceso->pcb_padre->pid, hilo_del_proceso->tcb_asociado->tid, hilo_del_proceso->tcb_asociado->prioridad);
    } else {
        t_cola_prioridades* cola_prioridad_nueva = create_priority_queue(hilo_del_proceso->tcb_asociado->prioridad);
        queue_push(cola_prioridad_nueva->cola, hilo_del_proceso);
        list_add(lista_colas_multinivel, cola_prioridad_nueva);
        log_info(kernel_log, "## [COLAS MULTINIVEL] Se agregó el hilo (%d:%d) a la NUEVA cola de prioridad %d en estado READY", hilo_del_proceso->pcb_padre->pid, hilo_del_proceso->tcb_asociado->tid, hilo_del_proceso->tcb_asociado->prioridad);
    }
}

t_cola_prioridades* queue_get_by_priority(t_list* lista_colas_prioridades, int prioridad){
    bool _queue_contains(void* ptr) {
	    t_cola_prioridades* cola = (t_cola_prioridades*) ptr;
	    return (prioridad == cola->prioridad);
	}
    t_cola_prioridades* cola_hallada = list_find(lista_colas_prioridades, _queue_contains);
	
    if (cola_hallada == NULL)
    {
        return (t_cola_prioridades*)0x0;
    }
    
    
    return cola_hallada;
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
        log_info(kernel_log, "## [PRIORIDADES] Se quitó el hilo (%d:%d) de la COLA READY CON PRIORIDAD %d", hilo->pcb_padre->pid, hilo->tcb_asociado->tid, hilo->tcb_asociado->prioridad);
    } else {
        log_debug(kernel_log, "## [PRIORIDADES] ERROR EN thread_find_by_priority_schedule");
        abort();
    }
}

t_hilo_planificacion* thread_find_by_multilevel_queues_schedule(t_list* lista_colas_multinivel){
    t_hilo_planificacion* hilo_a_ejecutar;

    pthread_mutex_lock(&mutex_colas_multinivel_existentes);

    int prioridad_maxima = maximum_priority_multilevel_queues_schedule(lista_colas_multinivel);

    for (int i = 0; i <= maximum_priority_multilevel_queues_schedule(lista_colas_multinivel); i++)
    {
        t_cola_prioridades* cola_hallada = queue_get_by_priority(lista_colas_multinivel, i);

        if(queue_find_by_priority(lista_colas_multinivel, i)){
            if (queue_is_empty(cola_hallada->cola))
            {
                log_debug(kernel_log, "## [COLAS MULTINIVEL] COLA CON PRIORIDAD: %d VACÍA", cola_hallada->prioridad);
            } else {
                hilo_a_ejecutar = (t_hilo_planificacion*)queue_pop(cola_hallada->cola);
                log_info(kernel_log, "## [COLAS MULTINIVEL] Se quitó el hilo (%d:%d) de la COLA CON PRIORIDAD %d", hilo_a_ejecutar->pcb_padre->pid, hilo_a_ejecutar->tcb_asociado->tid, cola_hallada->prioridad);
                i = maximum_priority_multilevel_queues_schedule(lista_colas_multinivel) +1;
            }
        } else {
            log_debug(kernel_log, "## [COLAS MULTINIVEL] COLA CON PRIORIDAD: %d NO EXISTE, TODAVÍA", i);
        }
    }
    pthread_mutex_unlock(&mutex_colas_multinivel_existentes);

    return hilo_a_ejecutar;
}

t_hilo_planificacion* thread_find_by_tid(t_list* lista, uint32_t tid){
    bool _list_contains(void* ptr) {
	    t_hilo_planificacion* hilo = (t_hilo_planificacion*) ptr;
	    return (tid == hilo->tcb_asociado->tid);
	}
	return list_find(lista, _list_contains);
}

pthread_mutex_t* find_by_name(t_list* lista_de_mutex, char* name){
    bool _name_equals(void* ptr) {
        t_mutex* mutexAEncontrar = (t_mutex*) ptr;
        return name == mutexAEncontrar->nombre;
    }
    return list_find(lista_de_mutex, _name_equals);
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

t_tcb* tid_find(t_list* lista_tcb, uint32_t tid){
    bool _list_contains(void* ptr){
        t_tcb* tcb_a_encontrar = (t_tcb*) ptr;
	    return (tcb_a_encontrar->tid == tid);
	}
	return list_find(lista_tcb, _list_contains);
}

int maximum_priority_multilevel_queues_schedule(t_list* lista_colas_prioridades){
    void* _max_priority(void* a, void* b) {
	    t_cola_prioridades* cola_a = (t_cola_prioridades*) a;
	    t_cola_prioridades* cola_b = (t_cola_prioridades*) b;
	    return cola_a->prioridad >= cola_b->prioridad ? cola_a : cola_b;
	}
	t_cola_prioridades* cola = list_get_maximum(lista_colas_prioridades, _max_priority);
	return cola->prioridad;
}

/*---------------------------- FUNCIONES EXECUTE ----------------------------*/

void* ejecutar_hilo(t_hilo_planificacion* hilo_a_ejecutar){
    //mover_hilo_a_exec(hilo_a_ejecutar);
    t_paquete* paquete = crear_paquete(EJECUTAR_HILO);

    t_buffer* buffer_envio = buffer_create(
        2*(sizeof(uint32_t))
        );

    buffer_add_uint32(buffer_envio, &hilo_a_ejecutar->pcb_padre->pid, kernel_log);
    buffer_add_uint32(buffer_envio, &hilo_a_ejecutar->tcb_asociado->tid, kernel_log);

    paquete->buffer = buffer_envio;

    enviar_paquete(paquete, fd_conexion_dispatch);
    log_debug(kernel_log, "SE ENVIO PAQUETE");
    eliminar_paquete(paquete);

    if(strcmp(kernel_registro.algoritmo_planificacion, "CMP") == 0){
        pthread_create(&hilo_quantum, NULL, (void*) iniciar_quantum, NULL);
        pthread_detach(hilo_quantum);
    }

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

char* avisar_creacion_proceso_memoria(int* pid, int* size_process, t_log* kernel_log){
    t_paquete* paquete = crear_paquete(CREAR_PROCESO);
    t_buffer* buffer = buffer_create(
        sizeof(int)*2
    );

    buffer_add_uint32(buffer, pid, kernel_log);
    buffer_add_uint32(buffer, size_process, kernel_log);

    paquete->buffer = buffer;

    pthread_mutex_lock(&mutex_uso_fd_memoria);
        int socket_memoria = crear_conexion_con_memoria(kernel_log, kernel_registro.ip_memoria, kernel_registro.puerto_memoria);
        enviar_paquete(paquete, socket_memoria);
        char* response_memoria = recibir_mensaje(socket_memoria, kernel_log);
        log_debug(kernel_log, "STRING RECIBIDO avisar_creacion_proceso_memoria: %s", response_memoria);
        close(socket_memoria);
    pthread_mutex_unlock(&mutex_uso_fd_memoria);

    return response_memoria;
}

char* avisar_creacion_hilo_memoria(int* pid, int* tid, char* path, int* prioridad, t_log* kernel_log){
    int length = strlen(path) + 1;
    t_paquete* paquete = crear_paquete(CREAR_HILO);
    t_buffer* buffer = buffer_create(
        length + sizeof(int)*3
    );

    buffer_add_string(buffer, length, path, kernel_log);
    buffer_add_uint32(buffer, (uint32_t*)pid, kernel_log);
    buffer_add_uint32(buffer, (uint32_t*)tid, kernel_log);
    buffer_add_uint32(buffer, (uint32_t*)prioridad, kernel_log);

    paquete->buffer = buffer;

    pthread_mutex_lock(&mutex_uso_fd_memoria);
        int socket_memoria = crear_conexion_con_memoria(kernel_log, kernel_registro.ip_memoria, kernel_registro.puerto_memoria);
        enviar_paquete(paquete, socket_memoria);
        char* response_memoria = recibir_mensaje(socket_memoria, kernel_log);
        log_debug(kernel_log, "STRING RECIBIDO avisar_creacion_hilo_memoria: %s", response_memoria);
        close(socket_memoria);
    pthread_mutex_unlock(&mutex_uso_fd_memoria);

    return response_memoria;
}

char* avisar_fin_proceso_memoria(uint32_t pid){
    t_paquete* paquete = crear_paquete(FINALIZAR_PROCESO);
    t_buffer* buffer = buffer_create(
        sizeof(int)
    );

    buffer_add_uint32(buffer, (uint32_t*)pid, kernel_log);

    paquete->buffer = buffer;

    pthread_mutex_lock(&mutex_uso_fd_memoria);
        int socket_memoria = crear_conexion_con_memoria(kernel_log, kernel_registro.ip_memoria, kernel_registro.puerto_memoria);
        enviar_paquete(paquete, socket_memoria);
        char* response_memoria = recibir_mensaje(socket_memoria, kernel_log);
        log_debug(kernel_log, "STRING RECIBIDO avisar_fin_proceso_memoria: %s", response_memoria);
        close(socket_memoria);
    pthread_mutex_unlock(&mutex_uso_fd_memoria);

    return response_memoria;
}

char* avisar_fin_hilo_memoria(uint32_t pid, uint32_t tid){
    t_paquete* paquete = crear_paquete(FINALIZAR_HILO);
    t_buffer* buffer = buffer_create(
        2*sizeof(int)
    );

    buffer_add_uint32(buffer, (uint32_t*)pid, kernel_log);
    buffer_add_uint32(buffer, (uint32_t*)tid, kernel_log);

    paquete->buffer = buffer;

    pthread_mutex_lock(&mutex_uso_fd_memoria);
        int socket_memoria = crear_conexion_con_memoria(kernel_log, kernel_registro.ip_memoria, kernel_registro.puerto_memoria);
        enviar_paquete(paquete, socket_memoria);
        char* response_memoria = recibir_mensaje(socket_memoria, kernel_log);
        log_debug(kernel_log, "STRING RECIBIDO avisar_fin_hilo_memoria: %s", response_memoria);
        close(socket_memoria);
    pthread_mutex_unlock(&mutex_uso_fd_memoria);

    buffer_destroy(buffer);
    eliminar_paquete(paquete);

    return response_memoria;
}

/*------------------------- FUNCIONES KERNEL - CPU --------------------------*/

void enviar_aviso_syscall(char* mensaje, cod_inst* codigo_instruccion){
    uint32_t length = strlen(mensaje) + 1;
    t_paquete* paquete = crear_paquete(AVISO_EXITO_SYSCALL);

	t_buffer* buffer;

	buffer = buffer_create(length);

	buffer_add_string(buffer, length, mensaje, kernel_log);
    buffer_add_uint32(buffer, codigo_instruccion, kernel_log);

	paquete->buffer = buffer;

	enviar_paquete(paquete, fd_conexion_interrupt);
	eliminar_paquete(paquete);
}

/*--------------------------------- SYSCALLS --------------------------------*/

void* operacion_a_atender(){
    int length;
    char* nombreMutex = string_new();
    uint32_t milisegundos_de_trabajo;
    int operacion = recibir_operacion(fd_conexion_interrupt);
    t_buffer* buffer = buffer_recieve(fd_conexion_interrupt);
    t_pid_tid pid_tid_recibido = recibir_pid_tid(buffer, kernel_log);
    
    switch (operacion)
    {
    case PROCESS_CREATE:        
        syscall_process_create(pid_tid_recibido.pid, pid_tid_recibido.tid);
        break;

    case PROCESS_EXIT:
        syscall_process_exit();
        break;

    case THREAD_CREATE:
        syscall_thread_create(buffer, pid_tid_recibido.pid, pid_tid_recibido.tid);
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
        nombreMutex = buffer_read_string(buffer, &length);
        syscall_mutex_create(nombreMutex , pid_tid_recibido.pid);
        break;

    case MUTEX_LOCK:
        nombreMutex = buffer_read_string(buffer, &length);
        syscall_mutex_lock(nombreMutex, pid_tid_recibido);
        break;
        
    case MUTEX_UNLOCK:
        nombreMutex = buffer_read_string(buffer, &length);
        syscall_mutex_unlock(nombreMutex, pid_tid_recibido);
        break;

    case DUMP_MEMORY:
        syscall_dump_memory();
        break;

    case IO:
        milisegundos_de_trabajo = buffer_read_uint32(buffer);
        syscall_io(milisegundos_de_trabajo);
        break;
    
    case FIN_INSTRUCCIONES:
        break;

    default:
        log_warning(kernel_log, "## Error en la OP enviada desde Kernel");
        break;
    }
}

void* ejecutar_io (uint32_t milisegundos){

    usleep(milisegundos);

    log_info(kernel_log,"Kernel atendio entrada/salida por: %i milisegundos ",milisegundos);
}

void* syscall_io(uint32_t milisegundos_de_trabajo){
    t_hilo_planificacion* hilo = hilo_en_ejecucion;

    poner_en_block(hilo_en_ejecucion);
    ejecutar_io(milisegundos_de_trabajo);
    
    poner_en_ready(hilo);
} 

void* syscall_mutex_lock(char* nombreMutex ,t_pid_tid pid_tid_recibido){
    t_pcb* pcb = list_find_by_pid(pid_tid_recibido.pid);

    t_mutex* mutex_encontrado = find_by_name(pcb->mutex_asociados,nombreMutex );

    if(mutex_encontrado != NULL){

       if(mutex_encontrado->tid_tomado == NULL){

            mutex_encontrado->tid_tomado = pid_tid_recibido.tid;
       }else{
        /*el hilo que realizó MUTEX_LOCK se bloqueará en la cola de bloqueados correspondiente a dicho mutex.*/
            poner_en_block(hilo_en_ejecucion);
            queue_push(mutex_encontrado->cola_bloqueados, pid_tid_recibido.tid);
       }
    }
}

void* syscall_mutex_unlock(char* nombreMutex ,t_pid_tid pid_tid_recibido){
    t_pcb* pcb = list_find_by_pid(pid_tid_recibido.pid);

    t_mutex* mutex_encontrado = find_by_name(pcb->mutex_asociados, nombreMutex);

    if(mutex_encontrado != NULL){
        if(mutex_encontrado->tid_tomado == pid_tid_recibido.tid){
            uint32_t tid_encontado = queue_pop(mutex_encontrado->cola_bloqueados);
            mutex_encontrado->tid_tomado = tid_encontado;
            t_hilo_planificacion* hilo = thread_find_by_tid(hilos_block, pid_tid_recibido.tid);
            poner_en_ready(hilo);
        }else{
            log_warning(kernel_log, "## [KERNEL] Se hizo UNLOCK de un MUTEX que no existe para el proceso PID: %d TID: %d",pid_tid_recibido.pid, pid_tid_recibido.tid);
        }
    }
}

void* syscall_mutex_create(char* nombreMutex, uint32_t pid){
    t_mutex* mutexCreado = create_mutex(nombreMutex);

    t_pcb* pcb = list_find_by_pid(pid);
    list_add(pcb->mutex_asociados,mutexCreado);
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
    t_tcb* tcb_main = create_tcb(pcb, prioridad_proceso);
    list_add(pcb->lista_tcb, tcb_main);

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
    log_info(kernel_log,"(%d:%d) - Solicitó syscall: DUMP_MEMORY", hilo_en_ejecucion->pcb_padre->pid, hilo_en_ejecucion->tcb_asociado->tid);
    t_paquete* paquete_aviso_dump_memory = crear_paquete(AVISO_DUMP_MEMORY);
    agregar_a_paquete(paquete_aviso_dump_memory,  hilo_en_ejecucion->pcb_padre->pid, sizeof(uint32_t));
    agregar_a_paquete(paquete_aviso_dump_memory,  hilo_en_ejecucion->tcb_asociado->tid, sizeof(uint32_t));

    //ENVIA EL PEDIDO A MEMORIA PARA FINALIZAR EL PROCESO
    pthread_mutex_lock(&mutex_uso_fd_memoria);
        int conexion_memoria = crear_conexion_con_memoria(kernel_log, kernel_registro.ip_memoria, kernel_registro.puerto_memoria);
        enviar_paquete(paquete_aviso_dump_memory, conexion_memoria);
        log_debug(kernel_log, "SE ENVIO AVISO DE DUMP_MEMORY");
        char* respuesta_memoria = recibir_mensaje(conexion_memoria, kernel_log);
        log_debug(kernel_log, "RESPUESTA DE MEMORIA POR MEMORY DUMP: %s", respuesta_memoria);
        
        if(strcmp(respuesta_memoria, "OK") == 0){

            /*Caso contrario, el hilo se desbloquea normalmente pasando a READY.*/

            t_hilo_planificacion* hilo_a_desbloquear = list_remove_by_condition(hilos_block, es_igual);
            poner_en_ready(hilo_a_desbloquear);
        }else{
            /*en caso de error, el proceso se enviará a EXIT. */

            t_pcb* pcb_a_liberar =  list_find_by_pid(hilo_en_ejecucion->pcb_padre->pid);
            free(pcb_a_liberar);
            log_debug(kernel_log,"Find de proceso:“## Finaliza el proceso <PID> %d", pcb_a_liberar->pid);
            inicializar_pcb_en_espera();
        }
        close(conexion_memoria);
    pthread_mutex_unlock(&mutex_uso_fd_memoria);
}

void* syscall_process_exit(){
    log_info(kernel_log,"(%d:%d) - Solicitó syscall: PROCESS_EXIT", hilo_en_ejecucion->pcb_padre->pid, hilo_en_ejecucion->tcb_asociado->tid);
    t_paquete* paquete_fin_proceso = crear_paquete(FINALIZAR_PROCESO);
    agregar_a_paquete(paquete_fin_proceso,  hilo_en_ejecucion->pcb_padre->pid, sizeof(uint32_t));

    //ENVIA EL PEDIDO A MEMORIA PARA FINALIZAR EL PROCESO
    pthread_mutex_lock(&mutex_uso_fd_memoria);
        int conexion_memoria = crear_conexion_con_memoria(kernel_log, kernel_registro.ip_memoria, kernel_registro.puerto_memoria);
        enviar_paquete(paquete_fin_proceso, conexion_memoria);
        log_debug(kernel_log, "SE ENVIO AVISO DE FINALIZACION DE PROCESO");

        //RECIBO EL PAQUETE CON LA RESPUESTA DE MEMORIA 
        char* respuesta_memoria = recibir_mensaje(conexion_memoria,kernel_log);
        log_debug(kernel_log, "RESPUESTA DE MEMORIA POR PROCESS EXIT: %s", respuesta_memoria);

        if(strcmp(respuesta_memoria, "FINALIZACION_ACEPTADA") == 0){
            t_pcb* pcb_a_liberar =  list_find_by_pid(hilo_en_ejecucion->pcb_padre->pid);
            free(pcb_a_liberar);
        
            log_debug(kernel_log,"Find de proceso:“## Finaliza el proceso <PID> %d", pcb_a_liberar->pid);
            inicializar_pcb_en_espera();
        }
        close(conexion_memoria);
    pthread_mutex_unlock(&mutex_uso_fd_memoria);
}

void* syscall_thread_create(t_buffer* buffer, uint32_t pid_solicitante, uint32_t tid_solicitante){
    log_info(kernel_log,"(%d:%d) - Solicitó syscall: THREAD_CREATE", pid_solicitante, tid_solicitante);
    uint32_t largo_string = 0;
    inst_cpu codigo = THREAD_CREATE;

    int prioridad_hilo = buffer_read_uint32(buffer);
    char* path = buffer_read_string(buffer, &largo_string);

    t_tcb* tcb = create_tcb(hilo_en_ejecucion->pcb_padre, prioridad_hilo);
    char* respuesta_creacion_hilo = avisar_creacion_hilo_memoria(&hilo_en_ejecucion->pcb_padre->pid, &tcb->tid, path, &prioridad_hilo, kernel_log);

    if (strcmp(respuesta_creacion_hilo, "OK") == 0)
    {
        t_hilo_planificacion* hilo_a_crear = create_hilo_planificacion(hilo_en_ejecucion->pcb_padre, tcb);
        poner_en_ready(hilo_a_crear);
        enviar_aviso_syscall("HILO_CREADO", &codigo);
    } else {
        enviar_aviso_syscall("HILO_NO_CREADO", &codigo);
        log_debug(kernel_log, "Rompimos algo con syscall_thread_create");
    }
}

void* syscall_thread_join(){
    /*uint32_t largo_string = 0;
    t_buffer* buffer;
    buffer = buffer_recieve(fd_conexion_dispatch);
    uint32_t tid = buffer_read_uint32(buffer);
    t_hilo_planificacion* hilo_a_bloquear = list_get(hilo_exec,0);
    list_add(hilo_a_bloquear->lista_hilos_block, tid);

    hilo_a_bloquear->estado = BLOCKED_STATE;
    
    poner_en_block();
    buffer_destroy(buffer);*/
}

void* syscall_thread_exit(t_hilo_planificacion* hilo){
    log_info(kernel_log,"##(%d:%d) - Solicitó syscall: THREAD_EXIT", hilo->pcb_padre->pid, hilo->tcb_asociado->tid);
    
    char* respuesta_memoria = avisar_fin_hilo_memoria(hilo->pcb_padre->pid, hilo->tcb_asociado->tid);
    
    if (strcmp(respuesta_memoria, "OK"))
    {
        log_debug(kernel_log, "SE ENVIO AVISO DE FINALIZACION DE HILO PID: %d TID: %d", hilo->pcb_padre->pid, hilo->tcb_asociado->tid);
        liberar_hilos_bloqueados(hilo);
        t_hilo_planificacion_destroy(hilo);
        free(respuesta_memoria);
    } else {
        log_debug(kernel_log, "Rompimos algo con finalizar hilo");
    }
}

void* syscall_thread_cancel(uint32_t tid){
    t_pcb* pcb = list_find_by_pid(hilo_en_ejecucion->pcb_padre->pid);
    t_hilo_planificacion* resultado = tid_find(pcb->lista_tcb, tid);

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
    free(pcb->lista_tcb);
    list_destroy(pcb->lista_tcb);
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

uint32_t siguiente_pid(){
    pthread_mutex_lock(&mutex_siguiente_id);
    pid_siguiente = pid_actual++;
    pthread_mutex_unlock(&mutex_siguiente_id);
    return pid_siguiente;
}

void signal_handler(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        printf("Recibida señal para terminar el servidor.\n");
        sem_post(&kernel_activo);  // Desbloquear el servidor
    }
}

void iniciar_quantum(){
    usleep(kernel_registro.quantum * 1000); 

    if(!termino_proceso){
        t_paquete* paquete = crear_paquete(INTERRUPCION_QUANTUM);
        enviar_paquete(paquete, fd_conexion_interrupt);
        eliminar_paquete(paquete);
    }
}