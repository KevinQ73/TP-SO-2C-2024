#include <memoria.h>

int main(int argc, char* argv[]) {

    //---------------------------- Iniciar archivos ----------------------------

    memoria_log = iniciar_logger("./files/memoria.log", "MEMORIA", 1, LOG_LEVEL_DEBUG);

    //memoria_log = iniciar_logger("./files/memoria_obligatorio.log", "MEMORIA", 1, LOG_LEVEL_INFO);

    memoria_config = iniciar_config(argv[1]);

    memoria_registro = levantar_datos_memoria(memoria_config);

    // ------------------------ Iniciar servidor con CPU y Kernel------------------------

    iniciar_memoria();

    fd_conexiones = iniciar_servidor(memoria_log, "MEMORIA", memoria_registro.puerto_escucha);
    log_debug(memoria_log,"SOCKET LISTEN LISTO PARA RECIBIR A CPU");

    // --------------------- Conexión como cliente de FILESYSTEM ----------------------

    conexion_filesystem = crear_conexion(memoria_log, memoria_registro.ip_filesystem, memoria_registro.puerto_filesystem);
    log_debug(memoria_log, "ME CONECTÉ A FILESYSTEM");

    // --------------------- Conexiones de clientes con el servidor ----------------------
    
    log_debug(memoria_log, "Esperando CPU...");
    fd_conexion_cpu = esperar_cliente(memoria_log, "CPU", fd_conexiones);

    atender_solicitudes();

    sem_wait(&memoria_activo);

    // --------------------- Arrancan funciones de memoria ----------------------

    //FINALIZACION DEL MODULO

    log_debug(memoria_log, "TERMINANDO MEMORIA");
    //terminar_modulo(conexion_cpu, memoria_log, memoria_config);
    //terminar_modulo(conexion_kernel, memoria_log, memoria_config);
    //terminar_modulo(conexion_filesystem, memoria_log, memoria_config);

}

/*----------------------- FUNCIONES DE INICIALIZACIÓN -----------------------*/

void iniciar_memoria(){
    log_info(memoria_log, "MEMORIA INICIALIZADA\n");
    memoria = malloc(memoria_registro.tam_memoria); //Lo puse como variable global

    contextos_de_ejecucion = dictionary_create();
    lista_particiones = memoria_registro.particiones;

    int size_lista_particiones = string_array_size(lista_particiones);
    char* bitmap_espacio_reservado[size_lista_particiones];

    for (int i = 0; i < size_lista_particiones; i++){
        bitmap_espacio_reservado[i] = 0;
    }
    
    bitmap_particion_fija = bitarray_create_with_mode(bitmap_espacio_reservado, size_lista_particiones, LSB_FIRST);

    // Para particiones dinamicas
    lista_procesos_activos = dictionary_create();
    lista_huecos_disponibles = list_create();

    t_hueco* hueco_inicial = crear_hueco(0, memoria_registro.tam_memoria);
    agregar_hueco(hueco_inicial);

    iniciar_semaforos();
}

void iniciar_semaforos(){
    pthread_mutex_init(&kernel_operando, NULL);
    pthread_mutex_init(&contexto_ejecucion_procesos, NULL);
    pthread_mutex_init(&mutex_fd_filesystem, NULL);
    pthread_mutex_init(&mutex_huecos_disponibles, NULL);
    pthread_mutex_init(&mutex_procesos_activos, NULL);

    sem_init(&memoria_activo, 0, 0);
    sem_init(&actualizar_contexto, 0, 0);
}

void atender_solicitudes(){
    pthread_create(&hiloMemoriaCpu, NULL, atender_cpu, NULL);
    pthread_detach(hiloMemoriaCpu);

    pthread_create(&hiloMemoriaKernel, NULL, atender_kernel, NULL);
    pthread_detach(hiloMemoriaKernel);
}

/*---------------------------- FUNCIONES DE KERNEL --------------------------*/

void* atender_kernel(){
    bool liberar_hilo_kernel = true;

    while(liberar_hilo_kernel){

        log_info(memoria_log, "Esperando solicitud de KERNEL...\n");
        fd_conexion_kernel = esperar_cliente(memoria_log, "KERNEL", fd_conexiones);

        if (fd_conexion_kernel < 0) {
            // Llamar a la función de manejo de errores
            manejar_error_accept(errno, memoria_log);
            if (errno == EINTR || errno == ECONNABORTED) {
                continue;  // Reintentar la operación en caso de interrupción o conexión abortada
            } else {
                liberar_hilo_kernel = false;
                abort();
            }
        } else {
            log_info(memoria_log, "## Kernel Conectado - FD del socket: <%d>", fd_conexion_kernel);
            pthread_create(&hilo_atender_kernel, NULL, atender_solicitudes_kernel, (void*)fd_conexion_kernel);
            pthread_detach(hilo_atender_kernel);
        }
    }
}

