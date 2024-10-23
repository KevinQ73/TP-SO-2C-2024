#include <memoria.h>

int main(int argc, char* argv[]) {

    //---------------------------- Iniciar archivos ----------------------------

    memoria_log = iniciar_logger("./files/memoria.log", "MEMORIA", 1, LOG_LEVEL_DEBUG);

    memoria_config = iniciar_config("./files/memoria.config");

    memoria_registro = levantar_datos_memoria(memoria_config);

    // ------------------------ Iniciar servidor con CPU y Kernel------------------------

    fd_conexiones = iniciar_servidor(memoria_log, "MEMORIA", memoria_registro.puerto_escucha);
    log_debug(memoria_log,"SOCKET LISTEN LISTO PARA RECIBIR CLIENTES");

    // --------------------- Conexión como cliente de FILESYSTEM ----------------------

    conexion_filesystem = crear_conexion(memoria_log, memoria_registro.ip_filesystem, memoria_registro.puerto_filesystem);
    log_debug(memoria_log, "ME CONECTÉ A FILESYSTEM");

    // --------------------- Conexiones de clientes con el servidor ----------------------
    
    log_debug(memoria_log, "Esperando CPU...");
    fd_conexion_cpu = esperar_cliente(memoria_log, "CPU", fd_conexiones);

    log_debug(memoria_log, "Esperando KERNEL...");
    fd_conexion_kernel = esperar_cliente(memoria_log, "KERNEL", fd_conexiones);

    // --------------------- Arrancan funciones de memoria ----------------------
    iniciar_memoria();
    atender_kernel();
    pthread_create(&hiloMemoriaCpu, NULL, (void *)atender_cpu, NULL);
    pthread_detach(hiloMemoriaCpu);
    //FINALIZACION DEL MODULO

    log_debug(memoria_log, "TERMINANDO MEMORIA");
    //terminar_modulo(conexion_cpu, memoria_log, memoria_config);
    //terminar_modulo(conexion_kernel, memoria_log, memoria_config);
    //terminar_modulo(conexion_filesystem, memoria_log, memoria_config);

}
void atender_kernel(){
    // recibir paquete de kernel
    t_list* paquete = recibir_paquete(fd_conexion_kernel);
    int op = recibir_operacion(fd_conexion_kernel);

    if(op == CREAR_PROCESO){
        pthread_create(&hiloCrearProceso,NULL,crear_proceso(),NULL);
        pthread_detach(hiloCrearProceso);
    }
    else if(op == FINALIZAR_PROCESO){
        pthread_create(&hiloFinalizarProceso,NULL,finalizar_proceso(),NULL);
        pthread_detach(hiloFinalizarProceso);
    }
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
        switch (op){
        case PID_TID:
            pid_tid_recibido = recibir_pid_tid(fd_conexion_cpu, memoria_log);
            log_debug(memoria_log, "## Contexto <Solicitado> - (PID:TID) - (<%d>:<%d>)", pid_tid_recibido.pid, pid_tid_recibido.tid);
            //Aqui debo buscar el contexto en memoria de usuario
            usleep(memoria_registro.retardo_respuesta * 1000);
            t_dictionary* registro_solicitado = dictionary_create();
            // registro_solicitado = buscar_registro(pid_tid_recibido);
            enviar_contexto_solicitado(registro_solicitado);
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

void enviar_contexto_solicitado(t_dictionary* registro_solicitado){
    t_paquete* paquete = crear_paquete(CONTEXTO_EJECUCION);
    for (int i = 0; i < 11; i++) {
    	uint32_t* registro = (uint32_t*) dictionary_get(registro_solicitado, nombres_registros[i]);
    	agregar_a_paquete(paquete, registro, sizeof(uint32_t));
    }
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