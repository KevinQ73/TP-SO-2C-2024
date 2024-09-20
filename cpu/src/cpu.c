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


    //iniciar_cpu();

    // ------------------------ FINALIZACIÓN CPU -------------------------

    log_debug(cpu_log, "TERMINANDO CPU");
    //terminar_modulo(fd_conexion_dispatch, cpu_log, cpu_config);
    //terminar_modulo(fd_conexion_interrupt, cpu_log, cpu_config);

    return EXIT_SUCCESS;
}

void atender_puerto_dispatch(){
    while (flag_dispatch)
    {
        int op = recibir_operacion(fd_conexion_dispatch);
        switch (op)
        {
        case PID_TID:
            pid_tid_recibido = recibir_paquete_kernel(fd_conexion_dispatch, cpu_log);
            log_debug(cpu_log, "PID RECIBIDO: %d, TID RECIBIDO: %d", pid_tid_recibido.pid, pid_tid_recibido.tid);
            ejecutar_proceso(pid_tid_recibido);
            break;
        
        case DESCONEXION:
            log_error(cpu_log, "Desconexion de Kernel - Dispatch");
            flag_dispatch = false;
            break;
        default:
            log_warning(cpu_log, "Operacion desconocida de Kernel - Dispatch");
            break;
        }
    }
}

void atender_puerto_interrupt(){

}

void ejecutar_proceso(t_pid_tid pid_tid_recibido){

}