void* atender_solicitudes_kernel(void* fd_conexion){
    int fd_memoria = (int*)fd_conexion;
    t_buffer* buffer;
    uint32_t pid;
    int size = 0;
    char* path = string_new();

    int operacion_kernel = recibir_operacion(fd_memoria);

    buffer = buffer_recieve(fd_memoria);

    switch (operacion_kernel)
    {
    case CREAR_PROCESO:
        pid = buffer_read_uint32(buffer);
        uint32_t size_process = buffer_read_uint32(buffer);

        if (crear_proceso(pid, size_process))
        {
            enviar_mensaje("OK", fd_memoria, memoria_log);
            log_info(memoria_log, "## [MEMORIA:KERNEL] Proceso <Creado> - PID: <%d> - Tamaño: <%d>. Cerrando FD del socket.\n", pid, size_process);
        } else {
            enviar_mensaje("ERROR_CREAR_PROCESO", fd_memoria, memoria_log);
            log_error(memoria_log, "## [MEMORIA:KERNEL] Fallo en la creación del proceso");
        }
        close(fd_memoria);
        break;

    case CREAR_HILO:
        path = buffer_read_string(buffer, &size);
        pid = buffer_read_uint32(buffer);
        uint32_t tid = buffer_read_uint32(buffer);
        uint32_t prioridad = buffer_read_uint32(buffer);

        crear_hilo(pid, tid, prioridad, path);

        enviar_mensaje("OK", fd_memoria, memoria_log);
        log_info(memoria_log, "## [MEMORIA:KERNEL] Hilo <Creado> - (PID:TID) - (<%d>:<%d>) PRIORIDAD: %d, Y PATH: %s. Cerrando FD del socket.\n", pid, tid, prioridad, path);

        close(fd_memoria);
        break;

    case FINALIZAR_PROCESO:
        pid = buffer_read_uint32(buffer);

        sem_wait(&actualizar_contexto);
        finalizar_proceso(pid);
        enviar_mensaje("FINALIZACION_ACEPTADA", fd_memoria, memoria_log);
        
        close(fd_memoria);    
        break;

    case FINALIZAR_HILO:
        pid = buffer_read_uint32(buffer);
        tid = buffer_read_uint32(buffer);

        sem_wait(&actualizar_contexto);
        finalizar_hilo(pid, tid);
        enviar_mensaje("OK", fd_memoria, memoria_log);
        
        close(fd_memoria);
        break;

    case AVISO_DUMP_MEMORY:
        pid = buffer_read_uint32(buffer);
        tid = buffer_read_uint32(buffer);

        char* timestamp = temporal_get_string_time("%H:%M:%S:%MS");
        char* nombre_archivo = string_new();

        t_contexto *contexto = buscar_contexto(pid, tid);
        void * contenido_proceso = leer_de_memoria(contexto->limite, contexto->base);
        string_append_with_format(&nombre_archivo, "<%s><%s><%s>.dmp",string_itoa(pid), string_itoa(tid), timestamp);

        t_paquete *archivo_fs = crear_paquete(DUMP_MEMORY);

        t_buffer* buffer = buffer_create(strlen(nombre_archivo)+2*sizeof(uint32_t)+strlen(contenido_proceso));

        buffer_add_string(buffer, strlen(nombre_archivo), nombre_archivo, memoria_log);
        buffer_add_string(buffer, strlen(contenido_proceso), contenido_proceso, memoria_log);

        archivo_fs->buffer = buffer;

        enviar_paquete(archivo_fs,conexion_filesystem);

        log_info(memoria_log, "## [MEMORIA:KERNEL] Memory Dump solicitado - (PID:TID) - (<%d>:<%d>)",pid,tid);

        char* response_fs = recibir_mensaje(conexion_filesystem, memoria_log);
        if(response_fs = "OK_FS"){
			enviar_mensaje("OK", fd_conexion_kernel, memoria_log);
		}else {
			enviar_mensaje("ERROR", fd_conexion_kernel, memoria_log);
		}
        free(nombre_archivo);
        free(contexto);
        close(fd_memoria);
        break;

    default:
        log_debug(memoria_log, "## [MEMORIA:KERNEL] OPERACIÓN DE KERNEL ERRONEA");
        break;
    }
    free(path);
    buffer_destroy(buffer);
}

bool crear_proceso(uint32_t pid, uint32_t size){
    bool memoria_asignada;
    
    int base_asignada = hay_particion_disponible(pid, size, memoria_registro.esquema);
    
    if (base_asignada != -1){
        char* key = string_itoa(pid);
        t_contexto_proceso* contexto_proceso = crear_contexto_proceso(pid, base_asignada, size);
        pthread_mutex_lock(&contexto_ejecucion_procesos);
            dictionary_put(contextos_de_ejecucion, key, contexto_proceso);
        pthread_mutex_unlock(&contexto_ejecucion_procesos);
        free(key);
        memoria_asignada = true;
    } else {
        log_error(memoria_log, "## [MEMORIA] OUT OF MEMORY");
        memoria_asignada = false;
    }
    return memoria_asignada;
}

