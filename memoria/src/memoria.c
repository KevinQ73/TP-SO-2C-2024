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

    //conexion_filesystem = crear_conexion(memoria_log, memoria_registro.ip_filesystem, memoria_registro.puerto_filesystem);
    //log_debug(memoria_log, "ME CONECTÉ A FILESYSTEM");

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

    lista_pseudocodigos = list_create(); //Lo puse aca para iniciar todo junto con la memoria
}

void atender_solicitudes(){
    pthread_create(&hiloMemoriaCpu, NULL, atender_cpu, NULL);
    pthread_detach(hiloMemoriaCpu);

    pthread_create(&hiloMemoriaKernel, NULL, atender_kernel, NULL);
    pthread_detach(hiloMemoriaKernel);
}

void* atender_kernel(){
    bool liberar_hilo_kernel = true;

    while(liberar_hilo_kernel){

        log_debug(memoria_log, "Esperando solicitud de KERNEL...");
        fd_conexion_kernel = esperar_cliente(memoria_log, "KERNEL", fd_conexiones);

        if (fd_conexion_kernel < 0) {
            // Llamar a la función de manejo de errores
            manejar_error_accept(errno, memoria_log);
            if (errno == EINTR || errno == ECONNABORTED) {
                continue;  // Reintentar la operación en caso de interrupción o conexión abortada
            } else {
                abort();
            }
        }

        log_info(memoria_log, "## Kernel Conectado - FD del socket: <%d>", fd_conexion_kernel);

        t_buffer* buffer;
        int size = 0;
        char* path = string_new();

        int operacion_kernel = recibir_operacion(fd_conexion_kernel);

        buffer = buffer_recieve(fd_conexion_kernel);

        switch (operacion_kernel)
        {
        case CREAR_PROCESO:
            uint32_t pid = buffer_read_uint32(buffer);
            uint32_t size_process = buffer_read_uint32(buffer);

            if (crear_proceso(pid, size_process))
            {
                enviar_mensaje("OK", fd_conexion_kernel, memoria_log);
                log_info(memoria_log, "## [MEMORIA:KERNEL] Proceso <Creado> - PID: <%d> - Tamaño: <%d>", pid, size_process);
            } else {
                enviar_mensaje("ERROR_CREAR_PROCESO", fd_conexion_kernel, memoria_log);
                log_error(memoria_log, "## [MEMORIA:KERNEL] Fallo en la creación del proceso");
            }
            close(fd_conexion_kernel);
            break;

        case CREAR_HILO:
            path = buffer_read_string(buffer, &size);
            int prioridad = buffer_read_uint32(buffer);

            log_debug(memoria_log, "## [MEMORIA:KERNEL] LLEGÓ CREACIÓN DE HILO DE PRIORIDAD: %d, Y PATH: %s", prioridad, path);
            enviar_mensaje("OK", fd_conexion_kernel, memoria_log);

            free(path);
            close(fd_conexion_kernel);
            break;

        case FINALIZAR_PROCESO:
            //aplicar_retardo();
            //uint32_t pid_proceso_finalizar = recibir_pid(fd_conexion_kernel);

            //finalizar_proceso(pid_proceso_finalizar);
            //pthread_mutex_unlock(&kernel_operando);
            break;

        default:
            log_debug(memoria_log, "## [MEMORIA:KERNEL] OPERACIÓN DE KERNEL ERRONEA");
            liberar_hilo_kernel = false;
            break;
        }
    }
}

