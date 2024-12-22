#include <cpu.h>

int main(int argc, char* argv[]) {

    //---------------------------- Iniciar archivos ----------------------------
    
    cpu_log = iniciar_logger("./files/cpu_obligatorio.log", "CPU", 1, LOG_LEVEL_INFO);

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

    iniciar_semaforos();

    pthread_create(&hilo_kernel_dispatch, NULL, (void *)atender_puerto_dispatch, NULL);
    pthread_detach(hilo_kernel_dispatch);

    // Atender los mensajes de Kernel - Interrupt
    pthread_create(&hilo_kernel_interrupt, NULL, (void *)atender_puerto_interrupt, NULL);
    pthread_join(hilo_kernel_interrupt, NULL);

    // ------------------------ FINALIZACIÓN CPU -------------------------
    sem_wait(&cpu_activo);

    log_warning(cpu_log, "TERMINANDO CPU");
    finalizar_cpu();
}

    /*--------------------- Atender conexiones a la CPU ---------------------*/

void iniciar_semaforos(){

    pthread_mutex_init(&mutex_pid_tid, NULL);
    pthread_mutex_init(&mutex_quantum, NULL);
    pthread_mutex_init(&mutex_interrupt, NULL);
    pthread_mutex_init(&mutex_dispatch, NULL);

    sem_init(&aviso_syscall, 0, 0);
    sem_init(&cpu_activo, 0, 0);
}

void finalizar_cpu(){
    pthread_cancel(hilo_kernel_interrupt);

    pthread_mutex_destroy(&mutex_pid_tid);
    pthread_mutex_destroy(&mutex_quantum);
    pthread_mutex_destroy(&mutex_interrupt);
    pthread_mutex_destroy(&mutex_dispatch);

    sem_destroy(&aviso_syscall);
    sem_destroy(&cpu_activo);

    free(pid_tid_recibido);
    free(registros_cpu);

    eliminar_logger(cpu_log);
    eliminar_config(cpu_config);

    close(fd_conexion_dispatch);
    close(fd_conexion_interrupt);

    log_debug(cpu_log, "eliminé todo");
}

void atender_puerto_dispatch(){
    while (flag_disconect_dispatch)
    {
        pthread_mutex_lock(&mutex_dispatch);
            int op = recibir_operacion(fd_conexion_dispatch);
        pthread_mutex_unlock(&mutex_dispatch);
        pthread_mutex_lock(&mutex_pid_tid);
            pid_tid_recibido = recibir_paquete_kernel(fd_conexion_dispatch, cpu_log);
        pthread_mutex_unlock(&mutex_pid_tid);

        if (pid_tid_recibido)
        {
            cpu_replanificar = false;
            false_quantum();
            desalojo_kernel = false;
        } else {
            log_error(cpu_log, "## PID TID RECIBIDO ES NULL");
        }
        switch (op)
        {
        case EJECUTAR_HILO:
            log_info(cpu_log, "## PID: <%d> - TID: <%d> Recibidos para empezar el ciclo de ejecución en CPU", pid_tid_recibido->pid, pid_tid_recibido->tid);
            solicitar_contexto_ejecucion(registros_cpu, pid_tid_recibido, conexion_memoria, cpu_log);
            ejecutar_hilo(pid_tid_recibido);
            break;

        case DESCONEXION:
            log_debug(cpu_log, "Desconexion de Kernel - Dispatch");
            flag_disconect_dispatch = false;
            break;
        default:
            log_debug(cpu_log, "Operacion desconocida de Kernel - Dispatch");
            flag_disconect_dispatch = false;
            break;
        }
    }
    sem_post(&cpu_activo);
}

void atender_puerto_interrupt(){
    while (flag_disconect_interrupt)
    {
        pthread_mutex_lock(&mutex_interrupt);
        int op = recibir_operacion(fd_conexion_interrupt);
        pthread_mutex_unlock(&mutex_interrupt);
        switch (op)
        {
        case INTERRUPCION_QUANTUM:
            int length = 0; 

            pthread_mutex_lock(&mutex_interrupt);
                t_buffer* buffer = buffer_recieve(fd_conexion_interrupt);
            pthread_mutex_unlock(&mutex_interrupt);
            char* mensaje = buffer_read_string(buffer, &length);

            if (strcmp(mensaje, "FIN_QUANTUM") == 0)
            {
                log_info(cpu_log, "## Llega interrupción al puerto Interrupt");
                recibo_kernel_ok = true;
                true_quantum();
            }
            free(mensaje);
	        buffer_destroy(buffer);
            break;

        case AVISO_EXITO_SYSCALL:
            recibo_kernel_ok = recibir_aviso_syscall(fd_conexion_interrupt, cpu_log);
            sem_post(&aviso_syscall);
            break;

        default:
            log_debug(cpu_log, "Operacion desconocida de Kernel - Interrupt");
            flag_disconect_interrupt = false;
            break;
        }
    }
    sem_post(&cpu_activo);
}