void crear_hilo(uint32_t pid, uint32_t tid, uint32_t prioridad, char* path){
    char* key = string_itoa(pid);
    pthread_mutex_lock(&contexto_ejecucion_procesos);
    t_contexto_proceso* contexto_proceso = dictionary_get(contextos_de_ejecucion, key);
    t_contexto_hilo* contexto_hilo = crear_contexto_hilo(tid, prioridad, path);

    list_add(contexto_proceso->lista_hilos, contexto_hilo);

    dictionary_put(contextos_de_ejecucion, key, contexto_proceso);
    pthread_mutex_unlock(&contexto_ejecucion_procesos);

    free(key);
}

void* finalizar_proceso(uint32_t pid){

    liberar_espacio_en_memoria(pid);
    process_remove_and_destroy_by_pid(pid);
}

void* finalizar_hilo(uint32_t pid, uint32_t tid){
    pthread_mutex_lock(&contexto_ejecucion_procesos);
    t_contexto_proceso* contexto_padre = buscar_contexto_proceso(pid);
    t_contexto_hilo* hilo_a_finalizar = thread_remove_by_tid(contexto_padre->lista_hilos, tid);

    thread_context_destroy(hilo_a_finalizar);

    log_info(memoria_log, "## [MEMORIA:KERNEL] ## Hilo <Destruido> - (PID:TID) - (<%d>:<%d>). Cerrando FD del socket.\n", pid, tid);
    pthread_mutex_unlock(&contexto_ejecucion_procesos);
    return NULL;
}

/*----------------------------- FUNCIONES DE CPU ----------------------------*/

void* atender_cpu(){
    bool liberar_hilo_cpu = true;

    while(liberar_hilo_cpu){
        t_pid_tid pid_tid_recibido;
        t_buffer* buffer;

        uint32_t base;
        uint32_t desplazamiento;
        uint32_t valor_a_escribir;

        uint32_t direccion_fisica;
        uint32_t tamanio_contexto = 11*sizeof(uint32_t);
        t_contexto* contexto;

        int byte_inicial = 0;

        int op = recibir_operacion(fd_conexion_cpu);
        buffer = buffer_recieve(fd_conexion_cpu);

        switch (op){
        case SOLICITAR_CONTEXTO_EJECUCION: //Aca me solicita el contexto de un pid_tid, hay que ver que enum usamos
        	pid_tid_recibido.pid = buffer_read_uint32(buffer);
        	pid_tid_recibido.tid = buffer_read_uint32(buffer);
            
           	usleep(memoria_registro.retardo_respuesta * 1000);

            log_info(memoria_log, "## [MEMORIA:CPU] Contexto <Solicitado> - (PID:TID) - (<%d>:<%d>)", pid_tid_recibido.pid, pid_tid_recibido.tid);
            contexto = buscar_contexto(pid_tid_recibido.pid, pid_tid_recibido.tid);
            enviar_contexto_solicitado(contexto);
            break;

        case ACTUALIZAR_CONTEXTO_EJECUCION: //Aca me pide actualizar el contexto de un pid_tid, hay que ver que enum usamos
        	pid_tid_recibido.pid = buffer_read_uint32(buffer);
        	pid_tid_recibido.tid = buffer_read_uint32(buffer);

            t_contexto* contexto_recibido = recibir_contexto(buffer);

            usleep(memoria_registro.retardo_respuesta * 1000);

            actualizar_contexto_ejecucion(contexto_recibido, pid_tid_recibido.pid, pid_tid_recibido.tid);
            sem_post(&actualizar_contexto);
            log_info(memoria_log, "## [MEMORIA:CPU] Contexto <Actualizado> - (PID:TID) - (<%d>:<%d>)", pid_tid_recibido.pid, pid_tid_recibido.tid);
            escribir_en_memoria(contextos_de_ejecucion, tamanio_contexto, direccion_fisica);
            enviar_mensaje("OK_CONTEXTO", fd_conexion_cpu, memoria_log);
            free(contexto_recibido);
            break;
            
        case PETICION_INSTRUCCION:
        	pid_tid_recibido.pid = buffer_read_uint32(buffer);
        	pid_tid_recibido.tid = buffer_read_uint32(buffer);
        	uint32_t programCounter = buffer_read_uint32(buffer);

        	usleep(memoria_registro.retardo_respuesta * 1000);

        	char* instruccion = buscar_instruccion(pid_tid_recibido.pid, pid_tid_recibido.tid, programCounter);
      	    log_info(memoria_log, "## Obtener instrucción - (PID:TID) - (<%d>:<%d>) - Instrucción: %s", pid_tid_recibido.pid, pid_tid_recibido.tid, instruccion);
            enviar_mensaje(instruccion, fd_conexion_cpu, memoria_log);
            //free(instruccion);
        	break;

        case WRITE_MEM:
            pid_tid_recibido.pid = buffer_read_uint32(buffer);
        	pid_tid_recibido.tid = buffer_read_uint32(buffer);

            base = buffer_read_uint32(buffer);
            desplazamiento = buffer_read_uint32(buffer);
            valor_a_escribir = buffer_read_uint32(buffer);

            byte_inicial = obtener_byte_inicial(base);

            direccion_fisica = byte_inicial + desplazamiento;

            usleep(memoria_registro.retardo_respuesta * 1000);
            log_info(memoria_log, "## [MEMORIA:CPU] <Escritura> - (PID:TID) - (<%d>:<%d>) - Dir. Física: <%d> - Tamaño: <4>", pid_tid_recibido.pid, pid_tid_recibido.tid, direccion_fisica);
            escribir_en_memoria(&valor_a_escribir, 4, direccion_fisica);
            enviar_mensaje("OK", fd_conexion_cpu, memoria_log);
            break;
            
        case READ_MEM:
            pid_tid_recibido.pid = buffer_read_uint32(buffer);
        	pid_tid_recibido.tid = buffer_read_uint32(buffer);

            base = buffer_read_uint32(buffer);
            desplazamiento = buffer_read_uint32(buffer);

            byte_inicial = obtener_byte_inicial(base);

            direccion_fisica = byte_inicial + desplazamiento;

            usleep(memoria_registro.retardo_respuesta * 1000);
            log_info(memoria_log, "## [MEMORIA:CPU] <Lectura> - (PID:TID) - (<%d>:<%d>) - Dir. Física: <%d> - Tamaño: <4>", pid_tid_recibido.pid, pid_tid_recibido.tid, direccion_fisica);

            enviar_datos_memoria(leer_de_memoria(4, direccion_fisica), 4);
            break;

        case DESCONEXION:
            log_error(memoria_log, "## [MEMORIA:CPU] Desconexion de Memoria-Cpu");
            liberar_hilo_cpu = false;
            break;

        default:
            log_warning(memoria_log, "## [MEMORIA:CPU] Operacion desconocida de Memoria-Cpu");
            liberar_hilo_cpu = false;
            break;
        }
        buffer_destroy(buffer);
    }
}

