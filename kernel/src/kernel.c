#include <kernel.h>

int main(int argc, char* argv[]) {

    //---------------------------- Iniciar archivos ----------------------------

    //kernel_log = iniciar_logger("./files/kernel.log", "KERNEL", 1, LOG_LEVEL_DEBUG);

    kernel_log = iniciar_logger("./files/kernel_obligatorio.log", "KERNEL", 1, LOG_LEVEL_INFO);

    kernel_config = iniciar_config(argv[3]);

    kernel_registro = levantar_datos(kernel_config);

    // --------------------- Conexión como cliente de CPU ----------------------

    fd_conexion_dispatch = crear_conexion(kernel_log, kernel_registro.ip_cpu, kernel_registro.puerto_cpu_dispatch);
    log_debug(kernel_log, "ME CONECTÉ A CPU DISPATCH");

    fd_conexion_interrupt = crear_conexion(kernel_log, kernel_registro.ip_cpu, kernel_registro.puerto_cpu_interrupt);
    log_debug(kernel_log, "ME CONECTÉ A CPU INTERRUPT");

    // --------------------- Inicio del modulo ----------------------

    signal(SIGTERM, signal_handler);  // Capturar la señal SIGTERM
    signal(SIGINT, signal_handler);   // Capturar la señal SIGINT (Ctrl+C)

    thread_states = dictionary_create();
    iniciar_listas();
    iniciar_semaforos();
    iniciar_colas();
    iniciar_primer_proceso(argv[1], argv[2]);
    iniciar_planificacion();

    sem_wait(&kernel_activo);

    // --------------------- Finalizacion del modulo----------------------

    log_warning(kernel_log, "TERMINANDO KERNEL");
    finalizar_modulo();

    //LIBERAR COLAS Y ELEMENTOS QUE CONTIENE
}

/*----------------------- FUNCIONES DE INICIALIZACIÓN -----------------------*/

void iniciar_listas(){
    procesos_creados = list_create();
    cola_ready = list_create();
    lista_t_hilos_bloqueados = list_create();
    lista_colas_multinivel = list_create();
    lista_prioridades = list_create();
    lista_hilo_en_ejecucion = list_create();
}

void iniciar_semaforos(){
    pthread_mutex_init(&mutex_cola_new, NULL);
    pthread_mutex_init(&mutex_cola_ready, NULL);
    pthread_mutex_init(&mutex_cola_block, NULL);
    pthread_mutex_init(&mutex_hilo_exec, NULL);
    pthread_mutex_init(&mutex_siguiente_id, NULL);
    pthread_mutex_init(&mutex_uso_fd_memoria, NULL);
    pthread_mutex_init(&mutex_uso_estado_hilos, NULL);
    pthread_mutex_init(&mutex_lista_procesos_ready, NULL);
    pthread_mutex_init(&mutex_colas_multinivel_existentes, NULL);
    pthread_mutex_init(&mutex_io, NULL);
    pthread_mutex_init(&mutex_interrupt, NULL);

    sem_init(&contador_procesos_en_new, 0, 0);
    sem_init(&kernel_activo, 0, 0);
    sem_init(&aviso_exit_proceso, 0, 0);
}

void iniciar_colas(){
    cola_new = queue_create();
}

void iniciar_primer_proceso(char* path, char* size){
    int size_process = atoi(size);
    int prioridad = 0;

    primer_proceso = create_pcb(path, size_process);
    char* response_memoria = avisar_creacion_proceso_memoria(&(primer_proceso->pid), &size_process, kernel_log);

    if (strcmp(response_memoria, "OK") == 0)
    {   
        create_process_state(primer_proceso->pid);
        log_info(kernel_log, "## (<PID>:%d) Se crea el proceso - Estado: NEW", primer_proceso->pid);
        t_tcb* primer_hilo = create_tcb(primer_proceso, 0);
        char* respuesta_creacion_hilo = avisar_creacion_hilo_memoria(&primer_proceso->pid, &primer_hilo->tid, path, &prioridad, kernel_log);

        if (strcmp(respuesta_creacion_hilo, "OK") == 0)
        {
            t_hilo_planificacion* hilo = create_hilo_planificacion(primer_proceso, primer_hilo);
            create_thread_state(primer_proceso->pid, primer_hilo->tid, 0);
            poner_en_ready(hilo);
            log_info(kernel_log, "## (<%d>:<%d>) Se crea el Hilo - Estado: READY", hilo->pid_padre, hilo->tid_asociado);
            agregar_proceso_activo(primer_proceso);
        } else {
            log_error(kernel_log, "ERROR AL INICIAR PRIMER HILO");
            abort();
        }
        free(respuesta_creacion_hilo);
    } else {
        log_error(kernel_log, "ERROR AL INICIAR PRIMER PROCESO");
        abort();
    }
    free(response_memoria);
}

void iniciar_planificacion(){
    pthread_create(&hiloPlanifCortoPlazo, NULL, (void*)planificador_corto_plazo, NULL);
    pthread_detach(hiloPlanifCortoPlazo);

    pthread_create(&hiloNew, NULL, planificador_largo_plazo, NULL);
    pthread_detach(hiloNew);
}

/*------------------------- PLANIFICADOR LARGO PLAZO ------------------------*/

void* planificador_largo_plazo(){
    while (1)
    {
        inicializar_pcb_en_espera();
    }
}

void* inicializar_pcb_en_espera(){        
    sem_wait(&contador_procesos_en_new);
    if(queue_size(cola_new) > 0){
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
                create_thread_state(pcb->pid, tcb_main->tid, tcb_main->prioridad);
                poner_en_ready(hilo);
            } else {
                log_debug(kernel_log, "NO SE PUDO INICIALIZAR HILO EN MEMORIA");
            }
            free(respuesta_memoria_hilo);
        } else {
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
                create_thread_state(pcb->pid, tcb_asociado->tid, tcb_asociado->prioridad);
                poner_en_ready(hilo);
            } else {
                log_debug(kernel_log, "NO SE PUDO REINTENTAR INICIALIZAR HILO EN MEMORIA");
            }
            free(respuesta_memoria_hilo);
        } else {
            sem_wait(&aviso_exit_proceso);
            reintentar_inicializar_pcb_en_espera(pcb);
        }
    free(respuesta_memoria);
}

void poner_en_new(t_pcb* pcb){
    pthread_mutex_lock(&mutex_cola_new);
        queue_push(cola_new, pcb);
    pthread_mutex_unlock(&mutex_cola_new);
    sem_post(&contador_procesos_en_new);
}