void check_interrupt(){
    if (estado_quantum())
    {
        log_info(cpu_log, "## Llega interrupción QUANTUM al puerto Interrupt");
        enviar_registros_memoria(registros_cpu, pid_tid_recibido, conexion_memoria, cpu_log);
        desalojar_pid_tid();

    } else if(desalojo_kernel) 
    {
        pthread_mutex_lock(&mutex_pid_tid);
            pid_tid_recibido = NULL;
        pthread_mutex_unlock(&mutex_pid_tid);
        cpu_replanificar = true;
        log_info(cpu_log, "## Hubo replanificación en Kernel");
    } else {
        log_debug(cpu_log, "NO HUBO INTERRUPCIÓN");
    }
}

bool estado_quantum(){
    bool estado;
    pthread_mutex_lock(&mutex_quantum);
    estado = quantum_is_called;
    pthread_mutex_unlock(&mutex_quantum);

    return estado;
}

bool true_quantum(){
    pthread_mutex_lock(&mutex_quantum);
    quantum_is_called = true;
    pthread_mutex_unlock(&mutex_quantum);

    return quantum_is_called;
}

bool false_quantum(){
    pthread_mutex_lock(&mutex_quantum);
    quantum_is_called = false;
    pthread_mutex_unlock(&mutex_quantum);

    return quantum_is_called;
}

void desalojar_pid_tid(){
    t_buffer* buffer;
    char* mensaje_quantum = "CPU_DESALOJADO";
    int length = strlen(mensaje_quantum) + 1;

	t_paquete* paquete = crear_paquete(DESALOJO_PID_TID_CPU);
	buffer = buffer_create(
	    sizeof(uint32_t)*2 + length 
    );

    buffer_add_uint32(buffer, &pid_tid_recibido->pid, cpu_log);
    buffer_add_uint32(buffer, &pid_tid_recibido->tid, cpu_log);
    buffer_add_string(buffer, length, mensaje_quantum, cpu_log);

    paquete->buffer = buffer;
    enviar_paquete(paquete, fd_conexion_dispatch);
    eliminar_paquete(paquete);

    log_info(cpu_log, "## TID <%d> fue desalojado en Kernel y CPU.", pid_tid_recibido->tid);
    pthread_mutex_lock(&mutex_pid_tid);
        pid_tid_recibido = NULL;
    pthread_mutex_unlock(&mutex_pid_tid);

    cpu_replanificar = true;
}

    /*-----------------------------------------------------------------------*/
    /*-------------------------- Ciclo de ejecución -------------------------*/

void ejecutar_hilo(t_pid_tid* pid_tid_recibido){
    while (!cpu_replanificar){
        char* instruccion = fetch(pid_tid_recibido, &registros_cpu->pc, conexion_memoria, cpu_log);
        log_info(cpu_log, "## TID: <%d> - FETCH - Program Counter: <%d>", pid_tid_recibido->tid, registros_cpu->pc);

        char** instruccion_parseada = decode(instruccion);

        execute(registros_cpu, pid_tid_recibido, instruccion_parseada);

        string_array_destroy(instruccion_parseada);
        free(instruccion);
    }
}