int obtener_byte_inicial(int base){
    int valor_byte_inicial = 0;
    
    if (strcmp(memoria_registro.esquema, "FIJAS") == 0)
    {
        for (int i = 0; i < base; i++)
        {
            valor_byte_inicial = valor_byte_inicial + atoi(lista_particiones[i]);
        }
    } else {

    }

    return valor_byte_inicial;
}


/*-------------------- FUNCIONES DE CONTEXTO DE EJECUCION -------------------*/

t_contexto_proceso* crear_contexto_proceso(uint32_t pid, uint32_t base, uint32_t limite){
    t_contexto_proceso* contexto = malloc(sizeof(t_contexto_proceso));

    contexto->pid = pid;
    contexto->base = base;
    contexto->limite = limite;
    contexto->lista_hilos = list_create();

    return contexto;
}

t_contexto_hilo* crear_contexto_hilo(uint32_t tid, uint32_t prioridad, char* path){
    t_contexto_hilo* contexto = malloc(sizeof(t_contexto_hilo));

    contexto->lista_instrucciones = leer_instrucciones(path, memoria_registro.path_instrucciones ,memoria_log);
    contexto->tid = tid;
    contexto->prioridad = prioridad;
    contexto->pc = 0;
    contexto->ax = 0;
    contexto->bx = 0;
    contexto->cx = 0;
    contexto->dx = 0;
    contexto->ex = 0;
    contexto->fx = 0;
    contexto->gx = 0;
    contexto->hx = 0;

    return contexto;
}

t_contexto* buscar_contexto(uint32_t pid, uint32_t tid){
    t_contexto* contexto_a_enviar = malloc(sizeof(t_contexto));
    t_contexto_proceso* contexto_proceso;
    t_contexto_hilo* contexto_hilo;

    char* key = string_itoa(pid);
    pthread_mutex_lock(&contexto_ejecucion_procesos);
        contexto_proceso = dictionary_get(contextos_de_ejecucion, key);
        contexto_hilo = thread_get_by_tid(contexto_proceso->lista_hilos, tid);

        contexto_a_enviar->base = contexto_proceso->base;
        contexto_a_enviar->limite = contexto_proceso->limite;
        contexto_a_enviar->pc = contexto_hilo->pc;
        contexto_a_enviar->ax = contexto_hilo->ax;
        contexto_a_enviar->bx = contexto_hilo->bx;
        contexto_a_enviar->cx = contexto_hilo->cx;
        contexto_a_enviar->dx = contexto_hilo->dx;
        contexto_a_enviar->ex = contexto_hilo->ex;
        contexto_a_enviar->fx = contexto_hilo->fx;
        contexto_a_enviar->gx = contexto_hilo->gx;
        contexto_a_enviar->hx = contexto_hilo->hx;
    pthread_mutex_unlock(&contexto_ejecucion_procesos);
    free(key);
    return contexto_a_enviar;
}

