#include <cpu.h>

int main(int argc, char* argv[]) {

    //---------------------------- Iniciar archivos ----------------------------

    cpu_log = iniciar_logger("./files/cpu.log", "CPU", 1, LOG_LEVEL_DEBUG);
    
    cpu_config = iniciar_config("./files/cpu.config");

    cpu_registro = levantar_datos(cpu_config);
    
    // --------------------- Conexión como servidor de KERNEL ----------------------

    fd_escucha_dispatch = iniciar_servidor(cpu_log, "CPU-DISPATCH", cpu_registro.puerto_cpu_dispatch);
    log_debug(cpu_log,"SOCKET DISPATCH LISTEN LISTO PARA RECIBIR CLIENTES");

    fd_escucha_interrupt = iniciar_servidor(cpu_log, "CPU-INTERRUPT", cpu_registro.puerto_cpu_interrupt);
    log_debug(cpu_log,"SOCKET INTERRUPT LISTEN LISTO PARA RECIBIR CLIENTES");

    // --------------------- Conexión como cliente de MEMORIA ----------------------

    conexion_memoria = crear_conexion(cpu_log, cpu_registro.ip_memoria, cpu_registro.puerto_memoria);
    log_debug(cpu_log, "ME CONECTÉ A MEMORIA");

    // ------------------------ Esperar conexión de Kernel -------------------------

    fd_conexion_dispatch = esperar_cliente(cpu_log, "KERNEL-DISPATCH", fd_escucha_dispatch);
    fd_conexion_interrupt = esperar_cliente(cpu_log, "KERNEL-INTERRUPT", fd_escucha_interrupt);

    // ------------------------ INICIAR CPU -------------------------

    registros_cpu = malloc(sizeof(t_contexto));

    pthread_create(&hilo_kernel_dispatch, NULL, (void *)atender_puerto_dispatch, NULL);
    pthread_detach(hilo_kernel_dispatch);

    // Atender los mensajes de Kernel - Interrupt
    pthread_create(&hilo_kernel_interrupt, NULL, (void *)atender_puerto_interrupt, NULL);
    pthread_join(hilo_kernel_interrupt, NULL);

    // ------------------------ FINALIZACIÓN CPU -------------------------

    log_debug(cpu_log, "TERMINANDO CPU");
    //terminar_modulo(fd_conexion_dispatch, cpu_log, cpu_config);
    //terminar_modulo(fd_conexion_interrupt, cpu_log, cpu_config);

    return EXIT_SUCCESS;
}

    /*--------------------- Atender conexiones a la CPU ---------------------*/


void atender_puerto_dispatch(){
    while (flag_disconect_dispatch)
    {
        int op = recibir_operacion(fd_conexion_dispatch);
        switch (op)
        {
        case EJECUTAR_HILO:
            pid_tid_recibido = recibir_paquete_kernel(fd_conexion_dispatch, cpu_log);
            log_debug(cpu_log, "PID RECIBIDO: %d, TID RECIBIDO: %d", pid_tid_recibido.pid, pid_tid_recibido.tid);
            ejecutar_hilo(pid_tid_recibido);
            break;
        
        case DESCONEXION:
            log_error(cpu_log, "Desconexion de Kernel - Dispatch");
            flag_disconect_dispatch = false;
            break;
        default:
            log_warning(cpu_log, "Operacion desconocida de Kernel - Dispatch");
            flag_disconect_dispatch = false;
            break;
        }
    }
}

void atender_puerto_interrupt(){
    while (flag_disconect_interrupt)
    {
        int op = recibir_operacion(fd_conexion_interrupt);
        switch (op)
        {
        case INTERRUPCION_QUANTUM:
            if (check_tid_interrupt(fd_conexion_interrupt)){
                interrupt_results(&(pid_tid_recibido.tid), "INTERRUPCIÓN POR QUANTUM");
            }
            break;
        
        case INTERRUPCION_USUARIO:
            if (check_tid_interrupt(fd_conexion_interrupt)){
                interrupt_results(&(pid_tid_recibido.tid), "INTERRUPCIÓN POR USUARIO");
            }
            break;

        default:
            log_debug(cpu_log, "Operacion desconocida de Kernel - Interrupt");
            flag_disconect_interrupt = false;
            break;
        }
    }
}

