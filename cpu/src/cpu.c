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

    registros_cpu = inicializar_registros();

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
    while (flag_disconect)
    {
        int op = recibir_operacion(fd_conexion_dispatch);
        switch (op)
        {
        case PID_TID:
            pid_tid_recibido = recibir_paquete_kernel(fd_conexion_dispatch, cpu_log);
            log_debug(cpu_log, "PID RECIBIDO: %d, TID RECIBIDO: %d", pid_tid_recibido.pid, pid_tid_recibido.tid);
            ejecutar_hilo(pid_tid_recibido);
            break;
        
        case DESCONEXION:
            log_error(cpu_log, "Desconexion de Kernel - Dispatch");
            flag_disconect = false;
            break;
        default:
            log_warning(cpu_log, "Operacion desconocida de Kernel - Dispatch");
            break;
        }
    }
}

void atender_puerto_interrupt(){
    while (flag_disconect)
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
            log_debug(cpu_log, "ERROR EN ATENDER_PUERTO_INTERRUPT");
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
    t_dictionary* contexto_ejecucion = solicitar_contexto_ejecucion(pid_tid_recibido, conexion_memoria, cpu_log);

    actualizar_registros_cpu(registros_cpu, contexto_ejecucion, cpu_log);

    do
    {
        uint32_t pc = dictionary_get(registros_cpu, "PC");
        char* instruccion = fetch(pid_tid_recibido, &pc, conexion_memoria, cpu_log);

        char** instruccion_parseada = decode(instruccion);

        execute(registros_cpu, instruccion_parseada, cpu_log);

        string_array_destroy(instruccion_parseada);
        free(instruccion);

    } while (!interrupt_is_called);
}