t_contexto_proceso* buscar_contexto_proceso(uint32_t pid){
    t_contexto_proceso* contexto_proceso;

    char* key = string_itoa(pid);
    contexto_proceso = dictionary_get(contextos_de_ejecucion, key);
    free(key);
    return contexto_proceso;
}

t_contexto_hilo* buscar_contexto_hilo(t_contexto_proceso* contexto_proceso, uint32_t tid){
    t_contexto_hilo* contexto_hilo;
    contexto_hilo = thread_get_by_tid(contexto_proceso->lista_hilos, tid);
    return contexto_hilo;
}

t_contexto* recibir_contexto(t_buffer* buffer){
    t_contexto* contexto = malloc(sizeof(t_contexto));

    contexto->base = buffer_read_uint32(buffer);
    log_debug(memoria_log, "BASE RECIBIDO: %d", contexto->base);
    contexto->limite = buffer_read_uint32(buffer);
    log_debug(memoria_log, "LIMITE RECIBIDO: %d", contexto->limite);
    contexto->pc = buffer_read_uint32(buffer);
    log_debug(memoria_log, "PC RECIBIDO: %d", contexto->pc);
    contexto->ax = buffer_read_uint32(buffer);
    log_debug(memoria_log, "AX RECIBIDO: %d", contexto->ax);
    contexto->bx = buffer_read_uint32(buffer);
    log_debug(memoria_log, "BX RECIBIDO: %d", contexto->bx);
    contexto->cx = buffer_read_uint32(buffer);
    log_debug(memoria_log, "CX RECIBIDO: %d", contexto->cx);
    contexto->dx = buffer_read_uint32(buffer);
    log_debug(memoria_log, "DX RECIBIDO: %d", contexto->dx);
    contexto->ex = buffer_read_uint32(buffer);
    log_debug(memoria_log, "EX RECIBIDO: %d", contexto->ex);
    contexto->fx = buffer_read_uint32(buffer);
    log_debug(memoria_log, "FX RECIBIDO: %d", contexto->fx);
    contexto->gx = buffer_read_uint32(buffer);
    log_debug(memoria_log, "GX RECIBIDO: %d", contexto->gx);
    contexto->hx = buffer_read_uint32(buffer);
    log_debug(memoria_log, "HX RECIBIDO: %d", contexto->hx);

    return contexto;
}

t_contexto_hilo* thread_get_by_tid(t_list* lista_hilos, int tid){
    bool _list_contains(void* ptr) {
	    t_contexto_hilo* hilo = (t_contexto_hilo*) ptr;
	    return (tid == hilo->tid);
	}
	return list_find(lista_hilos, _list_contains);
}

t_contexto_hilo* thread_remove_by_tid(t_list* lista_hilos, int tid){
    bool _list_contains(void* ptr) {
	    t_contexto_hilo* hilo = (t_contexto_hilo*) ptr;
	    return (tid == hilo->tid);
	}
	return list_remove_by_condition(lista_hilos, _list_contains);
}

t_contexto_proceso* process_remove_by_pid(int pid){
    char* key = string_itoa(pid);
    t_contexto_proceso* contexto = dictionary_remove(contextos_de_ejecucion, key);

    free(key);
    return contexto;
}

void process_remove_and_destroy_by_pid(int tid){
    char* key = string_itoa(tid);
    dictionary_remove_and_destroy(contextos_de_ejecucion, key, process_context_destroy);

    free(key);
}

void thread_context_destroy(t_contexto_hilo* contexto_hilo){
    list_destroy_and_destroy_elements(contexto_hilo->lista_instrucciones, free);
    free(contexto_hilo);

    log_debug(memoria_log, "SE DESTRUYÓ EL CONTEXTO DEL HILO");
}

void process_context_destroy(t_contexto_proceso* contexto_proceso){
    list_destroy_and_destroy_elements(contexto_proceso->lista_hilos, thread_context_destroy);
    free(contexto_proceso);

    log_debug(memoria_log, "SE DESTRUYÓ EL CONTEXTO DEL PROCESO");
}

/*-------------------------------- MISCELANEO -------------------------------*/