void execute(t_contexto* registros_cpu, t_pid_tid* pid_tid_recibido, char** instruccion_parseada){
    inst_cpu op = obtener_codigo_instruccion(instruccion_parseada[0]);

    switch (op)
    {
    case SET:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <SET> - <%s> - <%s>", pid_tid_recibido->tid, instruccion_parseada[1], instruccion_parseada[2]);
        execute_set(registros_cpu, instruccion_parseada[1], instruccion_parseada[2]);
        break;
    
    case READ_MEM: // REPLANIFICA SI HAY SEGFAULT -- LISTO
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <READ_MEM> - <%s> - <%s>", pid_tid_recibido->tid, instruccion_parseada[1], instruccion_parseada[2]);
        execute_read_mem(registros_cpu, pid_tid_recibido, instruccion_parseada[1], instruccion_parseada[2]);
        break;

    case WRITE_MEM: // REPLANIFICA SI HAY SEGFAULT -- LISTO
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <WRITE_MEM> - <%s> - <%s>", pid_tid_recibido->tid, instruccion_parseada[1], instruccion_parseada[2]);
        execute_write_mem(registros_cpu, pid_tid_recibido, instruccion_parseada[1], instruccion_parseada[2]);
        break;

    case SUM:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <SUM> - <%s> - <%s>", pid_tid_recibido->tid, instruccion_parseada[1], instruccion_parseada[2]);
        execute_sum(registros_cpu, instruccion_parseada[1], instruccion_parseada[2]);
        break;

    case SUB:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <SUB> - <%s> - <%s>", pid_tid_recibido->tid, instruccion_parseada[1], instruccion_parseada[2]);
        execute_sub(registros_cpu, instruccion_parseada[1], instruccion_parseada[2]);
        break;

    case JNZ:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <JNZ> - <%s> - <%s>", pid_tid_recibido->tid, instruccion_parseada[1], instruccion_parseada[2]);
        execute_jnz(registros_cpu, instruccion_parseada[1], instruccion_parseada[2]);
        break;

    case LOG:
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <LOG> - <%s>", pid_tid_recibido->tid, instruccion_parseada[1]);
        execute_log(registros_cpu, instruccion_parseada[1]);
        break;

    case DUMP_MEMORY: // NO REPLANIFICA -- NO LISTO
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <DUMP_MEMORY>", pid_tid_recibido->tid);
        execute_dump_memory(registros_cpu);
        break;

    case IO: // NO REPLANIFICA -- NO LISTO
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <IO> - <%s>", pid_tid_recibido->tid, instruccion_parseada[1]);
        execute_io(registros_cpu, instruccion_parseada[1]);
        break;

    case PROCESS_CREATE: // NO REPLANIFICA -- NO LISTO
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <PROCESS_CREATE> - <%s> - <%s> - <%s>", pid_tid_recibido->tid, instruccion_parseada[1], instruccion_parseada[2], instruccion_parseada[3]);
        execute_process_create(registros_cpu, instruccion_parseada[1], instruccion_parseada[2], instruccion_parseada[3]);
        break;

    case THREAD_CREATE: // NO REPLANIFICA -- NO LISTO
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <THREAD_CREATE> - <%s> - <%s>", pid_tid_recibido->tid, instruccion_parseada[1], instruccion_parseada[2]);
        execute_thread_create(registros_cpu, instruccion_parseada[1], instruccion_parseada[2]);
        break;

    case THREAD_JOIN: // REPLANIFICA -- LISTO
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <THREAD_JOIN> - <%s>", pid_tid_recibido->tid, instruccion_parseada[1]);
        execute_thread_join(registros_cpu, instruccion_parseada[1]);
        break;

    case THREAD_CANCEL: // REPLANIFICA SI EL CANCEL ES AL MISMO HILO -- LISTO
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <THREAD_CANCEL> - <%s>", pid_tid_recibido->tid, instruccion_parseada[1]);
        execute_thread_cancel(registros_cpu, instruccion_parseada[1]);
        break;

    case MUTEX_CREATE: // NO REPLANIFICA -- LISTO
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <MUTEX_CREATE> - <%s>", pid_tid_recibido->tid, instruccion_parseada[1]);
        execute_mutex_create(registros_cpu, instruccion_parseada[1]);
        break;

    case MUTEX_LOCK: // REPLANIFICA SI EL MUTEX SOLICITADO ESTÁ TOMADO -- LISTO
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <MUTEX_LOCK> - <%s>", pid_tid_recibido->tid, instruccion_parseada[1]);
        execute_mutex_lock(registros_cpu, instruccion_parseada[1]);
        break;

    case MUTEX_UNLOCK: // NO REPLANIFICA -- LISTO
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <MUTEX_UNLOCK> - <%s>", pid_tid_recibido->tid, instruccion_parseada[1]);
        execute_mutex_unlock(registros_cpu, instruccion_parseada[1]);
        break;

    case THREAD_EXIT: // REPLANIFICA -- LISTO
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <THREAD_EXIT>", pid_tid_recibido->tid);
        execute_thread_exit(registros_cpu);
        break;

    case PROCESS_EXIT: // REPLANIFICA -- LISTO
        log_info(cpu_log, "## TID: <%d> - Ejecutando: <PROCESS_EXIT>", pid_tid_recibido->tid);
        execute_process_exit(registros_cpu);
        break;

    default:
        log_error(cpu_log, "ERROR EN LA INSTRUCCION A EJECUTAR");
        abort();
        break;
    }
}

