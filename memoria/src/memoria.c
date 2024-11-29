#include <memoria.h>

int main(int argc, char* argv[]) {

    //---------------------------- Iniciar archivos ----------------------------

    memoria_log = iniciar_logger("./files/memoria.log", "MEMORIA", 1, LOG_LEVEL_DEBUG);

    memoria_config = iniciar_config(argv[1]);

    memoria_registro = levantar_datos_memoria(memoria_config);

    // ------------------------ Iniciar servidor con CPU y Kernel------------------------

    iniciar_memoria();

    fd_conexiones = iniciar_servidor(memoria_log, "MEMORIA", memoria_registro.puerto_escucha);
    log_debug(memoria_log,"SOCKET LISTEN LISTO PARA RECIBIR A CPU");

    // --------------------- Conexión como cliente de FILESYSTEM ----------------------

    conexion_filesystem = crear_conexion(memoria_log, memoria_registro.ip_filesystem, memoria_registro.puerto_filesystem);
    log_debug(memoria_log, "ME CONECTÉ A FILESYSTEM");

    //enviar_solicitud_fs();

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

void enviar_solicitud_fs(){

    for (int i = 0; i < 2; i++)
    {
        t_paquete* paquete = crear_paquete(DUMP_MEMORY);
        t_buffer* buffer = buffer_create(
            sizeof(uint32_t)*3
        );

        buffer_add_uint32(buffer, &i, memoria_log);
        buffer_add_uint32(buffer, &i, memoria_log);
        buffer_add_uint32(buffer, &i, memoria_log);

        paquete->buffer = buffer;
        pthread_mutex_lock(&mutex_fd_filesystem);
            int fd_conexion = crear_conexion_con_fs(memoria_log, memoria_registro.ip_filesystem, memoria_registro.puerto_filesystem);
            enviar_paquete(paquete, fd_conexion);
            char* response_fs = recibir_mensaje(fd_conexion, memoria_log);
            log_debug(memoria_log, "STRING RECIBIDO enviar_solicitud_fs: %s", response_fs);
            close(fd_conexion);
        pthread_mutex_unlock(&mutex_fd_filesystem);

        eliminar_paquete(paquete);
    }
}

void iniciar_semaforos(){
    pthread_mutex_init(&kernel_operando, NULL);
    pthread_mutex_init(&contexto_ejecucion_procesos, NULL);
    pthread_mutex_init(&mutex_fd_filesystem, NULL);
    pthread_mutex_init(&mutex_huecos_disponibles, NULL);
    pthread_mutex_init(&mutex_procesos_activos, NULL);

    sem_init(&memoria_activo, 0, 0);
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
            log_info(memoria_log, "## [MEMORIA:KERNEL] Proceso <Creado> - PID: <%d> - Tamaño: <%d>", pid, size_process);
        } else {
            enviar_mensaje("ERROR_CREAR_PROCESO", fd_memoria, memoria_log);
            log_error(memoria_log, "## [MEMORIA:KERNEL] Fallo en la creación del proceso");
        }
        log_info(memoria_log, "## Solicitud PROCESS_CREATE de Kernel finalizada. Cerrando FD del socket.\n");
        close(fd_memoria);
        break;

    case CREAR_HILO:
        path = buffer_read_string(buffer, &size);
        pid = buffer_read_uint32(buffer);
        uint32_t tid = buffer_read_uint32(buffer);
        uint32_t prioridad = buffer_read_uint32(buffer);

        crear_hilo(pid, tid, prioridad, path);

        enviar_mensaje("OK", fd_memoria, memoria_log);
        log_info(memoria_log, "## [MEMORIA:KERNEL] Hilo <Creado> - (PID:TID) - (<%d>:<%d>) PRIORIDAD: %d, Y PATH: %s", pid, tid, prioridad, path);

        free(path);
        log_info(memoria_log, "## Solicitud THREAD_CREATE de Kernel finalizada. Cerrando FD del socket.\n");
        close(fd_memoria);
        break;

    case FINALIZAR_PROCESO:
        pid = buffer_read_uint32(buffer);

        finalizar_proceso(pid);
        log_info(memoria_log, "## Solicitud PROCESS_EXIT de Kernel finalizada. Cerrando FD del socket.\n");
        break;

    case DUMP_MEMORY:
        pid = buffer_read_uint32(buffer);
        tid = buffer_read_uint32(buffer);
        size_process = buffer_read_uint32(buffer);

        char* timestamp = temporal_get_string_time("%H:%M:%S:%MS");
        char* nombre_archivo = string_new();

        char* nombre_pid = string_itoa(pid);
        char* nombre_tid = string_itoa(tid);
        char* nombre_size = string_itoa(size_process);

        t_contexto *contexto = buscar_contexto(pid, tid);
        void * contenido_proceso = leer_de_memoria(contexto->limite, contexto->base);

        string_append_with_format(&nombre_archivo, "%<",nombre_pid,"%>","%<", nombre_tid,"%>","%<", timestamp,">",".dmp");

        t_paquete *archivo_fs = crear_paquete(DUMP_MEMORY);
        
        t_buffer* buffer = buffer_create(sizeof(nombre_archivo)+sizeof(uint32_t)+sizeof(contenido_proceso));

        buffer_add_string(buffer, sizeof(nombre_archivo), nombre_archivo, memoria_log);
        buffer_add_uint32(buffer, size_process, memoria_log);
         buffer_add_string(buffer, sizeof(contenido_proceso), contenido_proceso, memoria_log);

        archivo_fs->buffer = buffer;

        enviar_paquete(archivo_fs,memoria_registro.puerto_filesystem);

        log_info(memoria_log, "## Memory Dump solicitado - (PID:TID) - (<%d>:<%d>)",pid,tid);

        char* response_fs = recibir_mensaje(fd_conexion, memoria_log);
        if(response_fs = "OK_FS"){
			enviar_mensaje("OK", fd_conexion_kernel, memoria_log);
		}else {
			enviar_mensaje("ERROR", fd_conexion_kernel, memoria_log);
		}
        free(nombre_archivo);
        break;

    case FINALIZAR_HILO:
        pid = buffer_read_uint32(buffer);
        tid = buffer_read_uint32(buffer);
    default:
        log_debug(memoria_log, "## [MEMORIA:KERNEL] OPERACIÓN DE KERNEL ERRONEA");
        break;
    }
}