void enviar_contexto_solicitado(t_contexto* contexto){
    t_paquete* paquete = crear_paquete(ENVIAR_CONTEXTO_EJECUCION);
    t_buffer* buffer;

    buffer = buffer_create(
        sizeof(uint32_t)*11
    );

    buffer_add_uint32(buffer, &contexto->base, memoria_log);
    buffer_add_uint32(buffer, &contexto->limite, memoria_log);
    buffer_add_uint32(buffer, &contexto->pc, memoria_log);
    buffer_add_uint32(buffer, &contexto->ax, memoria_log);
    buffer_add_uint32(buffer, &contexto->bx, memoria_log);
    buffer_add_uint32(buffer, &contexto->cx, memoria_log);
    buffer_add_uint32(buffer, &contexto->dx, memoria_log);
    buffer_add_uint32(buffer, &contexto->ex, memoria_log);
    buffer_add_uint32(buffer, &contexto->fx, memoria_log);
    buffer_add_uint32(buffer, &contexto->gx, memoria_log);
    buffer_add_uint32(buffer, &contexto->hx, memoria_log);

    paquete->buffer = buffer;

    enviar_paquete(paquete, fd_conexion_cpu);
    eliminar_paquete(paquete);
    free(contexto);
}

void* leer_de_memoria(uint32_t tamanio_lectura, uint32_t inicio_lectura){
    void* datos_memoria = malloc(tamanio_lectura);

    memcpy(datos_memoria, memoria + inicio_lectura, tamanio_lectura*sizeof(char));

    return datos_memoria;
}

void escribir_en_memoria(void* buffer_escritura, uint32_t tamanio_buffer, uint32_t inicio_escritura){
    memcpy(memoria + inicio_escritura, buffer_escritura, tamanio_buffer*sizeof(char));
}

char** buscar_instruccion(uint32_t pid, uint32_t tid, uint32_t program_counter){
    char* key = string_itoa(pid);
    t_contexto_proceso* contexto_proceso = dictionary_get(contextos_de_ejecucion, key);
    t_contexto_hilo* contexto_hilo = thread_get_by_tid(contexto_proceso->lista_hilos, tid);
    char* instruccion = string_new();

    if(list_size(contexto_hilo->lista_instrucciones) > program_counter){
        instruccion = list_get(contexto_hilo->lista_instrucciones, program_counter);
    } else {
        instruccion = "NO_INSTRUCCION";
    }

    //free(key);
    return instruccion;
}

bool busqueda_pid_tid(t_pseudocodigo* pseudocodigo){
    return (pseudocodigo->pid == pid_busqueda) && (pseudocodigo->tid == tid_busqueda);
}

void enviar_datos_memoria(void* buffer_recibido, uint32_t tamanio){
    t_paquete* paquete = crear_paquete(READ_MEM);
    t_buffer* buffer = buffer_create(4);

    buffer->stream = buffer_recibido;
    buffer->offset = 0;
    buffer->size = 4;

    paquete->buffer = buffer;
    enviar_paquete(paquete, fd_conexion_cpu);
    eliminar_paquete(paquete);
}

void actualizar_contexto_ejecucion(t_contexto* contexto_recibido, uint32_t pid, uint32_t tid){
    t_contexto_proceso* contexto_proceso;
    t_contexto_hilo* contexto_hilo;

    char* key = string_itoa(pid);
    pthread_mutex_lock(&contexto_ejecucion_procesos);
    contexto_proceso = dictionary_get(contextos_de_ejecucion, key);
    contexto_hilo = thread_get_by_tid(contexto_proceso->lista_hilos, tid);
    
    log_info(memoria_log, "## Registro BASE cambia valor %d a %d", contexto_proceso->base, contexto_recibido->base);
    contexto_proceso->base = contexto_recibido->base;
    log_info(memoria_log, "## Registro LIMITE cambia valor %d a %d", contexto_proceso->limite, contexto_recibido->limite);
    contexto_proceso->limite = contexto_recibido->limite;
    log_info(memoria_log, "## Registro PC cambia valor %d a %d", contexto_hilo->pc, contexto_recibido->pc);
    contexto_hilo->pc = contexto_recibido->pc;
    log_info(memoria_log, "## Registro AX cambia valor %d a %d", contexto_hilo->ax, contexto_recibido->ax);
    contexto_hilo->ax = contexto_recibido->ax;
    log_info(memoria_log, "## Registro BX cambia valor %d a %d", contexto_hilo->bx, contexto_recibido->bx);
    contexto_hilo->bx = contexto_recibido->bx;
    log_info(memoria_log, "## Registro CX cambia valor %d a %d", contexto_hilo->cx, contexto_recibido->cx);
    contexto_hilo->cx = contexto_recibido->cx;
    log_info(memoria_log, "## Registro DX cambia valor %d a %d", contexto_hilo->dx, contexto_recibido->dx);
    contexto_hilo->dx = contexto_recibido->dx;
    log_info(memoria_log, "## Registro EX cambia valor %d a %d", contexto_hilo->ex, contexto_recibido->ex);
    contexto_hilo->ex = contexto_recibido->ex;
    log_info(memoria_log, "## Registro FX cambia valor %d a %d", contexto_hilo->fx, contexto_recibido->fx);
    contexto_hilo->fx = contexto_recibido->fx;
    log_info(memoria_log, "## Registro GX cambia valor %d a %d", contexto_hilo->gx, contexto_recibido->gx);
    contexto_hilo->gx = contexto_recibido->gx;
    log_info(memoria_log, "## Registro HX cambia valor %d a %d", contexto_hilo->hx, contexto_recibido->hx);
    contexto_hilo->hx = contexto_recibido->hx;

    dictionary_put(contextos_de_ejecucion, key, contexto_proceso);
    pthread_mutex_unlock(&contexto_ejecucion_procesos);

    free(key);
}

