#include <memoria.h>

int main(int argc, char* argv[]) {

    //---------------------------- Iniciar archivos ----------------------------

    memoria_log = iniciar_logger("./files/memoria.log", "MEMORIA", 1, LOG_LEVEL_DEBUG);

    memoria_config = iniciar_config("./files/memoria.config");

    memoria_registro = levantar_datos_memoria(memoria_config);

    // ------------------------ Iniciar servidor con CPU y Kernel------------------------

    fd_conexiones = iniciar_servidor(memoria_log, "MEMORIA", memoria_registro.puerto_escucha);
    log_debug(memoria_log,"SOCKET LISTEN LISTO PARA RECIBIR A CPU");

    iniciar_memoria();

    // --------------------- Conexión como cliente de FILESYSTEM ----------------------

    conexion_filesystem = crear_conexion(memoria_log, memoria_registro.ip_filesystem, memoria_registro.puerto_filesystem);
    log_debug(memoria_log, "ME CONECTÉ A FILESYSTEM");

    // --------------------- Conexiones de clientes con el servidor ----------------------
    
    log_debug(memoria_log, "Esperando CPU...");
    fd_conexion_cpu = esperar_cliente(memoria_log, "CPU", fd_conexiones);

    atender_solicitudes();

    // --------------------- Arrancan funciones de memoria ----------------------

    //FINALIZACION DEL MODULO

    log_debug(memoria_log, "TERMINANDO MEMORIA");
    //terminar_modulo(conexion_cpu, memoria_log, memoria_config);
    //terminar_modulo(conexion_kernel, memoria_log, memoria_config);
    //terminar_modulo(conexion_filesystem, memoria_log, memoria_config);

}

void atender_solicitudes(){
    pthread_create(&hiloMemoriaCpu, NULL, atender_cpu, NULL);
    pthread_detach(hiloMemoriaCpu);

    pthread_create(&hiloMemoriaKernel, NULL, atender_kernel, NULL);
    pthread_join(hiloMemoriaKernel, NULL);
}

void* atender_kernel(){
    bool liberar_hilo = true;

    while(liberar_hilo){

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

        t_buffer* buffer;
        int size = 0;
        char* path = string_new();
        uint32_t prioridad;

        int operacion_kernel = recibir_operacion(fd_conexion_kernel);

        buffer = buffer_recieve(fd_conexion_kernel);

        switch (operacion_kernel)
        {
        case CREAR_PROCESO:
            path = buffer_read_string(buffer, &size);
            uint32_t size_process = buffer_read_uint32(buffer);
            prioridad = buffer_read_uint32(buffer);

            log_debug(memoria_log, "LLEGÓ CREACIÓN DE PROCESO DE PRIORIDAD: %d, TAMAÑO: %d, Y PATH: %s", prioridad, size_process, path);
            enviar_mensaje("OK", fd_conexion_kernel, memoria_log);

            free(path);
            close(fd_conexion_kernel);
            break;

        case CREAR_HILO:
            path = buffer_read_string(buffer, &size);
            prioridad = buffer_read_uint32(buffer);

            log_debug(memoria_log, "LLEGÓ CREACIÓN DE HILO DE PRIORIDAD: %d, TAMAÑO: %d, Y PATH: %s", prioridad, size_process, path);
            enviar_mensaje("OK", fd_conexion_kernel, memoria_log);
            pthread_mutex_unlock(&kernel_operando);

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
            log_debug(memoria_log, "OPERACIÓN DE KERNEL ERRONEA");
            liberar_hilo = false;
            break;
        }
    }
}

void sems(){
    pthread_mutex_init(&kernel_operando, NULL);
}