void execute_set(t_contexto* registro_cpu, char* registro, char* valor){
    int valor_int = (int)strtol(valor, NULL, 10);
    modificar_registro(registro_cpu, registro, valor_int, cpu_log);
    program_counter_update(registro_cpu, cpu_log);

    if (recibo_kernel_ok)
    {
        check_interrupt();
    } else {
        log_debug(cpu_log, "ERROR EN execute_set");
    }
}

void execute_read_mem(t_contexto* registro_cpu, t_pid_tid* pid_tid_recibido, char* registro_datos, char* registro_direccion){
    uint32_t valor_registro_direccion = get_register(registro_cpu, registro_direccion);
    t_direccion_fisica dir_fis = mmu(registro_cpu->base, registro_cpu->limite, valor_registro_direccion);
    
    if (dir_fis.segmentation_fault)
    {
        segmentation_fault = true;
        log_warning(cpu_log, "## TID: <%d> SEGMENTATION FAULT", pid_tid_recibido->tid);
        enviar_registros_memoria(registro_cpu, pid_tid_recibido, conexion_memoria, cpu_log);
        desalojar_pid_tid();
    } else {
        log_info(cpu_log, "## TID: <%d> - Acción: <LEER> - Dirección Física: <BASE: %d - DESPLAZAMIENTO: %d>", pid_tid_recibido->tid, dir_fis.base, dir_fis.desplazamiento);
        enviar_direccion_fisica(dir_fis, pid_tid_recibido, conexion_memoria, cpu_log);
        recibir_valor_memoria(registro_cpu, registro_datos, conexion_memoria, cpu_log);
        program_counter_update(registro_cpu, cpu_log);
    }

    if (!segmentation_fault)
    {
        if (recibo_kernel_ok)
        {
            check_interrupt();
        } else {
            log_error(cpu_log, "ERROR EN execute_read_mem");
        }
    }
}

void execute_write_mem(t_contexto* registro_cpu, t_pid_tid* pid_tid_recibido, char* registro_direccion, char* registro_datos){
    uint32_t valor_registro_datos = get_register(registro_cpu, registro_datos);
    uint32_t valor_registro_direccion = get_register(registro_cpu, registro_direccion);

    t_direccion_fisica dir_fis = mmu(registro_cpu->base, registro_cpu->limite, valor_registro_direccion);
    
    if (dir_fis.segmentation_fault)
    {
        log_warning(cpu_log, "## TID: <%d> SEGMENTATION FAULT", pid_tid_recibido->tid);
        enviar_registros_memoria(registro_cpu, pid_tid_recibido, conexion_memoria, cpu_log);
        desalojar_pid_tid();
    } else {
        log_info(cpu_log, "## TID: <%d> - Acción: <ESCRIBIR> - Dirección Física: <BASE: %d - DESPLAZAMIENTO: %d - VALOR A ESCRIBIR: %d>", pid_tid_recibido->tid, dir_fis.base, dir_fis.desplazamiento, valor_registro_datos);
        escribir_valor_en_memoria(dir_fis, pid_tid_recibido, valor_registro_datos, conexion_memoria, cpu_log);
        char* response_memoria = recibir_mensaje(conexion_memoria, cpu_log);
        log_debug(cpu_log, "STRING RECIBIDO execute_write_mem: %s", response_memoria);
        program_counter_update(registro_cpu, cpu_log);
    }
    
    if (!segmentation_fault)
    {
        if (recibo_kernel_ok)
        {
            check_interrupt();
        } else {
            log_error(cpu_log, "ERROR EN execute_write_mem");
        }
    }
}

void execute_sum(t_contexto* registro_cpu, char* registro_destino, char* registro_origen){
    int resultado_int = get_register(registro_cpu, registro_destino) + get_register(registro_cpu, registro_origen);
    modificar_registro(registro_cpu, registro_destino, resultado_int, cpu_log);
    program_counter_update(registro_cpu, cpu_log);

    if (recibo_kernel_ok)
    {
        check_interrupt();
    } else {
        log_error(cpu_log, "ERROR EN execute_sum");
    }
}