/*------------------------------------ WORKING ZONE ------------------------------------*/

int hay_particion_disponible(uint32_t pid, uint32_t size, char* esquema){
    uint32_t valor_resultado;
    uint32_t size_particion;
    
    uint32_t valor_espacio_desocupado = obtener_espacio_desocupado();

    if (valor_espacio_desocupado == OUT_OF_MEMORY)
    {
        valor_resultado = -1;
    } else {
        size_particion = atoi(lista_particiones[valor_espacio_desocupado]);
    }
    
    if (strcmp(esquema, "FIJAS") == 0)
    {
        if (size > size_particion){
            valor_resultado = -1;
        } else {
            valor_resultado = valor_espacio_desocupado;
        }
    } else {
        if (particion_dinamica(pid, size))
        {
            valor_resultado = pid;
        } else {
            valor_resultado = -1;
        }
    }
    return valor_resultado;
}

bool particion_dinamica(uint32_t pid, uint32_t size){
    t_hueco* hueco = obtener_hueco(size);
    bool asignacion_realizada;

    if (hueco == NULL)
    {
        asignacion_realizada = false;
    } else {
        asignar_particion_dinamica(hueco, pid, size);
        asignacion_realizada = true;
    }
    return asignacion_realizada;
}

void asignar_particion_dinamica(t_hueco* hueco, uint32_t pid, uint32_t size){
    t_proceso* proceso = malloc(sizeof(t_proceso));

    proceso->pid = pid;
    proceso->base = pid + 1;
    proceso->inicio = hueco->inicio;
    proceso->size = size;
    
    agregar_proceso_activo(proceso);

    pthread_mutex_lock(&mutex_huecos_disponibles);
        hueco->inicio = hueco->inicio + size;
        hueco->size = hueco->inicio - size;
    pthread_mutex_unlock(&mutex_huecos_disponibles);

}

void agregar_proceso_activo(t_proceso* proceso){
    pthread_mutex_lock(&mutex_procesos_activos);
        char* key = string_itoa(proceso->pid);
        dictionary_put(lista_procesos_activos, key, proceso);
    pthread_mutex_unlock(&mutex_procesos_activos);

    free(key);
}

t_proceso* obtener_proceso_activo(uint32_t pid){
    pthread_mutex_lock(&mutex_procesos_activos);
        char* key = string_itoa(pid);
        t_proceso* proceso = dictionary_remove(lista_procesos_activos, key);
    pthread_mutex_unlock(&mutex_procesos_activos);

    return proceso;
}

uint32_t obtener_espacio_desocupado(){
    int base_particion = -1;

    uint32_t largo_bitmap = bitarray_get_max_bit(bitmap_particion_fija);
    uint32_t i = 0;
    while(i < largo_bitmap && (base_particion < 0)){
        if(bitarray_test_bit(bitmap_particion_fija, i) == 0){
            base_particion = i;
            bitarray_set_bit(bitmap_particion_fija, i);
        } else {
            i++;
        }
    }
    if (i == largo_bitmap){
        return OUT_OF_MEMORY;
    } else {
        return base_particion;
    }
}

t_hueco* crear_hueco(uint32_t inicio, uint32_t size){
    t_hueco* hueco_disponible = malloc(sizeof(t_hueco));

    hueco_disponible->inicio = inicio;
    hueco_disponible->size = size;

    return hueco_disponible;
}

void agregar_hueco(t_hueco* hueco){
    pthread_mutex_lock(&mutex_huecos_disponibles);
        list_add(lista_huecos_disponibles, hueco);
    pthread_mutex_unlock(&mutex_huecos_disponibles);
}

t_hueco* first_fit(uint32_t size){
    pthread_mutex_lock(&mutex_huecos_disponibles);
        bool _list_contains(void* ptr){
            t_hueco* hueco = (t_hueco*) ptr;
	        return (hueco->size >= size);
	    }
	    return list_find(lista_huecos_disponibles, _list_contains);
    pthread_mutex_unlock(&mutex_huecos_disponibles);
}