void poner_en_exit(uint32_t pid, uint32_t tid){
    t_hilo_planificacion* hilo_a_eliminar;
    t_thread_state* estado_hilo = get_thread_state(pid, tid);

    switch (estado_hilo->estado)
    {
    case READY_STATE:
        hilo_a_eliminar = remover_de_ready(pid, tid, estado_hilo->prioridad);
        break;
    
    case BLOCKED_STATE:
        hilo_a_eliminar = remover_de_block(pid, tid);
        break;

    case EXEC_STATE:
        hilo_a_eliminar = desalojar_hilo();
        break;

    default:
        log_error(kernel_log, "Error en eliminar_hilo_del_planificador");
        abort();
        break;
    }
    change_thread_state(pid, tid, EXIT_STATE);

    char* response_memoria = avisar_fin_hilo_memoria(&pid, &tid);
    log_debug(kernel_log, "STRING RECIBIDO poner_en_exit: %s", response_memoria);

    liberar_hilos_bloqueados_por_tid(hilo_a_eliminar);
    log_info(kernel_log, "## (<%d>:<%d>) Finaliza el hilo", hilo_a_eliminar->pid_padre, hilo_a_eliminar->tid_asociado);
    eliminar_tcb_de_pcb(hilo_a_eliminar);
    t_hilo_planificacion_destroy(hilo_a_eliminar);

    free(response_memoria);
    sem_post(&aviso_exit_proceso);
}

void eliminar_tcb_de_pcb(t_hilo_planificacion* hilo){
    pthread_mutex_lock(&mutex_lista_procesos_ready);
        t_pcb* pcb = active_process_find_by_pid(hilo->pid_padre);
        t_tcb* tcb = tcb_remove_by_tid(pcb, hilo->tid_asociado);
        free(tcb);
    pthread_mutex_unlock(&mutex_lista_procesos_ready);
}

void finalizar_hilos_de_proceso(uint32_t pid, uint32_t tid){
    t_pcb* pcb = active_process_find_by_pid(pid);
    while (!list_is_empty(pcb->lista_tcb))
    {
        poner_en_exit(pid, tid);
    }
}

/*------------------------- PLANIFICADOR CORTO PLAZO ------------------------*/

void* planificador_corto_plazo(){
    char* planificacion = kernel_registro.algoritmo_planificacion;
    hilo_en_ejecucion = NULL;
    bool finalizar_corto_plazo = false;
    while (hilo_en_ejecucion || !finalizar_corto_plazo)
    {
        hilo_en_ejecucion = obtener_hilo_segun_algoritmo(planificacion);
        if (hilo_en_ejecucion)
        {
            hilo_desalojado = false;
            ejecutar_hilo(hilo_en_ejecucion);
        } else {
            log_info(kernel_log, "No hay más hilos para planificar, cierro modulo KERNEL");
            finalizar_corto_plazo = true;
        }
    }
    sem_post(&kernel_activo);
}

t_hilo_planificacion* obtener_hilo_segun_algoritmo(char* planificacion){
    t_hilo_planificacion* hilo = NULL;

    if (strcmp(planificacion, "FIFO") == 0)
    {
        pthread_mutex_lock(&mutex_cola_ready);
            if (list_size(cola_ready) > 0) {
                hilo = list_remove(cola_ready, 0);
            }
            if (!hilo) {
                log_warning(kernel_log, "No hay hilos en la cola ready");
            } else {
                log_info(kernel_log, "## [FIFO](%d:%d) Se seleccionó para ejecutar y pasa al estado EXECUTE", hilo->pid_padre, hilo->tid_asociado);
            }
        pthread_mutex_unlock(&mutex_cola_ready);

    } else if (strcmp(planificacion, "PRIORIDADES") == 0)
    {
        pthread_mutex_lock(&mutex_cola_ready);
            if (list_size(lista_prioridades) > 0) {
                hilo = thread_find_by_priority_schedule(lista_prioridades);
            }
            if (!hilo) {
                log_warning(kernel_log, "No hay hilos en la cola ready");
            } else {
                log_info(kernel_log, "## [PRIORIDADES](%d:%d) Se seleccionó para ejecutar y pasa al estado EXECUTE", hilo->pid_padre, hilo->tid_asociado);
            }
        pthread_mutex_unlock(&mutex_cola_ready);

    } else if (strcmp(planificacion, "CMN") == 0)
    {
        pthread_mutex_lock(&mutex_cola_ready);
            if (list_size(lista_prioridades) > 0){
                hilo = thread_find_by_multilevel_queues_schedule(lista_colas_multinivel);
            }
            if (!hilo) {
                log_warning(kernel_log, "No hay hilos en la cola ready");
            }
        pthread_mutex_unlock(&mutex_cola_ready);
    }
    return hilo;
}

void* poner_en_ready(t_hilo_planificacion* hilo_del_proceso){
    pthread_mutex_lock(&mutex_cola_ready);    
    change_thread_state(hilo_del_proceso->pid_padre, hilo_del_proceso->tid_asociado, READY_STATE);
    if(strcmp(kernel_registro.algoritmo_planificacion, "FIFO") == 0){
        list_add(cola_ready, hilo_del_proceso);

    } else if(strcmp(kernel_registro.algoritmo_planificacion, "PRIORIDADES") == 0){
        list_add(lista_prioridades, hilo_del_proceso);

    }else if(strcmp(kernel_registro.algoritmo_planificacion, "CMN") == 0){
        log_error(kernel_log, "VOY A PLANIFICAR");
        queue_push_by_priority(hilo_del_proceso);
    }else{
        log_error(kernel_log,"NO SE RECONOCE LA PLANIFICACION");
        abort();
    }
    pthread_mutex_unlock(&mutex_cola_ready);
}

void* poner_en_block(t_hilo_planificacion* hilo_del_proceso){
    pthread_mutex_lock(&mutex_cola_block);
        change_thread_state(hilo_del_proceso->pid_padre, hilo_del_proceso->tid_asociado, BLOCKED_STATE);
        list_add(lista_t_hilos_bloqueados, hilo_del_proceso);
    pthread_mutex_unlock(&mutex_cola_block);
}

void agregar_proceso_activo(t_pcb* pcb){
    pthread_mutex_lock(&mutex_lista_procesos_ready);
        list_add(procesos_creados, pcb);
    pthread_mutex_unlock(&mutex_lista_procesos_ready);
}

t_hilo_planificacion* remover_de_ready(uint32_t pid, uint32_t tid, uint32_t prioridad){
    t_hilo_planificacion* hilo_a_eliminar;
    
    if (strcmp(kernel_registro.algoritmo_planificacion, "CMN") == 0){
        pthread_mutex_lock(&mutex_cola_ready);
            t_cola_prioridades* cola = queue_get_by_priority(lista_colas_multinivel, prioridad);
            hilo_a_eliminar = thread_remove_by_tid(cola->cola, pid, tid);
        pthread_mutex_unlock(&mutex_cola_ready);

    } else if(strcmp(kernel_registro.algoritmo_planificacion, "PRIORIDADES") == 0){
        pthread_mutex_lock(&mutex_cola_ready);
            hilo_a_eliminar = thread_remove_by_tid(lista_prioridades, pid, tid);
        pthread_mutex_unlock(&mutex_cola_ready);

    } else {
        pthread_mutex_lock(&mutex_cola_ready);
            hilo_a_eliminar = thread_remove_by_tid(cola_ready, pid, tid);
        pthread_mutex_unlock(&mutex_cola_ready);
    }
    return hilo_a_eliminar;
}