void execute_sub(t_contexto* registro_cpu, char* registro_destino, char* registro_origen){
    int resultado_int = get_register(registro_cpu, registro_destino) - get_register(registro_cpu, registro_origen);
    modificar_registro(registro_cpu, registro_destino, resultado_int, cpu_log);
    program_counter_update(registro_cpu, cpu_log);

    if (recibo_kernel_ok)
    {
        check_interrupt();
    } else {
        log_error(cpu_log, "ERROR EN execute_sub");
    }
}

void execute_jnz(t_contexto* registro_cpu, char* registro, char* instruccion){
    int valor_int = get_register(registro_cpu, registro);
    if (valor_int != 0){
        int valor_instruccion = atoi(instruccion);
        program_counter_jump(registro_cpu, valor_instruccion, cpu_log);
    } else {
        program_counter_update(registro_cpu, cpu_log);
    }

    if (recibo_kernel_ok)
    {
        check_interrupt();
    } else {
        log_error(cpu_log, "ERROR EN execute_jnz");
    }
}

void execute_log(t_contexto* registro_cpu, char* registro){
    int info_int = get_register(registro_cpu, registro);
    log_info(cpu_log, "## Registro %s tiene valor %d", registro, info_int);
    program_counter_update(registro_cpu, cpu_log);

    if (recibo_kernel_ok)
    {
        check_interrupt();
    } else {
        log_error(cpu_log, "ERROR EN execute_log");
    }
}

void execute_dump_memory(t_contexto* registro_cpu){
    t_buffer* buffer = buffer_create(
        sizeof(int)*2
    );
    
    buffer_add_uint32(buffer, &pid_tid_recibido->pid, cpu_log);
    buffer_add_uint32(buffer, &pid_tid_recibido->tid, cpu_log);

    enviar_registros_memoria(registro_cpu, pid_tid_recibido, conexion_memoria, cpu_log);
    enviar_paquete_kernel(buffer, fd_conexion_dispatch, DUMP_MEMORY);

    program_counter_update(registro_cpu, cpu_log);
    sem_wait(&aviso_syscall);

    if (recibo_kernel_ok)
    {
        check_interrupt();
    } else {
        log_error(cpu_log, "ERROR EN execute_dump_memory");
    }
}

void execute_io(t_contexto* registro_cpu, char* tiempo){
    int valor_tiempo = atoi(tiempo);

    t_buffer* buffer = buffer_create(
        sizeof(int)*3
    );
    
    buffer_add_uint32(buffer, &pid_tid_recibido->pid, cpu_log);
    buffer_add_uint32(buffer, &pid_tid_recibido->tid, cpu_log);
    buffer_add_uint32(buffer, &valor_tiempo, cpu_log);

    enviar_registros_memoria(registro_cpu, pid_tid_recibido, conexion_memoria, cpu_log);
    enviar_paquete_kernel(buffer, fd_conexion_dispatch, IO);

    program_counter_update(registro_cpu, cpu_log);
    sem_wait(&aviso_syscall);

    if (recibo_kernel_ok)
    {
        check_interrupt();
    } else {
        log_error(cpu_log, "ERROR EN execute_io");
    }
}

void execute_process_create(t_contexto* registro_cpu, char* path, char* size_process, char* prioridad_hilo_main){
    int valor_size = atoi(size_process);
    int valor_prioridad = atoi(prioridad_hilo_main);
    int length = strlen(path) + 1;

    t_buffer* buffer = buffer_create(
        sizeof(int)*4 + length
    );
    
    buffer_add_uint32(buffer, &pid_tid_recibido->pid, cpu_log);
    buffer_add_uint32(buffer, &pid_tid_recibido->tid, cpu_log);
    buffer_add_uint32(buffer, &valor_size, cpu_log);
    buffer_add_uint32(buffer, &valor_prioridad, cpu_log);
    buffer_add_string(buffer, length, path, cpu_log);

    enviar_registros_memoria(registro_cpu, pid_tid_recibido, conexion_memoria, cpu_log);
    enviar_paquete_kernel(buffer, fd_conexion_dispatch, PROCESS_CREATE);

    program_counter_update(registro_cpu, cpu_log);

    sem_wait(&aviso_syscall);

    if (recibo_kernel_ok)
    {
        check_interrupt();
    } else {
        log_error(cpu_log, "ERROR EN execute_process_create");
    }
}