t_hueco* best_fit(uint32_t size){
    pthread_mutex_lock(&mutex_huecos_disponibles);
        void* _min_size_byte(void* a, void* b) {
	        t_hueco* hueco_a = (t_hueco*) a;
	        t_hueco* hueco_b = (t_hueco*) b;

            if (hueco_a->size <= hueco_b->size)
            {
                if (hueco_a->size >= size)
                {
                    return hueco_a;
                } else {
                    return hueco_b;
                }
            } else {
                if (hueco_b->size >= size)
                {
                    return hueco_b;
                } else {
                    return hueco_a;
                }
            }
	    }
	    t_hueco* hueco_hallado = list_get_minimum(lista_huecos_disponibles, _min_size_byte);

        if (hueco_hallado->size < size)
        {
            hueco_hallado = NULL;
        }
    pthread_mutex_unlock(&mutex_huecos_disponibles);
    return hueco_hallado;
}

t_hueco* worst_fit(uint32_t size){
    pthread_mutex_lock(&mutex_huecos_disponibles);
        void* _max_size_byte(void* a, void* b) {
	        t_hueco* hueco_a = (t_hueco*) a;
	        t_hueco* hueco_b = (t_hueco*) b;
            return hueco_a->size >= hueco_b->size ? hueco_a : hueco_b;
	    }
	    t_hueco* hueco_hallado = list_get_maximum(lista_huecos_disponibles, _max_size_byte);

        if (hueco_hallado->size < size)
        {
            hueco_hallado = NULL;
        }
    pthread_mutex_unlock(&mutex_huecos_disponibles);
    return hueco_hallado;
}

t_hueco* obtener_hueco(uint32_t size){
    t_hueco* hueco;

    if (strcmp(memoria_registro.algoritmo_busqueda, "FIRST") == 0)
    {
        hueco = first_fit(size);
    } else if (strcmp(memoria_registro.algoritmo_busqueda, "BEST") == 0)
    {
        hueco = best_fit(size);
    } else if (strcmp(memoria_registro.algoritmo_busqueda, "WORST") == 0)
    {
        hueco = worst_fit(size);
    } else {
        log_error(memoria_log, "ERROR EN obtener_hueco");
        abort();
    }
    
    return hueco;
}

void liberar_espacio_en_memoria(uint32_t pid){
    t_contexto_proceso* contexto_proceso = process_remove_by_pid(pid);
    if (strcmp(memoria_registro.esquema, "FIJAS") == 0)
    {
        int size_particion = get_size_partition(contexto_proceso->base);
        liberar_hueco_bitmap_fijas(contexto_proceso->base);
        log_info(memoria_log, "## [MEMORIA:KERNEL] Proceso <Destruido> - PID: <%d> -Tamaño: <%d>. Cerrando FD del socket.\n", pid, size_particion);
    } else {
        liberar_hueco_dinamico(contexto_proceso);
    }
}

int get_size_partition(uint32_t base){
    char* size = lista_particiones[base];

    int size_int = atoi(size);

    return size_int;
}

void liberar_hueco_bitmap_fijas(uint32_t base){
    bitarray_clean_bit(bitmap_particion_fija, base);
}

void liberar_hueco_dinamico(t_contexto_proceso* proceso){
    t_proceso* proceso_activo = obtener_proceso_activo(proceso->pid);

    if (byte_en_hueco((proceso_activo->inicio - 1)) && byte_en_hueco((proceso_activo->inicio + proceso_activo->size + 1)))
    {
        consolidar_huecos(proceso_activo->inicio, proceso_activo->size);
    } else {
        t_hueco* hueco = crear_hueco(proceso_activo->inicio, proceso_activo->size);
        agregar_hueco(hueco);
    }

    process_context_destroy(proceso);
}

void consolidar_huecos(uint32_t inicio, uint32_t size){
    t_hueco* hueco_izquierda = remover_hueco_que_contiene_byte(inicio -1);
    t_hueco* hueco_derecha = remover_hueco_que_contiene_byte(inicio + size + 1);

    t_hueco* hueco_consolidado = crear_hueco(hueco_izquierda->inicio, (hueco_derecha->inicio + hueco_derecha->size));
    agregar_hueco(hueco_consolidado);
}

bool byte_en_hueco(uint32_t byte){
    bool _list_contains(void* ptr){
        t_hueco* hueco = (t_hueco*) ptr;
	    return (hueco->inicio < byte < (hueco->inicio + hueco->size));
	}
	return list_any_satisfy(lista_huecos_disponibles, _list_contains);
}

t_hueco* remover_hueco_que_contiene_byte(uint32_t byte){
    bool _list_contains(void* ptr){
        t_hueco* hueco = (t_hueco*) ptr;
	    return (hueco->inicio < byte < (hueco->inicio + hueco->size));
	}
	return list_remove_by_condition(lista_huecos_disponibles, _list_contains);
}

void* memory_dump(){
    enviar_mensaje("OK", fd_conexion_kernel, memoria_log);
    return NULL;
}

/*------------------------------------ WORKING ZONE ------------------------------------*/