void* atender_cpu(){
    bool liberar_hilo_cpu = true;

    while(liberar_hilo_cpu){
        t_pid_tid pid_tid_recibido;
        t_buffer* buffer;
        uint32_t direccion_fisica;
        uint32_t tamanio_contexto = 11*sizeof(uint32_t);

        int op = recibir_operacion(fd_conexion_cpu);

        switch (op){
        case PID_TID: //Aca me solicita el contexto de un pid_tid, hay que ver que enum usamos
        	buffer = buffer_recieve(fd_conexion_cpu);
        	pid_tid_recibido.pid = buffer_read_uint32(buffer);
        	pid_tid_recibido.tid = buffer_read_uint32(buffer);
            
            log_debug(memoria_log, "## [MEMORIA:CPU] Contexto <Solicitado> - (PID:TID) - (<%d>:<%d>)", pid_tid_recibido.pid, pid_tid_recibido.tid);
            usleep(memoria_registro.retardo_respuesta * 1000);
            direccion_fisica = buscar_contexto(pid_tid_recibido.pid, pid_tid_recibido.tid);
            log_debug(memoria_log, "## [MEMORIA:CPU] <Lectura> - (PID:TID) - (<%d>:<%d>) - Dir. Física: <%d> - Tamaño: <%d>", pid_tid_recibido.pid, pid_tid_recibido.tid, direccion_fisica, tamanio_contexto);
            enviar_contexto_solicitado(leer_de_memoria(tamanio_contexto, direccion_fisica), tamanio_contexto);
            break;

        case CONTEXTO_EJECUCION: //Aca me pide actualizar el contexto de un pid_tid, hay que ver que enum usamos
        	buffer = buffer_recieve(fd_conexion_cpu);
        	pid_tid_recibido.pid = buffer_read_uint32(buffer);
        	pid_tid_recibido.tid = buffer_read_uint32(buffer);

            void* contexto_ejecucion;
            memcpy(contexto_ejecucion, buffer->stream, tamanio_contexto);

            log_debug(memoria_log, "## [MEMORIA:CPU] Contexto <Actualizado> - (PID:TID) - (<%d>:<%d>)", pid_tid_recibido.pid, pid_tid_recibido.tid);
            usleep(memoria_registro.retardo_respuesta * 1000);
            direccion_fisica = buscar_contexto(pid_tid_recibido.pid, pid_tid_recibido.tid);
            log_debug(memoria_log, "## [MEMORIA:CPU] <Escritura> - (PID:TID) - (<%d>:<%d>) - Dir. Física: <%d> - Tamaño: <%d>", pid_tid_recibido.pid, pid_tid_recibido.tid, direccion_fisica, tamanio_contexto);
            //escribir_en_memoria(contexto_ejecucion, tamanio_contexto, direccion_fisica);
            enviar_mensaje("OK", fd_conexion_cpu, memoria_log);
            break;
            
        case PETICION_INSTRUCCION:
        	buffer = buffer_recieve(fd_conexion_cpu);

        	pid_tid_recibido.pid = buffer_read_uint32(buffer);
        	pid_tid_recibido.tid = buffer_read_uint32(buffer);
        	uint32_t programCounter = buffer_read_uint32(buffer);

        	usleep(memoria_registro.retardo_respuesta * 1000);

        	char* path_pid_tid = buscar_path(pid_tid_recibido.pid, pid_tid_recibido.tid);
        	char* instruccion = obtenerInstruccion(path_pid_tid, programCounter);
			log_debug(memoria_log, "## [MEMORIA:CPU] Obtener instrucción - (PID:TID) - (<%d>:<%d>) - Instrucción: <%s>", pid_tid_recibido.pid, pid_tid_recibido.tid, instruccion);
        	enviar_mensaje(instruccion, fd_conexion_cpu, memoria_log);
        	break;
        case WRITE_MEM:
            buffer = buffer_recieve(fd_conexion_cpu);
            pid_tid_recibido.pid = buffer_read_uint32(buffer);
        	pid_tid_recibido.tid = buffer_read_uint32(buffer);
            direccion_fisica = buffer_read_uint32(buffer);
            uint32_t dato = buffer_read_uint32(buffer);

            usleep(memoria_registro.retardo_respuesta * 1000);
            log_debug(memoria_log, "## [MEMORIA:CPU] <Escritura> - (PID:TID) - (<%d>:<%d>) - Dir. Física: <%d> - Tamaño: <4>", pid_tid_recibido.pid, pid_tid_recibido.tid, direccion_fisica);

            //escribir_en_memoria(&dato, 4, direccion_fisica);
            enviar_mensaje("OK", fd_conexion_cpu, memoria_log);
            break;
        case READ_MEM:
            buffer = buffer_recieve(fd_conexion_cpu);
            pid_tid_recibido.pid = buffer_read_uint32(buffer);
        	pid_tid_recibido.tid = buffer_read_uint32(buffer);
            direccion_fisica = buffer_read_uint32(buffer);

            usleep(memoria_registro.retardo_respuesta * 1000);
            log_debug(memoria_log, "## [MEMORIA:CPU] <Lectura> - (PID:TID) - (<%d>:<%d>) - Dir. Física: <%d> - Tamaño: <4>", pid_tid_recibido.pid, pid_tid_recibido.tid, direccion_fisica);

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

void sems(){
    pthread_mutex_init(&kernel_operando, NULL);

    sem_init(&memoria_activo, 0, 0);
}

bool crear_proceso(uint32_t pid, uint32_t size){
    bool memoria_asignada;
    
    uint32_t base_asignada = hay_particion_disponible(pid, size, memoria_registro.esquema);
    
    if (base_asignada){
        char* key = string_itoa(pid);
        t_contexto_proceso* contexto_proceso = crear_contexto_proceso(pid, base_asignada, size);
        dictionary_put(contexto_ejecucion, key, contexto_proceso);
        memoria_asignada = true;
    } else {
        log_error(memoria_log, "## [MEMORIA] OUT OF MEMORY");
        memoria_asignada = false;
    }    
    return memoria_asignada;
}

t_contexto_proceso* crear_contexto_proceso(uint32_t pid, uint32_t base, uint32_t limite){
    t_contexto_proceso* contexto = malloc(sizeof(t_contexto_proceso));

    contexto->pid = pid;
    contexto->base = base;
    contexto->limite = limite;
    contexto->lista_hilos = list_create();

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
        // DINAMICO
    }
    return valor_resultado;
}

uint32_t obtener_espacio_desocupado(){
    uint32_t base_particion = -1;

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

// PETICION 3: recibo pid para finalizar el proceso
void* finalizar_proceso(){
    enviar_mensaje("FINALIZACION_ACEPTADA", fd_conexion_kernel, memoria_log);
    return NULL;
}
void* crear_hilo(){
    enviar_mensaje("OK", fd_conexion_kernel, memoria_log);
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

void enviar_contexto_solicitado(void* buffer, uint32_t tamanio){
    t_paquete* paquete = crear_paquete(CONTEXTO_EJECUCION);
    agregar_a_paquete(paquete, buffer, tamanio);
    enviar_paquete(paquete, fd_conexion_cpu);
    eliminar_paquete(paquete);
}

char* obtenerInstruccion(char* pathInstrucciones, uint32_t program_counter){
    FILE* archivo_instrucciones = fopen(pathInstrucciones, "rb");

    if (archivo_instrucciones == NULL) {
        log_error(memoria_log,"ARCHIVO DE INSTRUCCIONES INEXISTENTE");
        fclose(archivo_instrucciones);
        abort();
    }

    int numLineaActual = 0;
    char lineaActual[1000];

    char* instruccion = malloc(sizeof(char*));

    while(feof(archivo_instrucciones) == 0){
    	fgets(lineaActual, 1000, file);
    	if(numLineaActual == program_counter){
    		instruccion = strtok(lineaActual, "\n");
    		break;
    	}
    	numLineaActual++;
    }
    fclose(archivo_instrucciones);
    return instruccion;
}

void* leer_de_memoria(uint32_t tamanio_lectura, uint32_t inicio_lectura){
    void* datos_memoria = malloc(tamanio_lectura);

    memcpy(datos_memoria, memoria + inicio_lectura, tamanio_lectura);

    return datos_memoria;
}

char* buscar_path(int pid, int tid){
    tid_busqueda = tid;
    pid_busqueda = pid;
    
    t_pseudocodigo* busqueda = (t_pseudocodigo*) list_find(lista_pseudocodigos, (void*) busqueda_pid_tid);

    return busqueda->pseudocodigo;
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

uint32_t buscar_contexto(uint32_t pid, uint32_t tid){
    uint32_t direccion_fisica = 1; //Le puse este valor para prueba nada mas
    
    //Aca se debe implementar una funcion que me diga donde empieza el PIT_TID que busco
    //Aca debo ver como memoria de usuario guarda los TCB y PCB

    return direccion_fisica;
}