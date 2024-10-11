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

    //iniciar multihilos
    //FINALIZACION DEL MODULO

    log_debug(memoria_log, "TERMINANDO MEMORIA");
    //terminar_modulo(conexion_cpu, memoria_log, memoria_config);
    //terminar_modulo(conexion_kernel, memoria_log, memoria_config);
    //terminar_modulo(conexion_filesystem, memoria_log, memoria_config);

}
void atender_kernel(){
    // recibir paquete de kernel
    t_paquete* paquete = recibir_paquete(fd_conexion_kernel);
    int op = recibir_operacion(fd_conexion_kernel);

    if(op == CREAR_PROCESO){
        pthread_create(hiloCrearProceso,NULL,crear_proceso(),NULL);
        pthread_detach(hiloCrearProceso);
    }
    else if(op == FINALIZAR_PROCESO){
        pthread_create(hiloFinalizarProceso,NULL,finalizar_proceso(),NULL);
        pthread_detach(hiloFinalizarProceso);
    }  
}
// PETICION 1: recibo pid para inicializar el pcb
// PETICION 2: recibo pid para crear el proceso
void* crear_proceso(){
    enviar_mensaje("ESPACIO_ASIGNADO", fd_conexion_kernel, memoria_log);
}
// PETICION 3: recibo pid para finalizar el proceso
void* finalizar_proceso(){
    enviar_mensaje("FINALIZACION_ACEPTADA", fd_conexion_kernel, memoria_log);
}
