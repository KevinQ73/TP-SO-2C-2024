#include <memoria.h>

int main(int argc, char* argv[]) {

    //---------------------------- Iniciar archivos ----------------------------

    //memoria_log = iniciar_logger("./files/memoria.log", "MEMORIA", 1, LOG_LEVEL_DEBUG);

    memoria_log = iniciar_logger("./files/memoria_obligatorio.log", "MEMORIA", 1, LOG_LEVEL_INFO);

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
    //Para particiones fijas
    int desplazamiento = 0; //Para usarlo como desplazamiento y saber la base de memoria
    particiones_memoria = list_create();
    for(int i=0;i< string_array_size(memoria_registro.particiones);i++){
        t_particion* particion = malloc(sizeof(t_particion));
        particion->base = desplazamiento;
        particion->limite = atoi(memoria_registro.particiones[i]);
        desplazamiento += atoi(memoria_registro.particiones[i]);

        list_add(particiones_memoria, particion);
    }
    //Para arriba

    bitmap_particion_fija = bitarray_create_with_mode(bitmap_espacio_reservado, size_lista_particiones, LSB_FIRST);
    
    // Para particiones dinamicas
    lista_procesos_activos = dictionary_create();
    lista_huecos_disponibles = list_create();

    t_hueco* hueco_inicial = crear_hueco(0, (memoria_registro.tam_memoria));
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
    for (int i = 0; i < bitmap_particion_fija->size; i++){
        bitarray_clean_bit(bitmap_particion_fija, i);
    }

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
    //char* path = string_new();

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
        char* path;
        path = buffer_read_string(buffer, &size);
        pid = buffer_read_uint32(buffer);
        uint32_t tid = buffer_read_uint32(buffer);
        uint32_t prioridad = buffer_read_uint32(buffer);

        crear_hilo(pid, tid, prioridad, path);

        enviar_mensaje("OK", fd_memoria, memoria_log);
        log_info(memoria_log, "## [MEMORIA:KERNEL] Hilo <Creado> - (PID:TID) - (<%d>:<%d>) PRIORIDAD: %d, Y PATH: %s. Cerrando FD del socket.\n", pid, tid, prioridad, path);

        free(path);
        close(fd_memoria);
        break;

    case FINALIZAR_PROCESO:
        pid = buffer_read_uint32(buffer);

        sem_wait(&actualizar_contexto);
        log_debug(memoria_log, "Voy a finalizar proceso (%i)", pid);
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

        t_proceso* proceso = get_process(pid);
        void* contenido_proceso = leer_de_memoria(proceso->size, proceso->inicio);

        char* pid_string = string_itoa(pid);
        char* tid_string = string_itoa(tid);

        string_append_with_format(&nombre_archivo, "<%s><%s><%s>.dmp", pid_string, tid_string, timestamp);

        int length_nombre_archivo = strlen(nombre_archivo) +1;

        t_paquete* archivo_fs = crear_paquete(DUMP_MEMORY);

        t_buffer* buffer = buffer_create(
            length_nombre_archivo + sizeof(uint32_t) + proceso->size
        );

        buffer_add_string(buffer, length_nombre_archivo, nombre_archivo, memoria_log);
        buffer_add_uint32(buffer, &proceso->size, memoria_log);
        buffer_add(buffer, contenido_proceso, proceso->size);

        archivo_fs->buffer = buffer;
        
        int fd_filesystem = crear_conexion_con_filesystem(memoria_log, memoria_registro.ip_filesystem, memoria_registro.puerto_filesystem);
        enviar_paquete(archivo_fs, fd_filesystem);
        eliminar_paquete(archivo_fs);

        log_info(memoria_log, "## [MEMORIA:KERNEL] Memory Dump solicitado - (PID:TID) - (<%d>:<%d>)",pid,tid);

        char* response_fs = recibir_mensaje(fd_filesystem, memoria_log);
        if(strcmp(response_fs, "OK_FS") == 0){
			enviar_mensaje("OK", fd_memoria, memoria_log);
		}else {
			enviar_mensaje("ERROR", fd_memoria, memoria_log);
		}
        close(fd_filesystem);
        free(response_fs);
        free(nombre_archivo);
        free(pid_string);
        free(tid_string);
        close(fd_memoria);
        break;

    default:
        log_debug(memoria_log, "## [MEMORIA:KERNEL] OPERACIÓN DE KERNEL ERRONEA");
        break;
    }
    //free(path);
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
        //free(contexto_proceso);
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

    //free(contexto_proceso);
    //free(contexto_hilo);
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
    pthread_mutex_unlock(&contexto_ejecucion_procesos);

    thread_context_destroy(hilo_a_finalizar);

    //free(contexto_padre);
    //free(hilo_a_finalizar);
    log_info(memoria_log, "## [MEMORIA:KERNEL] ## Hilo <Destruido> - (PID:TID) - (<%d>:<%d>). Cerrando FD del socket.\n", pid, tid);

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
        int valor_a_escribir;

        int direccion_fisica;
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
            //escribir_en_memoria(contexto_recibido, tamanio_contexto, contexto_recibido->base);
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
            log_debug(memoria_log, "VALOR A ESCRIBIR EN MEMORIA: %d", valor_a_escribir);
            byte_inicial = obtener_byte_inicial(base, pid_tid_recibido.pid);

            direccion_fisica = byte_inicial + desplazamiento;

            usleep(memoria_registro.retardo_respuesta * 1000);
            log_info(memoria_log, "## [MEMORIA:CPU] <Escritura> - (PID:TID) - (<%d>:<%d>) - Dir. Física: <%d> - Tamaño: <4>", pid_tid_recibido.pid, pid_tid_recibido.tid, direccion_fisica);
            escribir_en_memoria(&valor_a_escribir, 4, direccion_fisica);
            log_debug(memoria_log, "VALOR A ESCRIBIR EN MEMORIA PARTE 2: %d", &valor_a_escribir);
            enviar_mensaje("OK", fd_conexion_cpu, memoria_log);
            break;
            
        case READ_MEM:
            pid_tid_recibido.pid = buffer_read_uint32(buffer);
        	pid_tid_recibido.tid = buffer_read_uint32(buffer);

            base = buffer_read_uint32(buffer);
            desplazamiento = buffer_read_uint32(buffer);

            byte_inicial = obtener_byte_inicial(base, pid_tid_recibido.pid);

            direccion_fisica = byte_inicial + desplazamiento;

            usleep(memoria_registro.retardo_respuesta * 1000);
            log_info(memoria_log, "## [MEMORIA:CPU] <Lectura> - (PID:TID) - (<%d>:<%d>) - Dir. Física: <%d> - Tamaño: <4>", pid_tid_recibido.pid, pid_tid_recibido.tid, direccion_fisica);

            void* datos_leidos = leer_de_memoria(4, direccion_fisica);
            int valor_leido = *(int*)datos_leidos;
            log_debug(memoria_log, "VALOR LEÍDO memoria %d", valor_leido);

            enviar_datos_memoria(datos_leidos, 4);

            free(datos_leidos);
            break;

        case DESCONEXION:
            log_debug(memoria_log, "## [MEMORIA:CPU] Desconexion de Memoria-Cpu");
            liberar_hilo_cpu = false;
            sem_post(&memoria_activo);
            break;

        default:
            log_debug(memoria_log, "## [MEMORIA:CPU] Operacion desconocida de Memoria-Cpu");
            liberar_hilo_cpu = false;
            sem_post(&memoria_activo);
            break;
        }
        buffer_destroy(buffer);
    }
}