bool check_tid_interrupt(int fd_kernel){
    t_buffer* buffer = buffer_recieve(fd_kernel);
    uint32_t tid_recibido = buffer_read_uint32(buffer);

    return tid_recibido == pid_tid_recibido.tid;
}

void interrupt_results(uint32_t* tid, char* motivo){
    uint32_t length = strlen(motivo) + 1;
    t_paquete* paquete = crear_paquete(ENVIO_TID);
    t_buffer* buffer = buffer_create(
        sizeof(uint32_t) + length
    );

    buffer_add_uint32(buffer, tid, cpu_log);
    log_debug(cpu_log, "TID A ENVIAR: %d", *tid);
    buffer_add_string(buffer, length, motivo, cpu_log);
    log_debug(cpu_log, "AVISO A ENVIAR: %s", motivo);

    paquete->buffer = buffer;
    enviar_paquete(paquete, fd_conexion_interrupt);
    eliminar_paquete(paquete);
}
    /*-----------------------------------------------------------------------*/
    /*-------------------------- Ciclo de ejecución -------------------------*/

void ejecutar_hilo(t_pid_tid pid_tid_recibido){
    solicitar_contexto_ejecucion(registros_cpu, pid_tid_recibido, conexion_memoria, cpu_log);
    do
    {
        char* instruccion = fetch(pid_tid_recibido, &registros_cpu->pc, conexion_memoria, cpu_log);
        log_info(cpu_log, "## TID: <%d> - FETCH - Program Counter: <%d>", pid_tid_recibido.tid, registros_cpu->pc);

        if (strcmp(instruccion, "NO_INSTRUCCION") == 0)
        {
            interrupt_is_called = true;
        } else {
            char** instruccion_parseada = decode(instruccion);

            execute(registros_cpu, pid_tid_recibido, instruccion_parseada);

            string_array_destroy(instruccion_parseada);
            free(instruccion);
        }

    } while (!interrupt_is_called);
}

void execute(t_contexto* registros_cpu, t_pid_tid pid_tid_recibido, char** instruccion_parseada){
    inst_cpu op = obtener_codigo_instruccion(instruccion_parseada[0]);

    switch (op)
    {
    case SET:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <SET> - <%s> - <%s>", pid_tid_recibido.tid, instruccion_parseada[1], instruccion_parseada[2]);
        execute_set(registros_cpu, instruccion_parseada[1], instruccion_parseada[2]);
        break;
    
    case READ_MEM:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <READ_MEM> - <%s> - <%s>", pid_tid_recibido.tid, instruccion_parseada[1], instruccion_parseada[2]);
        execute_read_mem();
        break;

    case WRITE_MEM:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <WRITE_MEM> - <%s> - <%s>", pid_tid_recibido.tid, instruccion_parseada[1], instruccion_parseada[2]);
        execute_write_mem();
        break;

    case SUM:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <SUM> - <%s> - <%s>", pid_tid_recibido.tid, instruccion_parseada[1], instruccion_parseada[2]);
        execute_sum();
        break;

    case SUB:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <SUB> - <%s> - <%s>", pid_tid_recibido.tid, instruccion_parseada[1], instruccion_parseada[2]);
        execute_sub();
        break;

    case JNZ:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <JNZ> - <%s> - <%s>", pid_tid_recibido.tid, instruccion_parseada[1], instruccion_parseada[2]);
        execute_jnz();
        break;

    case LOG:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <LOG> - <%s>", pid_tid_recibido.tid, instruccion_parseada[1]);
        execute_log();
        break;

    case DUMP_MEMORY:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <DUMP_MEMORY>", pid_tid_recibido.tid);
        execute_dump_memory();
        break;

    case IO:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <IO> - <%s>", pid_tid_recibido.tid, instruccion_parseada[1]);
        execute_io();
        break;

    case PROCESS_CREATE:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <PROCESS_CREATE> - <%s> - <%s> - <%s>", pid_tid_recibido.tid, instruccion_parseada[1], instruccion_parseada[2], instruccion_parseada[3]);
        execute_process_create();
        break;

    case THREAD_CREATE:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <THREAD_CREATE> - <%s> - <%s>", pid_tid_recibido.tid, instruccion_parseada[1], instruccion_parseada[2]);
        execute_thread_create(registros_cpu, instruccion_parseada[1], instruccion_parseada[2]);
        break;

    case THREAD_JOIN:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <THREAD_JOIN> - <%s>", pid_tid_recibido.tid, instruccion_parseada[1]);
        execute_thread_join();
        break;

    case THREAD_CANCEL:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <THREAD_CANCEL> - <%s>", pid_tid_recibido.tid, instruccion_parseada[1]);
        execute_thread_cancel();
        break;

    case MUTEX_CREATE:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <MUTEX_CREATE> - <%s>", pid_tid_recibido.tid, instruccion_parseada[1]);
        execute_mutex_create();
        break;

    case MUTEX_LOCK:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <MUTEX_LOCK> - <%s>", pid_tid_recibido.tid, instruccion_parseada[1]);
        execute_mutex_lock();
        break;

    case MUTEX_UNLOCK:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <MUTEX_UNLOCK> - <%s>", pid_tid_recibido.tid, instruccion_parseada[1]);
        execute_mutex_unlock();
        break;

    case THREAD_EXIT:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <THREAD_EXIT>", pid_tid_recibido.tid);
        execute_thread_exit();
        break;

    case PROCESS_EXIT:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <PROCESS_EXIT>", pid_tid_recibido.tid);
        execute_process_exit();
        break;

    default:
        break;
    }
}