void execute_thread_create(t_contexto* registro_cpu, char* path, char* prioridad){
    int valor_prioridad = atoi(prioridad);
    int length = strlen(path) + 1;

    t_buffer* buffer = buffer_create(
        sizeof(int)*3 + length
    );
    
    buffer_add_uint32(buffer, &pid_tid_recibido->pid, cpu_log);
    buffer_add_uint32(buffer, &pid_tid_recibido->tid, cpu_log);
    buffer_add_uint32(buffer, &valor_prioridad, cpu_log);
    buffer_add_string(buffer, length, path, cpu_log);

    program_counter_update(registro_cpu, cpu_log);
    enviar_registros_memoria(registro_cpu, pid_tid_recibido, conexion_memoria, cpu_log);
    enviar_paquete_kernel(buffer, fd_conexion_dispatch, THREAD_CREATE);

    sem_wait(&aviso_syscall);

    if (recibo_kernel_ok)
    {
        check_interrupt();
    } else {
        log_error(cpu_log, "ERROR EN execute_thread_create");
    }
}

void execute_thread_join(t_contexto* registro_cpu, char* tid_join){
    int valor_tid_join = atoi(tid_join);

    t_buffer* buffer = buffer_create(
        sizeof(int)*3
    );
    
    buffer_add_uint32(buffer, &pid_tid_recibido->pid, cpu_log);
    buffer_add_uint32(buffer, &pid_tid_recibido->tid, cpu_log);
    buffer_add_uint32(buffer, &valor_tid_join, cpu_log);

    program_counter_update(registro_cpu, cpu_log);
    enviar_registros_memoria(registro_cpu, pid_tid_recibido, conexion_memoria, cpu_log);
    enviar_paquete_kernel(buffer, fd_conexion_dispatch, THREAD_JOIN);

    sem_wait(&aviso_syscall);

    if (recibo_kernel_ok)
    {
        check_interrupt();
    } else {
        log_error(cpu_log, "ERROR EN execute_thread_join");
    }
}

void execute_thread_cancel(t_contexto* registro_cpu, char* tid_cancel){
    int valor_tid_cancel = atoi(tid_cancel);

    t_buffer* buffer = buffer_create(
        sizeof(int)*3
    );
    
    buffer_add_uint32(buffer, &pid_tid_recibido->pid, cpu_log);
    buffer_add_uint32(buffer, &pid_tid_recibido->tid, cpu_log);
    buffer_add_uint32(buffer, &valor_tid_cancel, cpu_log);

    enviar_registros_memoria(registro_cpu, pid_tid_recibido, conexion_memoria, cpu_log);
    enviar_paquete_kernel(buffer, fd_conexion_dispatch, THREAD_CANCEL);

    program_counter_update(registro_cpu, cpu_log);

    sem_wait(&aviso_syscall);

    if (recibo_kernel_ok)
    {
        check_interrupt();
    } else {
        log_error(cpu_log, "ERROR EN execute_thread_cancel");
    }
}

void execute_mutex_create(t_contexto* registro_cpu, char* recurso){
    int length = strlen(recurso) + 1;

    t_buffer* buffer = buffer_create(
        sizeof(int)*2 + length
    );
    
    buffer_add_uint32(buffer, &pid_tid_recibido->pid, cpu_log);
    buffer_add_uint32(buffer, &pid_tid_recibido->tid, cpu_log);
    buffer_add_string(buffer, length, recurso, cpu_log);

    enviar_registros_memoria(registro_cpu, pid_tid_recibido, conexion_memoria, cpu_log);
    enviar_paquete_kernel(buffer, fd_conexion_dispatch, MUTEX_CREATE);

    program_counter_update(registro_cpu, cpu_log);

    sem_wait(&aviso_syscall);

    if (recibo_kernel_ok)
    {
        check_interrupt();
    } else {
        log_error(cpu_log, "ERROR EN execute_mutex_create");
    }
}