t_hilo_planificacion* remover_de_block(uint32_t pid, uint32_t tid){
    t_hilo_planificacion* hilo_a_eliminar;
    
    pthread_mutex_lock(&mutex_cola_block);
        hilo_a_eliminar = thread_remove_by_tid(lista_t_hilos_bloqueados, pid, tid);
    pthread_mutex_unlock(&mutex_cola_block);

    return hilo_a_eliminar;
}

/*--------------------------- FUNCIONES DE COLAS ----------------------------*/

void queue_push_by_priority(t_hilo_planificacion* hilo_del_proceso){
    if (queue_find_by_priority(lista_colas_multinivel, hilo_del_proceso->prioridad))
    {
        t_cola_prioridades* cola_prioridad = queue_get_by_priority(lista_colas_multinivel, hilo_del_proceso->prioridad);
        list_add(cola_prioridad->cola, hilo_del_proceso);
        log_info(kernel_log, "## [COLAS MULTINIVEL] Se agregó el hilo (%d:%d) a la cola de prioridad %d en estado READY", hilo_del_proceso->pid_padre, hilo_del_proceso->tid_asociado, hilo_del_proceso->prioridad);
    } else {
        t_cola_prioridades* cola_prioridad_nueva = create_priority_queue(hilo_del_proceso->prioridad);
        list_add(cola_prioridad_nueva->cola, hilo_del_proceso);
        list_add(lista_colas_multinivel, cola_prioridad_nueva);
        log_info(kernel_log, "## [COLAS MULTINIVEL] Se agregó el hilo (%d:%d) a la NUEVA cola de prioridad %d en estado READY", hilo_del_proceso->pid_padre, hilo_del_proceso->tid_asociado, hilo_del_proceso->prioridad);
    }
}

bool queue_find_by_priority(t_list* lista_colas_multinivel, int prioridad){
    bool _queue_contains(void* ptr){
	    t_cola_prioridades* cola = (t_cola_prioridades*) ptr;
	    return (prioridad == cola->prioridad);
	}
	return list_any_satisfy(lista_colas_multinivel, _queue_contains);
}

t_cola_prioridades* queue_get_by_priority(t_list* lista_colas_multinivel, int prioridad){
    bool _queue_contains(void* ptr) {
	    t_cola_prioridades* cola = (t_cola_prioridades*) ptr;
	    return (prioridad == cola->prioridad);
	}
    t_cola_prioridades* cola_hallada = list_find(lista_colas_multinivel, _queue_contains);
    return cola_hallada;
}

int maximum_priority_multilevel_queues_schedule(t_list* lista_colas_multinivel){
    void* _max_priority(void* a, void* b) {
	    t_cola_prioridades* cola_a = (t_cola_prioridades*) a;
	    t_cola_prioridades* cola_b = (t_cola_prioridades*) b;
	    return cola_a->prioridad >= cola_b->prioridad ? cola_a : cola_b;
	}
	t_cola_prioridades* cola = list_get_maximum(lista_colas_multinivel, _max_priority);
	return cola->prioridad;
}

/*-------------------------- FUNCIONES DE LISTAS ----------------------------*/

/*--------------------------- FUNCIONES DE HILOS ----------------------------*/

t_hilo_planificacion* thread_find_by_maximum_priority(t_list* lista_prioridades){
    void* _min_priority(void* a, void* b) {
	    t_hilo_planificacion* hilo_a = (t_hilo_planificacion*) a;
	    t_hilo_planificacion* hilo_b = (t_hilo_planificacion*) b;
	    return hilo_a->prioridad <= hilo_b->prioridad ? hilo_a : hilo_b;
	}
	t_hilo_planificacion* hilo_encontrado = list_get_minimum(lista_prioridades, _min_priority);
    return hilo_encontrado;
}

t_hilo_planificacion* thread_find_by_priority_schedule(t_list* lista_prioridades){
    t_hilo_planificacion* hilo = thread_find_by_maximum_priority(lista_prioridades);
    t_hilo_planificacion* hilo_hallado = thread_remove_by_tid(lista_prioridades, hilo->pid_padre, hilo->tid_asociado);
    
    return hilo_hallado;
}

t_hilo_planificacion* thread_find_by_multilevel_queues_schedule(t_list* lista_colas_multinivel){
    t_hilo_planificacion* hilo_a_ejecutar;

    pthread_mutex_lock(&mutex_colas_multinivel_existentes);
    for (int i = 0; i <= maximum_priority_multilevel_queues_schedule(lista_colas_multinivel); i++)
    {
        t_cola_prioridades* cola_hallada = queue_get_by_priority(lista_colas_multinivel, i);

        if(queue_find_by_priority(lista_colas_multinivel, i)){
            if (list_is_empty(cola_hallada->cola))
            {
                log_debug(kernel_log, "## [COLAS MULTINIVEL] COLA CON PRIORIDAD: %d VACÍA", cola_hallada->prioridad);
            } else {
                hilo_a_ejecutar = list_remove(cola_hallada->cola, 0);
                log_info(kernel_log, "## [COLAS MULTINIVEL](%d:%d) - Prioridad %d Se seleccionó para ejecutar y pasa al estado EXECUTE", hilo_a_ejecutar->pid_padre, hilo_a_ejecutar->tid_asociado, cola_hallada->prioridad);
                i = maximum_priority_multilevel_queues_schedule(lista_colas_multinivel) +1;
            }
        } else {
            log_debug(kernel_log, "## [COLAS MULTINIVEL] COLA CON PRIORIDAD: %d NO EXISTE, TODAVÍA", i);
        }
    }
    pthread_mutex_unlock(&mutex_colas_multinivel_existentes);

    return hilo_a_ejecutar;
}

t_hilo_planificacion* thread_find_by_tid(t_list* lista, t_pid_tid pid_tid){
    bool _list_contains(void* ptr) {
	    t_hilo_planificacion* hilo = (t_hilo_planificacion*) ptr;
	    return ((pid_tid.tid == hilo->tid_asociado) &&(pid_tid.pid == hilo->pid_padre));
	}
	return list_find(lista, _list_contains);
}

t_hilo_planificacion* thread_remove_by_tid(t_list* lista, uint32_t pid, uint32_t tid){
    bool _list_contains(void* ptr) {
	    t_hilo_planificacion* hilo = (t_hilo_planificacion*) ptr;
	    return ((tid == hilo->tid_asociado) &&(pid == hilo->pid_padre));
	}
	return list_remove_by_condition(lista, _list_contains);
}

void liberar_hilos_bloqueados_por_tid(t_hilo_planificacion* hilo){
    bool hilos_desbloqueados = false;
    while (!hilos_desbloqueados)
    {
        if(exist_thread_blocked_by_tid(hilo->pid_padre, hilo->tid_asociado)){

            t_thread_state* thread_state = get_thread_blocked_by_tid(hilo->pid_padre, hilo->tid_asociado);

            change_thread_state(hilo->pid_padre, thread_state->tid, READY_STATE);
            remove_thread_state_tid_blocked(hilo->pid_padre, thread_state->tid);
            t_hilo_planificacion* hilo_obtenido = remover_de_block(hilo->pid_padre, thread_state->tid);
            poner_en_ready(hilo_obtenido);
        } else {
            hilos_desbloqueados = true;
        }
    }
}