int obtener_byte_inicial(int base, uint32_t pid){
    int valor_byte_inicial = 0;
    
    if (strcmp(memoria_registro.esquema, "FIJAS") == 0)
    {
        for (int i = 0; i < base; i++)
        {
            valor_byte_inicial = valor_byte_inicial + atoi(lista_particiones[i]);
        }
        log_debug(memoria_log, "VALOR_BYTE_INICIAL_FIJAS: %d", valor_byte_inicial);
    } else {
        t_proceso* proceso = get_process(pid);
        valor_byte_inicial = proceso->inicio;

        log_debug(memoria_log, "VALOR_BYTE_INICIAL_DINAMICAS: %d", valor_byte_inicial);
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

    memcpy(datos_memoria, memoria + inicio_lectura, tamanio_lectura);

    return datos_memoria;
}

void escribir_en_memoria(void* buffer_escritura, uint32_t tamanio_buffer, uint32_t inicio_escritura){
    memcpy(memoria + inicio_escritura, buffer_escritura, tamanio_buffer);
}

char** buscar_instruccion(uint32_t pid, uint32_t tid, uint32_t program_counter){
    char* key = string_itoa(pid);
    t_contexto_proceso* contexto_proceso = dictionary_get(contextos_de_ejecucion, key);
    t_contexto_hilo* contexto_hilo = thread_get_by_tid(contexto_proceso->lista_hilos, tid);
    char* instruccion;

    if(list_size(contexto_hilo->lista_instrucciones) > program_counter){
        instruccion = list_get(contexto_hilo->lista_instrucciones, program_counter);
    } else {
        instruccion = "NO_INSTRUCCION";
    }
    free(key);
    return instruccion;
}

bool busqueda_pid_tid(t_pseudocodigo* pseudocodigo){
    return (pseudocodigo->pid == pid_busqueda) && (pseudocodigo->tid == tid_busqueda);
}

void enviar_datos_memoria(void* buffer_recibido, uint32_t tamanio){
    uint32_t size_void = 4;

    t_paquete* paquete = crear_paquete(READ_MEM);
    t_buffer* buffer = buffer_create(
        sizeof(uint32_t) + size_void
    );

    buffer_add_uint32(buffer, &size_void, memoria_log);
    buffer_add(buffer, buffer_recibido, size_void);

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
    // uint32_t valor_resultado;
    // uint32_t size_particion;
    
    // uint32_t valor_espacio_desocupado = obtener_espacio_desocupado();

    // if (valor_espacio_desocupado == OUT_OF_MEMORY)
    // {
    //     valor_resultado = -1;
    // } else {
    //     size_particion = atoi(lista_particiones[valor_espacio_desocupado]);
    // }

    // if (strcmp(esquema, "FIJAS") == 0)
    // {
    //     if (size > size_particion){
    //         valor_resultado = -1;
    //     } else {
    //         valor_resultado = valor_espacio_desocupado;
    //     }
    // } else {
    //     if (particion_dinamica(pid, size))
    //     {
    //         valor_resultado = pid;
    //     } else {
    //         valor_resultado = -1;
    //     }
    // }
    // return valor_resultado;

    if(strcmp(esquema, "FIJAS") == 0){
        return asignacion_fija(pid, size);
    } else {
        return particion_dinamica(pid, size);
    }
}

//--- De aca para abajo es particion fija
int asignacion_fija(int pid,int tamanio){
    int bloque_asignado = -1;
    if(strcmp(memoria_registro.algoritmo_busqueda,"FIRST") == 0){
        bloque_asignado = buscar_primer_bloque(tamanio);
    }
    if(strcmp(memoria_registro.algoritmo_busqueda,"NEXT") == 0){
        bloque_asignado = buscar_siguiente_bloque(tamanio);
    }
    if(strcmp(memoria_registro.algoritmo_busqueda,"BEST") == 0){
        bloque_asignado = buscar_mejor_bloque(tamanio);
    }
    if(strcmp(memoria_registro.algoritmo_busqueda,"WORST") == 0){
        bloque_asignado = buscar_peor_bloque(tamanio);
    }
    if(bloque_asignado != -1){
        bitarray_set_bit(bitmap_particion_fija, bloque_asignado);
    }
    log_debug(memoria_log, "SE LE VA A ASIGNAR LA PARTICION (%i)", bloque_asignado);
    return bloque_asignado;
}

int buscar_primer_bloque(int tamanio){
    int bloque_asignado = -1;
    int contador = 0;

    do{
        if(!bitarray_test_bit(bitmap_particion_fija, contador)){
            log_debug(memoria_log, "LA PARTICION ES LA  (%i)", contador);
            log_debug(memoria_log, "LA PARTICION LIBRE TIENE UN TAMANIO DE (%i)", atoi(memoria_registro.particiones[contador]));
            log_debug(memoria_log, "LA PARTICION NECESITA SER DE (%i)", tamanio);
            if(tamanio <= atoi(memoria_registro.particiones[contador])){
                log_debug(memoria_log, "LA PARTICION TIENE ESPACIO SUFICIENTE");
                bloque_asignado = contador;
            }
        }
        contador++;
    }while(bloque_asignado == -1 && contador < bitmap_particion_fija->size);
    

    return bloque_asignado;
}

int buscar_siguiente_bloque(int tamanio){
    int bloque_asignado = -1;
    int contador = 0;
    do{
        if(!bitarray_test_bit(bitmap_particion_fija, contador) && contador >= ultima_asignacion){
            if(tamanio <= atoi(memoria_registro.particiones[contador])){
                bloque_asignado = contador;
                ultima_asignacion = contador;
            }
        }
        contador++;
    }while(bloque_asignado == -1 && contador < bitmap_particion_fija->size);

    if(bloque_asignado == -1 && ultima_asignacion >= 0){
        bloque_asignado = buscar_primer_bloque(tamanio);
        if(bloque_asignado != -1){
            ultima_asignacion = bloque_asignado;
        }
    }

    return bloque_asignado;
}

int buscar_mejor_bloque(int tamanio){
    int bloque_asignado = -1;
    int contador = 0;
    int fragmentacion = 0;
    int mejor_posicion = -1;
    do{
        if(!bitarray_test_bit(bitmap_particion_fija, contador)){
            if(tamanio <= atoi(memoria_registro.particiones[contador])){
                if(mejor_posicion == -1){
                    fragmentacion = atoi(memoria_registro.particiones[contador])-tamanio;
                    mejor_posicion = contador;
                }else{
                    if((atoi(memoria_registro.particiones[contador])-tamanio) < fragmentacion){
                        mejor_posicion = contador;
                        fragmentacion = atoi(memoria_registro.particiones[contador])-tamanio;
                    }
                }
            }
        }
        contador++;
    }while(bloque_asignado == -1 && contador < bitmap_particion_fija->size);

    bloque_asignado = mejor_posicion;

    return bloque_asignado;
}

int buscar_peor_bloque(int tamanio){
    int bloque_asignado = -1;
    int contador = 0;
    int fragmentacion = 0;
    int peor_posicion = -1;
    do{
        if(!bitarray_test_bit(bitmap_particion_fija, contador)){
            if(tamanio <= atoi(memoria_registro.particiones[contador])){
                if(peor_posicion == -1){
                    fragmentacion = atoi(memoria_registro.particiones[contador])-tamanio;
                    peor_posicion = contador;
                }else{
                    if((atoi(memoria_registro.particiones[contador])-tamanio) > fragmentacion){
                        peor_posicion = contador;
                        fragmentacion = atoi(memoria_registro.particiones[contador])-tamanio;
                    }
                }
            }
        }
        contador++;
    }while(bloque_asignado == -1 && contador < bitmap_particion_fija->size);

    bloque_asignado = peor_posicion;

    return bloque_asignado;
}

//---- Para arriba es una idea de particiones fijas, funciona.


void imprimir_estado_huecos(){
    void* print_hueco(void* ptr) {
	    t_hueco* hueco = (t_hueco*) ptr;
	    log_debug(memoria_log, "## <HUECO> Inicio:%d - Size:%d\n", hueco->inicio, hueco->size);
	}
	list_iterate(lista_huecos_disponibles, print_hueco);
}

void imprimir_estado_procesos_activos(){
    void* print_proceso(char* key, void* ptr) {
	    t_proceso* proceso = (t_proceso*) ptr;
	    log_debug(memoria_log, "## <PROCESO KEY:%s> PID:%d - Base:%d - Inicio:%d - Size:%d\n", key, proceso->pid, proceso->base, proceso->inicio, proceso->size);
	}
	dictionary_iterator(lista_procesos_activos, print_proceso);
}


int particion_dinamica(uint32_t pid, uint32_t size){
    t_hueco* hueco = obtener_hueco(size);
    int asignacion_realizada;

    if (hueco == NULL)
    {
        asignacion_realizada = -1;
        log_warning(memoria_log, "No hay un hueco disponible");
    } else {
        log_debug(memoria_log, "## HUECO HALLADO (INICIO:%d - SIZE:%d)", hueco->inicio, hueco->size);
        asignacion_realizada = asignar_particion_dinamica(hueco, pid, size);

        imprimir_estado_huecos();
        imprimir_estado_procesos_activos();
    }
    return asignacion_realizada;
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


int asignar_particion_dinamica(t_hueco* hueco, uint32_t pid, uint32_t size){
    t_proceso* proceso = malloc(sizeof(t_proceso));

    proceso->pid = pid;
    proceso->base = pid + 1;
    proceso->inicio = hueco->inicio;
    proceso->size = size;
    
    agregar_proceso_activo(proceso);
    log_info(memoria_log, "## PID:%d Se agregó a la tabla de procesos activos", pid);

    particionar_hueco(hueco, proceso);

    return proceso->base;
}

void particionar_hueco(t_hueco* hueco_padre, t_proceso* proceso_activo){
        log_debug(memoria_log, "## HUECO PADRE PRE-PARTICIONADO: INICIO=%d, SIZE=%d", hueco_padre->inicio, hueco_padre->size);

        hueco_padre->inicio = hueco_padre->inicio + proceso_activo->size; 
        hueco_padre->size = hueco_padre->size - proceso_activo->size;

        log_debug(memoria_log, "## HUECO PADRE PARTICIONADO: INICIO=%d, SIZE=%d", hueco_padre->inicio, hueco_padre->size);

        //t_hueco* hueco_particionado = crear_hueco(proceso_activo->inicio, proceso_activo->size);
        //log_debug(memoria_log, "## NUEVO HUECO DE LA PARTICIÓN: INICIO=%d, SIZE=%d", hueco_particionado->inicio, hueco_particionado->size);
        
        agregar_hueco(hueco_padre);
        //agregar_hueco(hueco_particionado);
}

void agregar_proceso_activo(t_proceso* proceso){
    pthread_mutex_lock(&mutex_procesos_activos);
        char* key = string_itoa(proceso->pid);
        dictionary_put(lista_procesos_activos, key, proceso);

        log_debug(memoria_log, "## PID:%d KEY:%s BASE:%d INICIO:%d SIZE:%d Se agregó a la tabla de procesos activos", proceso->pid, key, proceso->base, proceso->inicio, proceso->size);
    pthread_mutex_unlock(&mutex_procesos_activos);

    free(key);
}

t_proceso* obtener_proceso_activo(uint32_t pid){
    pthread_mutex_lock(&mutex_procesos_activos);
        char* key = string_itoa(pid);
        t_proceso* proceso = dictionary_remove(lista_procesos_activos, key);
    pthread_mutex_unlock(&mutex_procesos_activos);

    free(key);
    return proceso;
}

t_proceso* get_process(uint32_t pid){
    pthread_mutex_lock(&mutex_procesos_activos);
        char* key = string_itoa(pid);
        t_proceso* proceso = dictionary_get(lista_procesos_activos, key);
    pthread_mutex_unlock(&mutex_procesos_activos);

    free(key);
    return proceso;
}

uint32_t obtener_espacio_desocupado(){
    int base_particion = -1;

    uint32_t largo_bitmap = bitmap_particion_fija->size;
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
    log_debug(memoria_log, "Se agregó a la lista el hueco de inicio: %d y size:%d", hueco->inicio, hueco->size);
}

t_hueco* first_fit(uint32_t size){
    pthread_mutex_lock(&mutex_huecos_disponibles);
        bool _list_contains(void* ptr){
            t_hueco* hueco = (t_hueco*) ptr;
	        return (hueco->size >= size);
	    }
	    return list_remove_by_condition(lista_huecos_disponibles, _list_contains);
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
	    t_hueco* hueco_a_comparar = list_get_minimum(lista_huecos_disponibles, _min_size_byte);

        if (hueco_a_comparar->size < size)
        {
            hueco_a_comparar = NULL;
        } else {
            list_remove_element(lista_huecos_disponibles, hueco_a_comparar);
        }
    pthread_mutex_unlock(&mutex_huecos_disponibles);
    return hueco_a_comparar;
}

t_hueco* worst_fit(uint32_t size){
    pthread_mutex_lock(&mutex_huecos_disponibles);
        void* _max_size_byte(void* a, void* b) {
	        t_hueco* hueco_a = (t_hueco*) a;
	        t_hueco* hueco_b = (t_hueco*) b;
            return hueco_a->size >= hueco_b->size ? hueco_a : hueco_b;
	    }
	    t_hueco* hueco_a_comparar = list_get_maximum(lista_huecos_disponibles, _max_size_byte);

        t_hueco* hueco_hallado;
        if (hueco_a_comparar->size < size)
        {
            hueco_hallado = NULL;
        } else {
            list_remove_element(lista_huecos_disponibles, hueco_a_comparar);
        }
    pthread_mutex_unlock(&mutex_huecos_disponibles);
    return hueco_hallado;
}

void liberar_espacio_en_memoria(uint32_t pid){
    t_contexto_proceso* contexto_proceso = process_remove_by_pid(pid);
    if (strcmp(memoria_registro.esquema, "FIJAS") == 0)
    {
        int size_particion = get_size_partition(contexto_proceso->base);
        liberar_hueco_bitmap_fijas(contexto_proceso->base);
        log_debug(memoria_log, "Voy a borrar la particion (%i)", contexto_proceso->base);
        perror("Voy a borrar??");
        log_info(memoria_log, "## [MEMORIA:KERNEL] Proceso <Destruido> - PID: <%d> -Tamaño: <%d>. Cerrando FD del socket.\n", pid, size_particion);
    } else {
        liberar_hueco_dinamico(contexto_proceso);
    }
    //destruir_contexto_proceso(contexto_proceso); // Libera la memoria asociada al contexto
}

void destruir_contexto_proceso(t_contexto_proceso* contexto) {
    if (contexto == NULL) {
        return;
    }
    if (!list_is_empty(contexto->lista_hilos)) {
        list_destroy_and_destroy_elements(contexto->lista_hilos, destroy_t_contexto_hilos); // Libera los elementos de la lista
    }
    free(contexto);
}

void destroy_t_contexto_hilos(t_contexto_hilo* hilo){
    list_destroy_and_destroy_elements(hilo->lista_instrucciones, (void*)free);
    free(hilo);
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

    // if (byte_en_hueco(proceso_activo->inicio - 1) == true && byte_en_hueco(proceso_activo->inicio + proceso_activo->size + 1) == true)
    // {
    //     log_debug(memoria_log, "TENGO ESPACIO PARA CONSOLIDAR");
    //     consolidar_huecos(proceso_activo->inicio, proceso_activo->size);
    // } else {
    //     t_hueco* hueco = crear_hueco(proceso_activo->inicio, proceso_activo->size);
    //     agregar_hueco(hueco);
    // }

    t_hueco* hueco_izquierda;
    t_hueco* hueco_derecha;
    t_hueco* hueco_consolidado;


    if (byte_en_hueco((proceso_activo->inicio - 1)) && !byte_en_hueco((proceso_activo->inicio + proceso_activo->size + 1)))
    {
        log_debug(memoria_log, "Tengo un hueco libre a izquierda");
        hueco_izquierda = remover_hueco_que_contiene_byte(proceso_activo->inicio -1);
        hueco_consolidado = consolidar_huecos(hueco_izquierda->inicio, (hueco_izquierda->size + proceso_activo->size));
        agregar_hueco(hueco_consolidado);
        free(hueco_izquierda);

    }else if(!byte_en_hueco((proceso_activo->inicio - 1)) && byte_en_hueco((proceso_activo->inicio + proceso_activo->size + 1))){
        log_debug(memoria_log, "Tengo un hueco libre a derecha");
        hueco_derecha = remover_hueco_que_contiene_byte(proceso_activo->inicio + proceso_activo->size + 1);
        hueco_consolidado = consolidar_huecos(proceso_activo->inicio, (proceso_activo->size + hueco_derecha->size));
        agregar_hueco(hueco_consolidado);
        free(hueco_derecha);

    }else if(byte_en_hueco((proceso_activo->inicio - 1)) && byte_en_hueco((proceso_activo->inicio + proceso_activo->size + 1))){
        log_debug(memoria_log, "Estoy entre 2 bloques libres");

        hueco_izquierda = remover_hueco_que_contiene_byte(proceso_activo->inicio -1);
        hueco_derecha = remover_hueco_que_contiene_byte(proceso_activo->inicio + proceso_activo->size + 1);

        hueco_consolidado = consolidar_huecos(hueco_izquierda->inicio, (hueco_izquierda->size + proceso_activo->size + hueco_derecha->size));
        agregar_hueco(hueco_consolidado);
        free(hueco_izquierda);
        free(hueco_derecha);
    }
    else {
        log_debug(memoria_log, "No Tengo un hueco libre a los lados");
        t_hueco* hueco = crear_hueco(proceso_activo->inicio, proceso_activo->size);
        agregar_hueco(hueco);
    }
    process_context_destroy(proceso);
}

t_hueco* consolidar_huecos(uint32_t inicio, uint32_t size){
    t_hueco* hueco_consolidado = crear_hueco(inicio, size);

    return hueco_consolidado;
    /*
    t_hueco* hueco_izquierda = remover_hueco_que_contiene_byte(inicio -1);
    t_hueco* hueco_derecha = remover_hueco_que_contiene_byte(inicio + size + 1);

    t_hueco* hueco_consolidado = crear_hueco(hueco_izquierda->inicio, (hueco_derecha->inicio + hueco_derecha->size));
    agregar_hueco(hueco_consolidado);
    */
}

bool byte_en_hueco(uint32_t byte){
    bool _list_contains(void* ptr){
        t_hueco* hueco = (t_hueco*) ptr;

	    return (hueco->inicio < byte && byte < (hueco->inicio + hueco->size));
	}
    
    
	return list_any_satisfy(lista_huecos_disponibles, _list_contains);
}

t_hueco* remover_hueco_que_contiene_byte(uint32_t byte){
    bool _list_contains(void* ptr){
        t_hueco* hueco = (t_hueco*) ptr;
	    return (hueco->inicio < byte && byte < (hueco->inicio + hueco->size));
	}
	return list_remove_by_condition(lista_huecos_disponibles, _list_contains);
}

void* memory_dump(){
    enviar_mensaje("OK", fd_conexion_kernel, memoria_log);
    return NULL;
}

int crear_conexion_con_filesystem(t_log* memoria_log, char* ip, char* puerto){
    int conexion_memoria = crear_conexion(memoria_log, ip, puerto);
    log_debug(memoria_log, "ME CONECTÉ A FILESYSTEM");

    return conexion_memoria;
}

/*------------------------------------ WORKING ZONE ------------------------------------*/