void iniciar_memoria(){
    memoria = malloc(memoria_registro.tam_memoria); //Lo puse como variable global
    lista_particiones = memoria_registro.particiones;
    lista_pseudocodigos = list_create(); //Lo puse aca para iniciar todo junto con la memoria
}
void* crear_proceso(){
    if(strcmp(memoria_registro.esquema,"FIJAS")){
        
        }
    else{

    }
    enviar_mensaje("ESPACIO_ASIGNADO", fd_conexion_kernel, memoria_log);
    return NULL;
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
void atender_cpu(){
    while (1)
    {
        int op = recibir_operacion(fd_conexion_cpu);
        t_pid_tid pid_tid_recibido;
        t_buffer* buffer;
        uint32_t direccion_fisica;
        uint32_t tamanio_contexto = 11*sizeof(uint32_t);

        switch (op){
        case PID_TID: //Aca me solicita el contexto de un pid_tid, hay que ver que enum usamos
        	buffer = buffer_recieve(fd_conexion_cpu);
        	pid_tid_recibido.pid = buffer_read_uint32(buffer);
        	pid_tid_recibido.tid = buffer_read_uint32(buffer);
            log_debug(memoria_log, "## Contexto <Solicitado> - (PID:TID) - (<%d>:<%d>)", pid_tid_recibido.pid, pid_tid_recibido.tid);
            usleep(memoria_registro.retardo_respuesta * 1000);
            direccion_fisica = buscar_contexto(pid_tid_recibido.pid, pid_tid_recibido.tid);
            log_debug(memoria_log, "## <Lectura> - (PID:TID) - (<%d>:<%d>) - Dir. Física: <%d> - Tamaño: <%d>", pid_tid_recibido.pid, pid_tid_recibido.tid, direccion_fisica, tamanio_contexto);
            enviar_contexto_solicitado(leer_de_memoria(tamanio_contexto, direccion_fisica), tamanio_contexto);
            break;
        case CONTEXTO_EJECUCION: //Aca me pide actualizar el contexto de un pid_tid, hay que ver que enum usamos
        	buffer = buffer_recieve(fd_conexion_cpu);
        	pid_tid_recibido.pid = buffer_read_uint32(buffer);
        	pid_tid_recibido.tid = buffer_read_uint32(buffer);

            void* contexto_ejecucion;
            memcpy(contexto_ejecucion, buffer->stream, tamanio_contexto);

            log_debug(memoria_log, "## Contexto <Actualizado> - (PID:TID) - (<%d>:<%d>)", pid_tid_recibido.pid, pid_tid_recibido.tid);
            usleep(memoria_registro.retardo_respuesta * 1000);
            direccion_fisica = buscar_contexto(pid_tid_recibido.pid, pid_tid_recibido.tid);
            log_debug(memoria_log, "## <Escritura> - (PID:TID) - (<%d>:<%d>) - Dir. Física: <%d> - Tamaño: <%d>", pid_tid_recibido.pid, pid_tid_recibido.tid, direccion_fisica, tamanio_contexto);
            escribir_en_memoria(contexto_ejecucion, tamanio_contexto, direccion_fisica);
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
			log_debug(memoria_log, "## Obtener instrucción - (PID:TID) - (<%d>:<%d>) - Instrucción: <%s>", pid_tid_recibido.pid, pid_tid_recibido.tid, instruccion);
        	enviar_mensaje(instruccion, fd_conexion_cpu, memoria_log);
        	break;
        case WRITE_MEM:
            buffer = buffer_recieve(fd_conexion_cpu);
            pid_tid_recibido.pid = buffer_read_uint32(buffer);
        	pid_tid_recibido.tid = buffer_read_uint32(buffer);
            direccion_fisica = buffer_read_uint32(buffer);
            uint32_t dato = buffer_read_uint32(buffer);

            usleep(memoria_registro.retardo_respuesta * 1000);
            log_debug(memoria_log, "## <Escritura> - (PID:TID) - (<%d>:<%d>) - Dir. Física: <%d> - Tamaño: <4>", pid_tid_recibido.pid, pid_tid_recibido.tid, direccion_fisica);

            escribir_en_memoria(&dato, 4, direccion_fisica);
            enviar_mensaje("OK", fd_conexion_cpu, memoria_log);
            break;
        case READ_MEM:
            buffer = buffer_recieve(fd_conexion_cpu);
            pid_tid_recibido.pid = buffer_read_uint32(buffer);
        	pid_tid_recibido.tid = buffer_read_uint32(buffer);
            direccion_fisica = buffer_read_uint32(buffer);

            usleep(memoria_registro.retardo_respuesta * 1000);
            log_debug(memoria_log, "## <Lectura> - (PID:TID) - (<%d>:<%d>) - Dir. Física: <%d> - Tamaño: <4>", pid_tid_recibido.pid, pid_tid_recibido.tid, direccion_fisica);

            enviar_datos_memoria(leer_de_memoria(4, direccion_fisica), 4);
            break;
        case DESCONEXION:
            log_error(memoria_log, "Desconexion de Memoria-Cpu");
            break;
        default:
            log_warning(memoria_log, "Operacion desconocida de Memoria-Cpu");
            break;
        }
    }
}

void enviar_contexto_solicitado(void* buffer, uint32_t tamanio){
    t_paquete* paquete = crear_paquete(CONTEXTO_EJECUCION);
    agregar_a_paquete(paquete, buffer, tamanio);
    enviar_paquete(paquete, fd_conexion_cpu);
    eliminar_paquete(paquete);
}

char* obtenerInstruccion(char* pathInstrucciones, uint32_t program_counter){
    FILE* file;

    file = fopen(pathInstrucciones, "rt");
    int numLineaActual = 0;
    char lineaActual[1000];

    char* instruccion = malloc(sizeof(char*));

    while(feof(file) == 0){
    	fgets(lineaActual, 1000, file);
    	if(numLineaActual == program_counter){
    		instruccion = strtok(lineaActual, "\n");
    		break;
    	}
    	numLineaActual++;
    }
    fclose(file);
    return instruccion;
}

void escribir_en_memoria(void* buffer_escritura, uint32_t tamanio_buffer, uint32_t inicio_escritura){
    memcpy(memoria + inicio_escritura, buffer_escritura, tamanio_buffer);
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