bool is_thread_exist(uint32_t pid, uint32_t tid){
    t_pcb* pcb = active_process_find_by_pid(pid);

    bool _list_contains(void* ptr){
	    t_tcb* hilo = (t_tcb*) ptr;
	    return (hilo->tid == tid && pcb->pid == pid);
	}
	return list_any_satisfy(pcb->lista_tcb, _list_contains);
}

t_tcb* tid_find(t_list* lista_tcb, uint32_t tid){
    bool _list_contains(void* ptr){
        t_tcb* tcb_a_encontrar = (t_tcb*) ptr;
	    return (tcb_a_encontrar->tid == tid);
	}
	return list_find(lista_tcb, _list_contains);
}

t_tcb* tcb_remove_by_tid(t_pcb* pcb, uint32_t tid){
    bool _list_contains(void* ptr){
	    t_tcb* tcb_a_encontrar = (t_tcb*) ptr;
	    return (tcb_a_encontrar->tid == tid);
	}
	return list_remove_by_condition(pcb->lista_tcb, _list_contains);
}

/*---------------------------- FUNCIONES DE PCB -----------------------------*/

t_pcb* active_process_find_by_pid(uint32_t pid){ 
    bool _list_contains(void* ptr) {
	    t_pcb* pcb_a_encontrar = (t_pcb*) ptr;
	    return (pcb_a_encontrar->pid == pid);
	}
	return list_find(procesos_creados, _list_contains);
}

t_pcb* active_process_remove_by_pid(uint32_t pid){
    pthread_mutex_lock(&mutex_lista_procesos_ready);
    bool _list_contains(void* ptr){
	    t_pcb* pcb_a_encontrar = (t_pcb*) ptr;
	    return (pcb_a_encontrar->pid == pid);
	}
    pthread_mutex_unlock(&mutex_lista_procesos_ready);
	return list_remove_by_condition(procesos_creados, _list_contains);
}

/*----------------------- FUNCIONES DE ESTADO DE HILOS ----------------------*/

void create_process_state(uint32_t pid){
    t_list* estados_hilos = list_create();
    
    char* key = string_itoa(pid);
    pthread_mutex_lock(&mutex_uso_estado_hilos);
        dictionary_put(thread_states, key, estados_hilos);
    pthread_mutex_unlock(&mutex_uso_estado_hilos);

    free(key);
}

void create_thread_state(uint32_t pid, uint32_t tid, uint32_t prioridad){
    t_thread_state* estado_hilo = malloc(sizeof(t_thread_state));

    estado_hilo->tid = tid;
    estado_hilo->estado = READY_STATE;
    estado_hilo->prioridad = prioridad;
    estado_hilo->tid_bloqueante = -1;

    pthread_mutex_lock(&mutex_uso_estado_hilos);
        t_list* lista_estados_hilos = get_list_thread_state(pid);
        list_add(lista_estados_hilos, estado_hilo);
    pthread_mutex_unlock(&mutex_uso_estado_hilos);

    log_debug(kernel_log, "## Se creo el estado del hilo (%d:%d)", pid, tid);
}

t_list* get_list_thread_state(uint32_t pid){
    char* key = string_itoa(pid);
    t_list* lista_estados_hilos = dictionary_get(thread_states, key);
    log_debug(kernel_log, "## Se obtuvo la lista de estados de hilos del PID: %d", pid);
    free(key);

    return lista_estados_hilos;
}

t_thread_state* get_thread_state(uint32_t pid, uint32_t tid){
    t_list* lista_estados_hilos = get_list_thread_state(pid);
    t_thread_state* estado_hilo = find_thread_state_by_tid(lista_estados_hilos, tid);
    log_debug(kernel_log, "## Se obtuvo el estado del hilo (%d:%d)", pid, tid);

    return estado_hilo;
}

t_thread_state* find_thread_state_by_tid(t_list* lista, uint32_t tid){
    bool _list_contains(void* ptr) {
	    t_thread_state* estado = (t_thread_state*) ptr;
	    return (tid == estado->tid);
	}
	return list_find(lista, _list_contains);
}

void change_thread_state(uint32_t pid, uint32_t tid, estado_proceso estado){
    pthread_mutex_lock(&mutex_uso_estado_hilos);
        t_thread_state* estado_hilo = get_thread_state(pid, tid);
        estado_hilo->estado = estado;
    pthread_mutex_unlock(&mutex_uso_estado_hilos);
    log_debug(kernel_log, "## (%d:%d) Se cambió el estado del hilo. Valor: %d", pid, tid, estado_hilo->estado);
}

void add_thread_state_tid_blocked(uint32_t pid, uint32_t tid_bloqueado, uint32_t tid_bloqueante){
    pthread_mutex_lock(&mutex_uso_estado_hilos);
        t_thread_state* estado_hilo = get_thread_state(pid, tid_bloqueado);
        estado_hilo->estado = BLOCKED_STATE;
        estado_hilo->tid_bloqueante = tid_bloqueante;
    pthread_mutex_unlock(&mutex_uso_estado_hilos);
    log_debug(kernel_log, "## El hilo (%d:%d) fue bloqueado por el hilo (%d:%d)", pid, tid_bloqueado, pid, estado_hilo->tid_bloqueante);
}

void remove_thread_state_tid_blocked(uint32_t pid, uint32_t tid_bloqueado){
    pthread_mutex_lock(&mutex_uso_estado_hilos);
        t_thread_state* estado_hilo = get_thread_state(pid, tid_bloqueado);
        estado_hilo->tid_bloqueante = -1;
    pthread_mutex_unlock(&mutex_uso_estado_hilos);
    log_debug(kernel_log, "## El hilo (%d:%d) fue desbloqueado", pid, tid_bloqueado);
}

bool exist_thread_blocked_by_tid(uint32_t pid, uint32_t tid_bloqueado){
    t_list* estados_hilos = get_list_thread_state(pid);

        bool _list_contains(void* ptr){
	        t_thread_state* estado_hilo = (t_thread_state*) ptr;
	        return ((tid_bloqueado == estado_hilo->tid_bloqueante) && (BLOCKED_STATE == estado_hilo->estado));
	    }
	return list_any_satisfy(estados_hilos, _list_contains);
}

t_thread_state* get_thread_blocked_by_tid(uint32_t pid, uint32_t tid_bloqueante){
    t_list* estados_hilos = get_list_thread_state(pid);

        bool _list_contains(void* ptr){
	        t_thread_state* estado_hilo = (t_thread_state*) ptr;
	        return ((tid_bloqueante == estado_hilo->tid_bloqueante) && (BLOCKED_STATE == estado_hilo->estado));
	    }
	return list_find(estados_hilos, _list_contains);
}

/*--------------------------- FUNCIONES DE MUTEX ----------------------------*/

t_mutex* find_mutex_by_name(t_list* lista_de_mutex, char* name){
    bool _name_equals(void* ptr) {
        t_mutex* mutexAEncontrar = (t_mutex*) ptr;
        return name == mutexAEncontrar->nombre;
    }
    return list_find(lista_de_mutex, _name_equals);
}