bool crear_proceso(uint32_t pid, uint32_t size){
    bool memoria_asignada;
    
    uint32_t base_asignada = hay_particion_disponible(pid, size, memoria_registro.esquema);
    
    if (base_asignada){
        char* key = string_itoa(pid);
        t_contexto_proceso* contexto_proceso = crear_contexto_proceso(pid, base_asignada, size);
        pthread_mutex_lock(&contexto_ejecucion_procesos);
            dictionary_put(contexto_ejecucion, key, contexto_proceso);
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
    t_contexto_proceso* contexto_proceso = dictionary_get(contexto_ejecucion, key);
    t_contexto_hilo* contexto_hilo = crear_contexto_hilo(tid, prioridad, path);

    list_add(contexto_proceso->lista_hilos, contexto_hilo);

    dictionary_put(contexto_ejecucion, key, contexto_proceso);
    pthread_mutex_unlock(&contexto_ejecucion_procesos);
}

void* finalizar_proceso(){
    enviar_mensaje("FINALIZACION_ACEPTADA", fd_conexion_kernel, memoria_log);
    return NULL;
}

void* finalizar_hilo(){
    enviar_mensaje("OK", fd_conexion_kernel, memoria_log);
    return NULL;
}

void* memory_dump(){
    enviar_mensaje("OK", fd_conexion_kernel, memoria_log);
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

        int op = recibir_operacion(fd_conexion_cpu);

        switch (op){
        case SOLICITAR_CONTEXTO_EJECUCION: //Aca me solicita el contexto de un pid_tid, hay que ver que enum usamos
            buffer = buffer_recieve(fd_conexion_cpu);
        	pid_tid_recibido.pid = buffer_read_uint32(buffer);
        	pid_tid_recibido.tid = buffer_read_uint32(buffer);
            
           	usleep(memoria_registro.retardo_respuesta * 1000);

            log_info(memoria_log, "## [MEMORIA:CPU] Contexto <Solicitado> - (PID:TID) - (<%d>:<%d>)", pid_tid_recibido.pid, pid_tid_recibido.tid);
            contexto = buscar_contexto(pid_tid_recibido.pid, pid_tid_recibido.tid);
            enviar_contexto_solicitado(contexto);
            break;

        case ACTUALIZAR_CONTEXTO_EJECUCION: //Aca me pide actualizar el contexto de un pid_tid, hay que ver que enum usamos
        	buffer = buffer_recieve(fd_conexion_cpu);
        	pid_tid_recibido.pid = buffer_read_uint32(buffer);
        	pid_tid_recibido.tid = buffer_read_uint32(buffer);

            t_contexto* contexto_recibido = recibir_contexto(buffer);

            usleep(memoria_registro.retardo_respuesta * 1000);

            actualizar_contexto_ejecucion(contexto_recibido, pid_tid_recibido.pid, pid_tid_recibido.tid);
            log_info(memoria_log, "## [MEMORIA:CPU] Contexto <Actualizado> - (PID:TID) - (<%d>:<%d>)", pid_tid_recibido.pid, pid_tid_recibido.tid);
            //escribir_en_memoria(contexto_ejecucion, tamanio_contexto, direccion_fisica);
            enviar_mensaje("OK_CONTEXTO", fd_conexion_cpu, memoria_log);
            break;
            
        case PETICION_INSTRUCCION:
        	buffer = buffer_recieve(fd_conexion_cpu);

        	pid_tid_recibido.pid = buffer_read_uint32(buffer);
        	pid_tid_recibido.tid = buffer_read_uint32(buffer);
        	uint32_t programCounter = buffer_read_uint32(buffer);

        	usleep(memoria_registro.retardo_respuesta * 1000);

        	char* instruccion = buscar_instruccion(pid_tid_recibido.pid, pid_tid_recibido.tid, programCounter);
      	    log_info(memoria_log, "## Obtener instrucción - (PID:TID) - (<%d>:<%d>) - Instrucción: %s", pid_tid_recibido.pid, pid_tid_recibido.tid, instruccion);
            enviar_mensaje(instruccion, fd_conexion_cpu, memoria_log);
        	break;

        case WRITE_MEM:
            buffer = buffer_recieve(fd_conexion_cpu);
            pid_tid_recibido.pid = buffer_read_uint32(buffer);
        	pid_tid_recibido.tid = buffer_read_uint32(buffer);

            base = buffer_read_uint32(buffer);
            desplazamiento = buffer_read_uint32(buffer);
            valor_a_escribir = buffer_read_uint32(buffer);

            usleep(memoria_registro.retardo_respuesta * 1000);
            log_info(memoria_log, "## [MEMORIA:CPU] <Escritura> - (PID:TID) - (<%d>:<%d>) - Dir. Física: <%d> - Tamaño: <4>", pid_tid_recibido.pid, pid_tid_recibido.tid, direccion_fisica);

            //escribir_en_memoria(&dato, 4, direccion_fisica);
            enviar_mensaje("OK", fd_conexion_cpu, memoria_log);
            break;
            
        case READ_MEM:
            buffer = buffer_recieve(fd_conexion_cpu);
            pid_tid_recibido.pid = buffer_read_uint32(buffer);
        	pid_tid_recibido.tid = buffer_read_uint32(buffer);

            base = buffer_read_uint32(buffer);
            desplazamiento = buffer_read_uint32(buffer);

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
    }
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
        contexto_proceso = dictionary_get(contexto_ejecucion, key);
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

uint32_t hay_particion_disponible(uint32_t pid, uint32_t size, char* esquema){
    uint32_t valor_resultado;
    int size_particion = atoi(lista_particiones[0]);

    if (strcmp(esquema, "FIJAS") == 0)
    {
        if (obtener_espacio_desocupado() == OUT_OF_MEMORY)
        {
            valor_resultado = 0;
        } if (size > size_particion){
            valor_resultado = 0;
        } else {
            valor_resultado = obtener_espacio_desocupado();
        }
    } else {
        particion_dinamica(pid, size);
    }
    return valor_resultado;
}

void particion_dinamica(uint32_t pid, uint32_t size){
    
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
}

void* leer_de_memoria(uint32_t tamanio_lectura, uint32_t inicio_lectura){
    void* datos_memoria = malloc(tamanio_lectura);

    memcpy(datos_memoria, memoria + inicio_lectura, tamanio_lectura);

    return datos_memoria;
}

t_contexto_hilo* thread_get_by_tid(t_list* lista_hilos, int tid){
    bool _list_contains(void* ptr) {
	    t_contexto_hilo* hilo = (t_contexto_hilo*) ptr;
	    return (tid == hilo->tid);
	}
	return list_find(lista_hilos, _list_contains);
}

char** buscar_instruccion(uint32_t pid, uint32_t tid, uint32_t program_counter){
    char* key = string_itoa(pid);
    t_contexto_proceso* contexto_proceso = dictionary_get(contexto_ejecucion, key);
    t_contexto_hilo* contexto_hilo = thread_get_by_tid(contexto_proceso->lista_hilos, tid);
    char* instruccion = string_new();

    if(list_size(contexto_hilo->lista_instrucciones) > program_counter){
        instruccion = list_get(contexto_hilo->lista_instrucciones, program_counter);
    } else {
        instruccion = "NO_INSTRUCCION";
    }
    return instruccion;
}

bool busqueda_pid_tid(t_pseudocodigo* pseudocodigo){
    return (pseudocodigo->pid == pid_busqueda) && (pseudocodigo->tid == tid_busqueda);
}

void enviar_datos_memoria(void* buffer, uint32_t tamanio){
    t_paquete* paquete = crear_paquete(READ_MEM);
    agregar_a_paquete(paquete, buffer, tamanio);
    enviar_paquete(paquete, fd_conexion_cpu);
    eliminar_paquete(paquete);
}

void actualizar_contexto_ejecucion(t_contexto* contexto_recibido, uint32_t pid, uint32_t tid){
    t_contexto_proceso* contexto_proceso;
    t_contexto_hilo* contexto_hilo;

    char* key = string_itoa(pid);
    pthread_mutex_lock(&contexto_ejecucion_procesos);
    contexto_proceso = dictionary_get(contexto_ejecucion, key);
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

    dictionary_put(contexto_ejecucion, key, contexto_proceso);
    pthread_mutex_unlock(&contexto_ejecucion_procesos);
}

/*------------------------------------ WORKING ZONE ------------------------------------*/

void iniciar_memoria(){
    log_info(memoria_log, "MEMORIA INICIALIZADA\n");
    memoria = malloc(memoria_registro.tam_memoria); //Lo puse como variable global

    contexto_ejecucion = dictionary_create();
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

t_hueco* first_fit(){
    pthread_mutex_lock(&mutex_huecos_disponibles);
        void* _min_init_byte(void* a, void* b) {
	        t_hueco* hueco_a = (t_hueco*) a;
	        t_hueco* hueco_b = (t_hueco*) b;
	        return hueco_a->inicio <= hueco_b->inicio ? hueco_a : hueco_b;
	    }
	    t_hueco* hueco_hallado = list_get_minimum(lista_huecos_disponibles, _min_init_byte);
    pthread_mutex_unlock(&mutex_huecos_disponibles);
    return hueco_hallado;
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
    pthread_mutex_unlock(&mutex_huecos_disponibles);
    return hueco_hallado;
}

t_hueco* worst_fit(){
    pthread_mutex_lock(&mutex_huecos_disponibles);
        void* _max_size_byte(void* a, void* b) {
	        t_hueco* hueco_a = (t_hueco*) a;
	        t_hueco* hueco_b = (t_hueco*) b;
            return hueco_a->size >= hueco_b->size ? hueco_a : hueco_b;
	    }
	    t_hueco* hueco_hallado = list_get_maximum(lista_huecos_disponibles, _max_size_byte);
    pthread_mutex_unlock(&mutex_huecos_disponibles);
    return hueco_hallado;
}

t_hueco* obtener_hueco(uint32_t size){
    t_hueco* hueco;

    if (strcmp(memoria_registro.esquema, "FIRST"))
    {
        hueco = first_fit();
    } else if (strcmp(memoria_registro.esquema, "BEST"))
    {
        hueco = best_fit(size);
    } else if (strcmp(memoria_registro.esquema, "WORST"))
    {
        hueco = worst_fit();
    } else {
        log_error(memoria_log, "ERROR EN obtener_hueco");
        abort();
    }
    
    return hueco;
}

/*------------------------------------ WORKING ZONE ------------------------------------*/