void execute_set(t_contexto* registro_cpu, char* registro, char* valor){
    int valor_int = (int)strtol(valor, NULL, 10);
    modificar_registro(registro_cpu, registro, valor_int, cpu_log);
    program_counter_update(registro_cpu, cpu_log);
}

void execute_read_mem(t_contexto* registro_cpu, char* registro_datos, char* registro_direccion){
    /**int valor_int = (int)strtol(registro_cpu.registro_direccion, NULL, 10);
    modificar_registro(registro_cpu, registro_datos, valor_int, cpu_log);
    program_counter_update(registro_cpu, cpu_log);*/
}

void execute_write_mem(t_contexto* registro_cpu, char* registro_direccion, char* registro_datos){
    /**int valor_int = (int)strtol(registro_cpu.registro_direccion, NULL, 10);
    modificar_registro(registro_cpu, registro_direccion, valor_int, cpu_log);
    program_counter_update(registro_cpu, cpu_log);*/
}

void execute_sum(t_contexto* registro_cpu, char* registro_destino, char* registro_origen){
    int resultado_int = get_register(registro_cpu, registro_destino) + get_register(registro_cpu, registro_origen);
    modificar_registro(registro_cpu, registro_destino, resultado_int, cpu_log);
    program_counter_update(registro_cpu, cpu_log);
}

void execute_sub(t_contexto* registro_cpu, char* registro_destino, char* registro_origen){
    int resultado_int = get_register(registro_cpu, registro_destino) - get_register(registro_cpu, registro_origen);
    modificar_registro(registro_cpu, registro_destino, resultado_int, cpu_log);
    program_counter_update(registro_cpu, cpu_log);
}

void execute_jnz(t_contexto* registro_cpu, char* registro, char* instruccion){
    int valor_int = get_register(registro_cpu, registro);
    if (valor_int /= 0){
        program_counter_jump(registro_cpu, instruccion, cpu_log);
    }
}

void execute_log(t_contexto* registro_cpu, char* registro){
    int info_int = get_register(registro_cpu, registro);
    log_info(cpu_log, "## Registro %s tiene valor %d", registro, info_int);
}

void execute_dump_memory(){
    
}

void execute_io(){
    
}

void execute_process_create(){
    
}

void execute_thread_create(t_contexto* registro_cpu, char* path, char* prioridad){
    int valor_int = atoi(prioridad);
    int length = strlen(path) + 1;

    t_buffer* buffer = buffer_create(
        sizeof(int) + length
    );

    buffer_add_uint32(buffer, &valor_int, cpu_log);
    buffer_add_string(buffer, length, path, cpu_log);

    enviar_paquete_kernel(buffer, fd_conexion_dispatch, THREAD_CREATE);
    enviar_registros_memoria(registro_cpu, pid_tid_recibido, conexion_memoria, cpu_log);
    char* respuesta_kernel = recibir_mensaje(fd_conexion_dispatch, cpu_log);

    if (strcmp(respuesta_kernel, "HILO_CREADO") != 0)
    {
        log_error(cpu_log, "## La Syscall THREAD_CREATE falló");
    }
    
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