bool mutex_any_satisfy_by_name(t_list* lista_de_mutex, char* name){
    bool _name_equals(void* ptr) {
        t_mutex* mutexAEncontrar = (t_mutex*) ptr;
        return name == mutexAEncontrar->nombre;
    }
    return list_any_satisfy(lista_de_mutex, _name_equals);
}

bool mutex_taken_by_tid(t_list* lista_de_mutex, uint32_t tid, char* name){
    bool _mutex_equals(void* ptr) {
        t_mutex* mutexAEncontrar = (t_mutex*) ptr;
        return (name == mutexAEncontrar->nombre) && (tid == mutexAEncontrar->tid_tomado);
    }
    return list_any_satisfy(lista_de_mutex, _mutex_equals);
}

bool is_mutex_exist(uint32_t pid, char* nombre){
    pthread_mutex_lock(&mutex_lista_procesos_ready);
        t_pcb* pcb = active_process_find_by_pid(pid);
        bool bool_mutex = mutex_any_satisfy_by_name(pcb->mutex_asociados, nombre);
    pthread_mutex_unlock(&mutex_lista_procesos_ready);

    return bool_mutex;
}

bool is_mutex_taken_by_tid(uint32_t pid, uint32_t tid, char* nombre){
    pthread_mutex_lock(&mutex_lista_procesos_ready);
        t_pcb* pcb = active_process_find_by_pid(pid);
        bool bool_mutex = mutex_taken_by_tid(pcb->mutex_asociados, tid, nombre);
    pthread_mutex_unlock(&mutex_lista_procesos_ready);
    
    return bool_mutex;
}

int is_mutex_taken(uint32_t pid, char* nombre){
    pthread_mutex_lock(&mutex_lista_procesos_ready);
        t_pcb* pcb = active_process_find_by_pid(pid);
        uint32_t valor_mutex = mutex_value(pcb->mutex_asociados, nombre);
    pthread_mutex_unlock(&mutex_lista_procesos_ready);
    
    return valor_mutex;
}

void block_thread_mutex(uint32_t pid, uint32_t tid, char* nombre){
    pthread_mutex_lock(&mutex_lista_procesos_ready);
        t_pcb* pcb = active_process_find_by_pid(pid);
        block_tid_by_mutex(pcb->mutex_asociados, nombre, tid);
    pthread_mutex_unlock(&mutex_lista_procesos_ready);
}

void unblock_thread_mutex(uint32_t pid, char* nombre){
    pthread_mutex_lock(&mutex_lista_procesos_ready);
        t_pcb* pcb = active_process_find_by_pid(pid);
        uint32_t tid = unblock_tid_by_mutex(pcb->mutex_asociados, nombre);
    pthread_mutex_unlock(&mutex_lista_procesos_ready);
}

void lock_mutex_to_thread(uint32_t pid, uint32_t tid, char* nombre){
    pthread_mutex_lock(&mutex_lista_procesos_ready);
        t_pcb* pcb = active_process_find_by_pid(pid);
        add_tid_to_mutex(pcb->mutex_asociados, nombre, tid);
    pthread_mutex_unlock(&mutex_lista_procesos_ready);
}

void unlock_mutex_to_thread(uint32_t pid, char* nombre){
    pthread_mutex_lock(&mutex_lista_procesos_ready);
        t_pcb* pcb = active_process_find_by_pid(pid);
        left_tid_to_mutex(pcb->mutex_asociados, nombre);

        uint32_t tid = unblock_tid_by_mutex(pcb->mutex_asociados, nombre);
        add_tid_to_mutex(pcb->mutex_asociados, nombre, tid);
    pthread_mutex_unlock(&mutex_lista_procesos_ready);
}

void add_mutex_to_process(t_mutex* mutex, uint32_t pid){
    pthread_mutex_lock(&mutex_lista_procesos_ready);
        t_pcb* pcb = active_process_find_by_pid(pid);
        list_add(pcb->mutex_asociados, mutex);
    pthread_mutex_unlock(&mutex_lista_procesos_ready);
}

void block_tid_by_mutex(t_list* lista_de_mutex, char* nombre, uint32_t tid){
    t_mutex* mutex = find_mutex_by_name(lista_de_mutex, nombre);
    queue_push(mutex->cola_bloqueados, &tid);
}

uint32_t unblock_tid_by_mutex(t_list* lista_de_mutex, char* nombre){
    t_mutex* mutex = find_mutex_by_name(lista_de_mutex, nombre);
    uint32_t tid = *(uint32_t*)queue_pop(mutex->cola_bloqueados);

    log_debug(kernel_log, "## TID POP de la cola de bloqueados del mutex: %d", tid);

    return tid;
}

void add_tid_to_mutex(t_list* lista_de_mutex, char* nombre, uint32_t tid){
    t_mutex* mutex = find_mutex_by_name(lista_de_mutex, nombre);
    mutex->tid_tomado = tid;
    mutex->valor = 0;
}

void left_tid_to_mutex(t_list* lista_de_mutex, char* nombre){
    t_mutex* mutex = find_mutex_by_name(lista_de_mutex, nombre);
    mutex->tid_tomado = -1;
    mutex->valor = 1;
}

uint32_t mutex_value(t_list* lista_de_mutex, char* name){
    t_mutex* mutex = find_mutex_by_name(lista_de_mutex, name);
    return mutex->valor;
}

/*---------------------------- FUNCIONES EXECUTE ----------------------------*/

void* ejecutar_hilo(t_hilo_planificacion* hilo_a_ejecutar){
    pthread_mutex_lock(&mutex_hilo_exec);
        change_thread_state(hilo_a_ejecutar->pid_padre, hilo_a_ejecutar->tid_asociado, EXEC_STATE);
        
        t_paquete* paquete = crear_paquete(EJECUTAR_HILO);
        t_buffer* buffer_envio = buffer_create(
            2*(sizeof(uint32_t))
        );

        buffer_add_uint32(buffer_envio, &hilo_a_ejecutar->pid_padre, kernel_log);
        buffer_add_uint32(buffer_envio, &hilo_a_ejecutar->tid_asociado, kernel_log);

        paquete->buffer = buffer_envio;
        list_add(lista_hilo_en_ejecucion, hilo_a_ejecutar);
        enviar_paquete(paquete, fd_conexion_dispatch);

        log_debug(kernel_log, "## (%d:%d) Hilo a ejecutar en CPU", hilo_a_ejecutar->pid_padre, hilo_a_ejecutar->tid_asociado);
        eliminar_paquete(paquete);
    pthread_mutex_unlock(&mutex_hilo_exec);

    if(strcmp(kernel_registro.algoritmo_planificacion, "CMN") == 0){
        pthread_create(&hilo_quantum, NULL, (void*) iniciar_quantum, NULL);
        pthread_detach(hilo_quantum);
    }

    while (!hilo_desalojado)
    {
        operacion_a_atender();
    }
}

