#include <cpu.h>

int main(int argc, char* argv[]) {

    //---------------------------- Iniciar archivos ----------------------------

    cpu_log = iniciar_logger("./files/cpu.log", "CPU", 1, LOG_LEVEL_DEBUG);

    cpu_config = iniciar_config("./files/cpu.config");

    cpu_registro = levantar_datos(cpu_config);
    log_debug(cpu_log, "LEVANTÉ DATOS");
    // --------------------- Conexión como cliente de MEMORIA ----------------------

    //conexion_memoria = crear_conexion(cpu_registro.ip_memoria, cpu_registro.puerto_memoria);
    //log_debug(kernel_log, "ME CONECTÉ A MEMORIA");

    // --------------------- Conexión como servidor de KERNEL ----------------------

    //fd_escucha_dispatch = iniciar_servidor(cpu_registro.puerto_cpu_dispatch);
    fd_escucha_interrupt = iniciar_servidor(cpu_registro.puerto_cpu_interrupt);
    log_debug(cpu_log,"SOCKET LISTEN LISTO PARA RECIBIR CLIENTES");

    //fd_conexion_dispatch = esperar_cliente(fd_escucha_dispatch);
    fd_conexion_interrupt = esperar_cliente(fd_escucha_interrupt);

    log_debug(cpu_log, "TERMINANDO CPU");
    //terminar_modulo(fd_conexion_dispatch, cpu_log, cpu_config);
    //terminar_modulo(fd_conexion_interrupt, cpu_log, cpu_config);
}