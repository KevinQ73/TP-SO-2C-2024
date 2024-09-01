#include <kernel.h>

int main(int argc, char* argv[]) {

    //---------------------------- Iniciar archivos ----------------------------

    kernel_log = iniciar_logger("./files/kernel.log", "KERNEL", 1, LOG_LEVEL_DEBUG);

    kernel_config = iniciar_config("./files/kernel.config");

    kernel_registro = levantar_datos(kernel_config);

    // --------------------- Conexión como cliente de CPU ----------------------

    conexion_cpu_dispatch = crear_conexion(kernel_registro.ip_cpu, kernel_registro.puerto_cpu_dispatch);
    log_debug(kernel_log, "ME CONECTÉ A CPU DISPATCH");

    conexion_cpu_interrupt = crear_conexion(kernel_registro.ip_cpu, kernel_registro.puerto_cpu_interrupt);
    log_debug(kernel_log, "ME CONECTÉ A CPU INTERRUPT");

    // --------------------- Conexión como cliente de MEMORIA ----------------------

    //conexion_memoria = crear_conexion(kernel_registro.ip_memoria, kernel_registro.puerto_memoria);
    //log_debug(kernel_log, "ME CONECTÉ A MEMORIA");

    //FINALIZACION DEL MODULO

    log_debug(kernel_log, "TERMINANDO KERNEL");
    //terminar_modulo(conexion_cpu_interrupt, kernel_log, kernel_config)
    //terminar_modulo(conexion_cpu_dispatch, kernel_log, kernel_config)
    //terminar_modulo(conexion_memoria, kernel_log, kernel_config);
}