t_hilo_planificacion* desalojar_hilo(){
    pthread_mutex_lock(&mutex_hilo_exec);
        if (list_is_empty(lista_hilo_en_ejecucion))
        {
            ejecutar_hilo(obtener_hilo_segun_algoritmo(kernel_registro.algoritmo_planificacion));
        }
            t_hilo_planificacion* hilo = list_remove(lista_hilo_en_ejecucion, 0);
            hilo_desalojado = true;
            if ((strcmp(kernel_registro.algoritmo_planificacion, "CMN") == 0) && !hilo_desalojado_por_quantum)
            {
                pthread_cancel(hilo_quantum);
            }
    pthread_mutex_unlock(&mutex_hilo_exec);

    return hilo;
}

void* ejecutar_io(void* ptr){
    pthread_mutex_lock(&mutex_io);
        t_hilo_planificacion* hilo;
        hilo_args_io* args = (hilo_args_io*)ptr;

        usleep(args->milisegundos * 1000);

        hilo = remover_de_block(args->pid, args->tid);
        poner_en_ready(hilo);

        log_info(kernel_log,"## (<%d>:<%d>) finalizó IO y pasa a READY", args->pid, args->tid);
    pthread_mutex_unlock(&mutex_io);
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
        eliminar_paquete(paquete);

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
        eliminar_paquete(paquete);

        char* response_memoria = recibir_mensaje(socket_memoria, kernel_log);
        log_debug(kernel_log, "STRING RECIBIDO avisar_creacion_hilo_memoria: %s", response_memoria);
        close(socket_memoria);
    pthread_mutex_unlock(&mutex_uso_fd_memoria);

    return response_memoria;
}

char* avisar_fin_proceso_memoria(uint32_t* pid){
    t_paquete* paquete = crear_paquete(FINALIZAR_PROCESO);
    t_buffer* buffer = buffer_create(
        sizeof(int)
    );

    buffer_add_uint32(buffer, (uint32_t*)pid, kernel_log);

    paquete->buffer = buffer;

    pthread_mutex_lock(&mutex_uso_fd_memoria);
        int socket_memoria = crear_conexion_con_memoria(kernel_log, kernel_registro.ip_memoria, kernel_registro.puerto_memoria);
        enviar_paquete(paquete, socket_memoria);
        eliminar_paquete(paquete);

        char* response_memoria = recibir_mensaje(socket_memoria, kernel_log);
        log_debug(kernel_log, "STRING RECIBIDO avisar_fin_proceso_memoria: %s", response_memoria);
        close(socket_memoria);
    pthread_mutex_unlock(&mutex_uso_fd_memoria);

    return response_memoria;
}

char* avisar_fin_hilo_memoria(uint32_t* pid, uint32_t* tid){
    t_paquete* paquete = crear_paquete(FINALIZAR_HILO);
    t_buffer* buffer = buffer_create(
        2*sizeof(int)
    );

    buffer_add_uint32(buffer, pid, kernel_log);
    buffer_add_uint32(buffer, tid, kernel_log);

    paquete->buffer = buffer;

    pthread_mutex_lock(&mutex_uso_fd_memoria);
        int socket_memoria = crear_conexion_con_memoria(kernel_log, kernel_registro.ip_memoria, kernel_registro.puerto_memoria);
        enviar_paquete(paquete, socket_memoria);
        char* response_memoria = recibir_mensaje(socket_memoria, kernel_log);
        log_debug(kernel_log, "STRING RECIBIDO avisar_fin_hilo_memoria: %s", response_memoria);
        close(socket_memoria);
    pthread_mutex_unlock(&mutex_uso_fd_memoria);

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
    pthread_mutex_lock(&mutex_interrupt);
	    enviar_paquete(paquete, fd_conexion_interrupt);
    pthread_mutex_unlock(&mutex_interrupt);
	eliminar_paquete(paquete);
}

/*--------------------------------- SYSCALLS --------------------------------*/

void* operacion_a_atender(){
    pthread_mutex_lock(&mutex_interrupt);
        int operacion = recibir_operacion(fd_conexion_interrupt);
        t_buffer* buffer = buffer_recieve(fd_conexion_interrupt);
    pthread_mutex_unlock(&mutex_interrupt);
    t_pid_tid pid_tid_recibido = recibir_pid_tid(buffer, kernel_log);
    
    switch (operacion)
    {
    case PROCESS_CREATE:        
        syscall_process_create(buffer, pid_tid_recibido);
        break;

    case PROCESS_EXIT:
        syscall_process_exit(pid_tid_recibido);
        break;

    case THREAD_CREATE:
        syscall_thread_create(buffer, pid_tid_recibido);
        break;

    case THREAD_JOIN:
        syscall_thread_join(buffer, pid_tid_recibido);
        break;

    case THREAD_CANCEL:
        syscall_thread_cancel(buffer, pid_tid_recibido);
        break;

    case THREAD_EXIT:
        syscall_thread_exit(pid_tid_recibido);
        break;

    case MUTEX_CREATE:
        syscall_mutex_create(buffer, pid_tid_recibido);
        break;

    case MUTEX_LOCK:
        syscall_mutex_lock(buffer, pid_tid_recibido);
        break;
        
    case MUTEX_UNLOCK:
        syscall_mutex_unlock(buffer, pid_tid_recibido);
        break;

    case DUMP_MEMORY:
        syscall_dump_memory(pid_tid_recibido);
        break;

    case IO:
        syscall_io(buffer, pid_tid_recibido);
        break;

    default:
        log_warning(kernel_log, "## Error en la OP enviada desde CPU");
        break;
    }

    buffer_destroy(buffer);
}

void* syscall_process_create(t_buffer* buffer, t_pid_tid pid_tid){
    uint32_t largo_string = 0;
    inst_cpu codigo = PROCESS_CREATE;

    uint32_t tam_proceso = buffer_read_uint32(buffer);
    uint32_t prioridad = buffer_read_uint32(buffer);
    char* path = buffer_read_string(buffer, &largo_string);

    log_info(kernel_log,"## (%d:%d) - Solicitó syscall: PROCESS CREATE", pid_tid.pid, pid_tid.tid);

    t_pcb* pcb = create_pcb(path, tam_proceso);
    t_tcb* tcb_main = create_tcb(pcb, prioridad);
    list_add(pcb->lista_tcb, tcb_main);

    create_process_state(pcb->pid);
    poner_en_new(pcb);
    enviar_aviso_syscall("PROCESO INICIALIZADO", &codigo);
    log_info(kernel_log,"## PID: %d- Se crea el proceso- Estado: NEW", pcb->pid);
}   

void* syscall_process_exit(t_pid_tid pid_tid_recibido){
    inst_cpu codigo = PROCESS_EXIT;
    log_info(kernel_log,"## (%d:%d) - Solicitó syscall: PROCESS_EXIT", pid_tid_recibido.pid, pid_tid_recibido.tid);
    finalizar_hilos_de_proceso(pid_tid_recibido.pid, pid_tid_recibido.tid);
    char* respuesta_memoria = avisar_fin_proceso_memoria(&pid_tid_recibido.pid);

    if (strcmp(respuesta_memoria, "FINALIZACION_ACEPTADA") == 0)
    {
        enviar_aviso_syscall("PROCESO FINALIZADO", &codigo);
        log_info(kernel_log,"## [KERNEL:MEMORIA] Finalización del proceso PID: %d realizado satisfactoriamente en MEMORIA", pid_tid_recibido.pid);
    } else {
        log_error(kernel_log, "ERROR EN syscall_process_exit");
        abort();
    }
    log_info(kernel_log,"## Finaliza el proceso <%d>", pid_tid_recibido.pid);

    free(respuesta_memoria);
}

