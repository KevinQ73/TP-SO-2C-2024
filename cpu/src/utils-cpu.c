#include <utils-cpu.h>

t_cpu levantar_datos(t_config* config){

    t_cpu datos_config;

    datos_config.ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    datos_config.puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    datos_config.puerto_cpu_dispatch = config_get_string_value(config,"PUERTO_ESCUCHA_DISPATCH");
    datos_config.puerto_cpu_interrupt = config_get_string_value(config,"PUERTO_ESCUCHA_INTERRUPT");
    datos_config.log_level = config_get_string_value(config, "LOG_LEVEL");

    return datos_config;
}

t_dictionary* inicializar_registros(){
    t_dictionary* registros = dictionary_create();
    int valor_inicial = 0;
    for (int i = 0; i < 11; i++) {
        dictionary_put(registros, nombres_registros[i], valor_inicial);
    }
    return registros;
}

t_pid_tid recibir_paquete_kernel(int socket_kernel, t_log* cpu_log){

    t_pid_tid pid_tid = recibir_pid_tid(socket_kernel, cpu_log);
    
    return pid_tid;
}

void recibir_paquete_memoria(t_dictionary* registros, int fd_memoria, t_log* cpu_log){
    t_buffer* buffer;
    uint32_t valor = 0;
    int i = 0;

    int op = recibir_operacion(fd_memoria);

    if (op == CONTEXTO_EJECUCION)
    {
        log_debug(cpu_log, "SE RECIBIÓ EL CONTEXTO DE EJECUCIÓN DE MEMORIA");
    } else {
        log_warning(cpu_log, "ERROR EN EL ENVÍO DEL CONTEXTO DE EJECUCIÓN");
        abort();
    }

    buffer = buffer_recieve(fd_memoria);

    while (i < 11)
    {
        valor = buffer_read_uint32(buffer);
        modificar_registro(registros, nombres_registros[i], valor, cpu_log);
    }

    buffer_destroy(buffer);
}

t_dictionary* solicitar_contexto_ejecucion(t_pid_tid pid_tid, int fd_memoria, t_log* log_cpu){
    t_dictionary* contexto_recibido = dictionary_create();
    
    enviar_pid_tid(&(pid_tid.pid), &(pid_tid.tid), fd_memoria, log_cpu);

    recibir_paquete_memoria(contexto_recibido, fd_memoria, log_cpu);

    return contexto_recibido;
}

void actualizar_registros_cpu(t_dictionary* registro_destino, t_dictionary* registro_inicial, t_log* log){
    for (int i = 0; i < dictionary_size(registro_destino); i++)
    {
        uint32_t valor = dictionary_get(registro_inicial, nombres_registros[i]);
        modificar_registro(registro_destino, nombres_registros[i], valor, log);
    }
}

void modificar_registro(t_dictionary* registro_destino, char* key, int nuevo_valor, t_log* log){
    uint32_t valor = dictionary_get(registro_destino, key);

    log_debug(log, "SE CAMBIA EL REGISTRO %s DE VALOR %d A %d", key, valor, nuevo_valor);
    dictionary_put(registro_destino, key, nuevo_valor);
}

void program_counter_update(t_dictionary* registro_cpu, t_log* log){
    uint32_t valor = dictionary_get(registro_cpu, "PC");
    uint32_t nuevo_valor = valor++;
    log_debug(log, "SE CAMBIA EL PROGRAM COUNTER DE VALOR %d A %d", valor, nuevo_valor);
    modificar_registro(registro_cpu, "PC", nuevo_valor, log);
}

char* fetch(t_pid_tid pid_tid_recibido, uint32_t* program_counter, int fd_memoria, t_log* log){
    t_buffer* buffer;

    t_paquete* paquete_memoria = crear_paquete(PETICION_INSTRUCCION);
    buffer = buffer_create(
        (sizeof(uint32_t) * 3)
    );

    buffer_add_uint32(buffer, &(pid_tid_recibido.pid), log);
    log_debug(log, "PID A ENVIAR: %d", pid_tid_recibido.pid);
    buffer_add_uint32(buffer, &(pid_tid_recibido.tid), log);
    log_debug(log, "TID A ENVIAR: %d", pid_tid_recibido.tid);
    buffer_add_uint32(buffer, program_counter, log);
    log_debug(log, "PROGRAM COUNTER A ENVIAR: %d", program_counter);

    paquete_memoria->buffer = buffer;
    enviar_paquete(paquete_memoria, fd_memoria);
    eliminar_paquete(paquete_memoria);

    char* instruccion = recibir_mensaje(fd_memoria, log);
    return instruccion;
}

char** decode(char* instruccion){
    char** instruccion_parseada = parsear_instruccion(instruccion);
    return instruccion_parseada;
}

void execute(t_dictionary* registro_a_modificar, char** instruccion_parseada, t_log* log){
    inst_cpu op = obtener_codigo_instruccion(instruccion_parseada[0]);

    switch (op)
    {
    case SET:
        execute_set(registro_a_modificar, instruccion_parseada[1], instruccion_parseada[2], log);
        break;
    
    case READ_MEM:
        execute_read_mem();
        break;

    case WRITE_MEM:
        execute_write_mem();
        break;

    case SUM:
        execute_sum();
        break;

    case SUB:
        execute_sub();
        break;

    case JNZ:
        execute_jnz();
        break;

    case LOG:
        execute_log();
        break;

    case DUMP_MEMORY:
        execute_dump_memory();
        break;

    case IO:
        execute_io();
        break;

    case PROCESS_CREATE:
        execute_process_create();
        break;

    case THREAD_CREATE:
        execute_thread_create();
        break;

    case THREAD_JOIN:
        execute_thread_join();
        break;

    case THREAD_CANCEL:
        execute_thread_cancel();
        break;

    case MUTEX_CREATE:
        execute_mutex_create();
        break;

    case MUTEX_LOCK:
        execute_mutex_lock();
        break;

    case MUTEX_UNLOCK:
        execute_mutex_unlock();
        break;

    case THREAD_EXIT:
        execute_thread_exit();
        break;

    case PROCESS_EXIT:
        execute_process_exit();
        break;

    default:
        break;
    }
}

void execute_set(t_dictionary* registro_cpu, char* registro, char* valor, t_log* log){
    int valor_int = (int)strtol(valor, NULL, 10);
    modificar_registro(registro_cpu, registro, valor_int, log);
    program_counter_update(registro_cpu, log);
}

void execute_read_mem(){

}

void execute_write_mem(){
    
}

void execute_sum(){
    
}

void execute_sub(){
    
}

void execute_jnz(){
    
}

void execute_log(){
    
}

void execute_dump_memory(){
    
}

void execute_io(){
    
}

void execute_process_create(){
    
}

void execute_thread_create(){
    
}

void execute_thread_join(){
    
}

void execute_thread_cancel(){
    
}

void execute_mutex_create(){
    
}

void execute_mutex_lock(){
    
}

void execute_mutex_unlock(){
    
}

void execute_thread_exit(){
    
}

void execute_process_exit(){
    
}