void execute_mutex_lock(t_contexto* registro_cpu, char* recurso){
    int length = strlen(recurso) + 1;

    t_buffer* buffer = buffer_create(
        sizeof(int)*2 + length
    );
    
    buffer_add_uint32(buffer, &pid_tid_recibido->pid, cpu_log);
    buffer_add_uint32(buffer, &pid_tid_recibido->tid, cpu_log);
    buffer_add_string(buffer, length, recurso, cpu_log);

    enviar_registros_memoria(registro_cpu, pid_tid_recibido, conexion_memoria, cpu_log);
    enviar_paquete_kernel(buffer, fd_conexion_dispatch, MUTEX_LOCK);
    program_counter_update(registro_cpu, cpu_log);

    sem_wait(&aviso_syscall);

    if (recibo_kernel_ok)
    {
        check_interrupt();
    } else {
        log_error(cpu_log, "ERROR EN execute_mutex_lock");
    }
}

void execute_mutex_unlock(t_contexto* registro_cpu, char* recurso){
    int length = strlen(recurso) + 1;

    t_buffer* buffer = buffer_create(
        sizeof(int)*2 + length
    );
    
    buffer_add_uint32(buffer, &pid_tid_recibido->pid, cpu_log);
    buffer_add_uint32(buffer, &pid_tid_recibido->tid, cpu_log);
    buffer_add_string(buffer, length, recurso, cpu_log);

    enviar_registros_memoria(registro_cpu, pid_tid_recibido, conexion_memoria, cpu_log);
    enviar_paquete_kernel(buffer, fd_conexion_dispatch, MUTEX_UNLOCK);
    program_counter_update(registro_cpu, cpu_log);

    sem_wait(&aviso_syscall);

    if (recibo_kernel_ok)
    {
        check_interrupt();
    } else {
        log_error(cpu_log, "ERROR EN execute_mutex_unlock");
    }
}

void execute_process_exit(t_contexto* registro_cpu){
    t_buffer* buffer = buffer_create(
        sizeof(int)*2
    );
    
    buffer_add_uint32(buffer, &pid_tid_recibido->pid, cpu_log);
    buffer_add_uint32(buffer, &pid_tid_recibido->tid, cpu_log);

    enviar_registros_memoria(registro_cpu, pid_tid_recibido, conexion_memoria, cpu_log);
    enviar_paquete_kernel(buffer, fd_conexion_dispatch, PROCESS_EXIT);

    log_warning(cpu_log, "Cambio de contexto de CPU a Kernel por PROCESS_EXIT");

    pthread_mutex_lock(&mutex_pid_tid);
        pid_tid_recibido = NULL;
    pthread_mutex_unlock(&mutex_pid_tid);

    sem_wait(&aviso_syscall);
    cpu_replanificar = true;
}

void execute_thread_exit(t_contexto* registro_cpu){
    t_buffer* buffer = buffer_create(
        sizeof(int)*2
    );
    
    buffer_add_uint32(buffer, &pid_tid_recibido->pid, cpu_log);
    buffer_add_uint32(buffer, &pid_tid_recibido->tid, cpu_log);

    enviar_registros_memoria(registro_cpu, pid_tid_recibido, conexion_memoria, cpu_log);
    enviar_paquete_kernel(buffer, fd_conexion_dispatch, THREAD_EXIT);

    log_warning(cpu_log, "Cambio de contexto de CPU a Kernel por THREAD_EXIT");

    pthread_mutex_lock(&mutex_pid_tid);
        pid_tid_recibido = NULL;
    pthread_mutex_unlock(&mutex_pid_tid);
    
    sem_wait(&aviso_syscall);
    cpu_replanificar = true;
}

bool recibir_aviso_syscall(int fd_conexion_kernel, t_log* log){
    t_buffer* buffer;
	uint32_t length = 0;
    inst_cpu codigo_instruccion = 0;

	buffer = buffer_recieve(fd_conexion_kernel);

	char* mensaje_syscall = buffer_read_string(buffer, &length);
    codigo_instruccion = buffer_read_uint32(buffer);

    char* string_codigo_instruccion = obtener_string_codigo_instruccion(codigo_instruccion);

    if (strcmp(mensaje_syscall, "REPLANIFICACION_KERNEL") == 0)
    {
        desalojo_kernel = true;
    }
    
    log_warning(log, "## La Syscall %s devolvió el mensaje de estado de ejecución en KERNEL: %s", string_codigo_instruccion, mensaje_syscall);
    free(mensaje_syscall);
	buffer_destroy(buffer);

    return true;
}