void* syscall_thread_create(t_buffer* buffer, t_pid_tid pid_tid){
    uint32_t largo_string = 0;
    inst_cpu codigo = THREAD_CREATE;

    uint32_t prioridad_hilo = buffer_read_uint32(buffer);
    char* path = buffer_read_string(buffer, &largo_string);

    log_info(kernel_log,"## (%d:%d) - Solicitó syscall: THREAD_CREATE", pid_tid.pid, pid_tid.tid);
    t_pcb* pcb_padre = active_process_find_by_pid(pid_tid.pid);

    t_tcb* tcb = create_tcb(pcb_padre, prioridad_hilo);
    char* respuesta_creacion_hilo = avisar_creacion_hilo_memoria(pcb_padre, &tcb->tid, path, &prioridad_hilo, kernel_log);

    if (strcmp(respuesta_creacion_hilo, "OK") == 0)
    {
        t_hilo_planificacion* hilo_a_crear = create_hilo_planificacion(pcb_padre, tcb);
        create_thread_state(pcb_padre->pid, tcb->tid, prioridad_hilo);
        poner_en_ready(hilo_a_crear);
        enviar_aviso_syscall("HILO_CREADO", &codigo);
    } else {
        log_debug(kernel_log, "Rompimos algo con syscall_thread_create");
        abort();
    }
    free(path);
    free(respuesta_creacion_hilo);
}

void* syscall_thread_join(t_buffer* buffer, t_pid_tid pid_tid){
    inst_cpu codigo = THREAD_JOIN;

    uint32_t hilo_bloqueante = buffer_read_uint32(buffer);
    log_info(kernel_log,"## (%d:%d) - Solicitó syscall: THREAD_JOIN - <TID: %d>", pid_tid.pid, pid_tid.tid, hilo_bloqueante);

    if (is_thread_exist(pid_tid.pid, hilo_bloqueante))
    {
        t_hilo_planificacion* hilo = desalojar_hilo();
        add_thread_state_tid_blocked(hilo->pid_padre, hilo->tid_asociado, hilo_bloqueante);
        poner_en_block(hilo);
        enviar_aviso_syscall("DESALOJO_EN_KERNEL", &codigo);
        log_info(kernel_log, "## (<%d>:<%d>) - Bloqueado por: <PTHREAD_JOIN>", pid_tid.pid, pid_tid.tid);
    } else {
        enviar_aviso_syscall("HILO_NO_BLOQUEADO", &codigo);
        log_debug(kernel_log, "## El hilo no existe o ya finalizó, continuo ejecución");
    }
}

void* syscall_thread_exit(t_pid_tid pid_tid){
    inst_cpu codigo = THREAD_EXIT;

    log_info(kernel_log,"## (%d:%d) - Solicitó syscall: THREAD_EXIT", pid_tid.pid, pid_tid.tid);

    poner_en_exit(pid_tid.pid, pid_tid.tid);
    enviar_aviso_syscall("DESALOJO_EN_KERNEL", &codigo);
}

void* syscall_thread_cancel(t_buffer* buffer, t_pid_tid pid_tid){
    inst_cpu codigo = THREAD_CANCEL;

    uint32_t hilo_a_finalizar = buffer_read_uint32(buffer);

    log_info(kernel_log,"## (%d:%d) - Solicitó syscall: THREAD_CANCEL - <TID: %d>", pid_tid.pid, pid_tid.tid, hilo_a_finalizar);
    t_pcb* pcb = active_process_find_by_pid(pid_tid.pid);

    if (is_thread_exist(pid_tid.pid, hilo_a_finalizar))
    {
        poner_en_exit(pid_tid.pid, hilo_a_finalizar);
        if (hilo_a_finalizar == pid_tid.tid)
        {
            enviar_aviso_syscall("DESALOJO_EN_KERNEL", &codigo);
        } else {
            enviar_aviso_syscall("HILO_CANCELADO", &codigo);
        }
    } else {
        enviar_aviso_syscall("HILO_NO_CANCELADO", &codigo);
        log_debug(kernel_log, "## El hilo no existe o ya finalizó, continuo ejecución");
    }
}

void* syscall_io(t_buffer* buffer, t_pid_tid pid_tid){
    inst_cpu codigo = IO;

    uint32_t milisegundos_de_trabajo = buffer_read_uint32(buffer);
    log_info(kernel_log,"## (%d:%d) - Solicitó syscall: IO - <Milisegundos: %d>", pid_tid.pid, pid_tid.tid, milisegundos_de_trabajo);

    t_hilo_planificacion* hilo = desalojar_hilo();
    poner_en_block(hilo);
    enviar_aviso_syscall("DESALOJO_EN_KERNEL", &codigo);
    log_info(kernel_log, "## (<%d>:<%d>) - Bloqueado por: <IO>", pid_tid.pid, pid_tid.tid);

    hilo_args_io* args = malloc(sizeof(hilo_args_io));
    args->milisegundos = milisegundos_de_trabajo;
    args->pid = pid_tid.pid;
    args->tid = pid_tid.tid;

    pthread_create(&hilo_io, NULL, ejecutar_io, args);
    pthread_detach(hilo_io);
}

void* syscall_mutex_create(t_buffer* buffer, t_pid_tid pid_tid){
    inst_cpu codigo = MUTEX_CREATE;

    uint32_t length = 0;
    char* nombre_mutex = buffer_read_string(buffer, &length);
    enviar_aviso_syscall("MUTEX_CREADO", &codigo);

    log_info(kernel_log,"## (%d:%d) - Solicitó syscall: MUTEX_CREATE - <Nombre: %s>", pid_tid.pid, pid_tid.tid, nombre_mutex);

    t_mutex* mutexCreado = create_mutex(nombre_mutex);
    add_mutex_to_process(mutexCreado, pid_tid.pid);
}

void* syscall_mutex_lock(t_buffer* buffer, t_pid_tid pid_tid){
    inst_cpu codigo = MUTEX_LOCK;

    uint32_t length = 0;
    char* nombre_mutex = buffer_read_string(buffer, &length);
    log_info(kernel_log,"## (%d:%d) - Solicitó syscall: MUTEX_LOCK - <Nombre: %s>", pid_tid.pid, pid_tid.tid, nombre_mutex);
    
    if (is_mutex_exist(pid_tid.pid, nombre_mutex))
    {
        if (is_mutex_taken(pid_tid.pid, nombre_mutex))
        {
            block_thread_mutex(pid_tid.pid, pid_tid.tid, nombre_mutex);
            t_hilo_planificacion* hilo = desalojar_hilo();
            poner_en_block(hilo);
            enviar_aviso_syscall("DESALOJO_EN_KERNEL", &codigo);
            log_info(kernel_log, "## (<%d>:<%d>) - Bloqueado por: <MUTEX>", pid_tid.pid, pid_tid.tid);
        } else {
            enviar_aviso_syscall("MUTEX_TOMADO", &codigo);
            lock_mutex_to_thread(pid_tid.pid, pid_tid.tid, nombre_mutex);
        }
    } else {
        enviar_aviso_syscall("DESALOJO_EN_KERNEL", &codigo);
        poner_en_exit(pid_tid.pid, pid_tid.tid);
    }
}

void* syscall_mutex_unlock(t_buffer* buffer, t_pid_tid pid_tid){
    inst_cpu codigo = MUTEX_UNLOCK;

    uint32_t length = 0;
    char* nombre_mutex = buffer_read_string(buffer, &length);
    log_info(kernel_log,"## (%d:%d) - Solicitó syscall: MUTEX_UNLOCK - <Nombre: %s>", pid_tid.pid, pid_tid.tid, nombre_mutex);

    if (is_mutex_exist(pid_tid.pid, nombre_mutex))
    {
        if (is_mutex_taken_by_tid(pid_tid.pid, pid_tid.tid, nombre_mutex))
        {
            unlock_mutex_to_thread(pid_tid.pid, nombre_mutex);
            t_hilo_planificacion* hilo = remover_de_block(pid_tid.pid, pid_tid.tid);
            poner_en_ready(hilo);
            enviar_aviso_syscall("HILO_DESBLOQUEADO", &codigo);
        } else {
            log_debug(kernel_log, "## El MUTEX no está asignado al hilo (%d,%d), continuo ejecución", pid_tid.pid, pid_tid.tid);
            enviar_aviso_syscall("HILO_NO_DESBLOQUEADO", &codigo);
        }   
    } else {
        enviar_aviso_syscall("DESALOJO_EN_KERNEL", &codigo);
        poner_en_exit(pid_tid.pid, pid_tid.tid);
    }
}

void* syscall_dump_memory(t_pid_tid pid_tid){
    inst_cpu codigo = DUMP_MEMORY;

    log_info(kernel_log,"(%d:%d) - Solicitó syscall: DUMP_MEMORY", pid_tid.pid, pid_tid.tid);
    
    t_paquete* paquete = crear_paquete(AVISO_DUMP_MEMORY);
    t_buffer* buffer = buffer_create(
        sizeof(uint32_t)*2
    );

    buffer_add_uint32(buffer, &pid_tid.pid, kernel_log);
    buffer_add_uint32(buffer, &pid_tid.tid, kernel_log);

    paquete->buffer = buffer;

    //ENVIA EL PEDIDO A MEMORIA PARA FINALIZAR EL PROCESO
    pthread_mutex_lock(&mutex_uso_fd_memoria);
        int conexion_memoria = crear_conexion_con_memoria(kernel_log, kernel_registro.ip_memoria, kernel_registro.puerto_memoria);
        enviar_paquete(paquete, conexion_memoria);
        eliminar_paquete(paquete);
        
        log_debug(kernel_log, "SE ENVIO AVISO DE DUMP_MEMORY");
        char* respuesta_memoria = recibir_mensaje(conexion_memoria, kernel_log);
        log_debug(kernel_log, "RESPUESTA DE MEMORIA POR MEMORY DUMP: %s", respuesta_memoria);
        close(conexion_memoria);
    pthread_mutex_unlock(&mutex_uso_fd_memoria);

    if(strcmp(respuesta_memoria, "OK") == 0){
        t_hilo_planificacion* hilo_a_desbloquear = remover_de_block(pid_tid.pid, pid_tid.tid);
        poner_en_ready(hilo_a_desbloquear);
        enviar_aviso_syscall("DUMP_MEMORY", &codigo);
    }else{
        enviar_aviso_syscall("DESALOJO_EN_KERNEL", &codigo);
        poner_en_exit(pid_tid.pid, pid_tid.tid);
    }
}

/*--------------------------- FINALIZACIÓN DE TADS --------------------------*/

void t_hilo_planificacion_destroy(t_hilo_planificacion* hilo) {
    free(hilo);
}

void pcb_destroy(t_pcb* pcb){
    free(pcb->path_instrucciones_hilo_main);
    list_destroy_and_destroy_elements(pcb->lista_tcb, free);
    list_destroy_and_destroy_elements(pcb->mutex_asociados, process_mutex_destroy);
    free(pcb);
}

void process_mutex_destroy(t_mutex* mutex){
    queue_destroy(mutex->cola_bloqueados);
    free(mutex->nombre);
    free(mutex);
}

void destroy_process_state(){
    dictionary_destroy_and_destroy_elements(thread_states, destroy_thread_state);
}

void destroy_thread_state(t_list* lista_estados){
    list_destroy_and_destroy_elements(lista_estados, free);
}

/*------------------------- FINALIZACIÓN DEL MODULO -------------------------*/

void finalizar_modulo(){
    eliminar_logger(kernel_log);
    eliminar_config(kernel_config);
    eliminar_listas();
    eliminar_colas();
    destroy_process_state();
    finalizar_semaforos();
}

void finalizar_semaforos(){
    pthread_mutex_destroy(&mutex_cola_new);
    pthread_mutex_destroy(&mutex_cola_ready);
    pthread_mutex_destroy(&mutex_cola_block);
    pthread_mutex_destroy(&mutex_hilo_exec);
    pthread_mutex_destroy(&mutex_uso_fd_memoria);
    pthread_mutex_destroy(&mutex_uso_estado_hilos);
    pthread_mutex_destroy(&mutex_lista_procesos_ready);
    pthread_mutex_destroy(&mutex_colas_multinivel_existentes);
    pthread_mutex_destroy(&mutex_siguiente_id);
    pthread_mutex_destroy(&mutex_io);
    pthread_mutex_destroy(&mutex_interrupt);

    sem_destroy(&contador_procesos_en_new);
    sem_destroy(&aviso_exit_proceso);
    sem_destroy(&kernel_activo);
}

void eliminar_listas(){
    list_destroy(procesos_creados);
    list_destroy(cola_ready);
    list_destroy(lista_t_hilos_bloqueados);
    list_destroy(lista_colas_multinivel);
    list_destroy(lista_prioridades);
    list_destroy(lista_hilo_en_ejecucion);
}

void eliminar_colas(){
    queue_destroy_and_destroy_elements(cola_new, pcb_destroy);
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

    if(!hilo_desalojado){
        char* mensaje = "FIN_QUANTUM";
        int length = strlen(mensaje) + 1;
        t_buffer* buffer = buffer_create(
            length
        );
        t_paquete* paquete = crear_paquete(INTERRUPCION_QUANTUM);

        buffer_add_string(buffer, length, mensaje, kernel_log);
        paquete->buffer = buffer;

        pthread_mutex_lock(&mutex_interrupt);
            enviar_paquete(paquete, fd_conexion_interrupt);
        pthread_mutex_unlock(&mutex_interrupt);
        eliminar_paquete(paquete);

        hilo_desalojado_por_quantum = true;
        t_hilo_planificacion* hilo_desalojado = desalojar_hilo();
        poner_en_ready(hilo_desalojado);
    }
}


/*------------------------------------ WORKING ZONE ------------------------------------*/


/*------------------------------------ WORKING ZONE ------------------------------------*/
