#include <memoria.h>

int main(int argc, char* argv[]) {

    //---------------------------- Iniciar archivos ----------------------------

    memoria_log = iniciar_logger("./files/memoria.log", "MEMORIA", 1, LOG_LEVEL_DEBUG);

    memoria_config = iniciar_config("./files/memoria.config");

    memoria_registro = levantar_datos_memoria(memoria_config);

    // --------------------- Conexión como cliente de FILESYSTEM ----------------------

//    conexion_filesystem = crear_conexion(memoria_registro.ip_filesystem, memoria_registro.puerto_filesystem);
//    log_debug(memoria_log, "ME CONECTÉ A FILESYSTEM");

    // --------------------- Conexión como servidor CPU----------------------
//    fd_conexion_cpu = iniciar_servidor(memoria_registro.puerto_escucha);
//    log_debug(memoria_log,"SOCKET LISTEN LISTO PARA RECIBIR CLIENTES");
//
//    conexion_cpu = esperar_cliente(fd_conexion_cpu);

    // --------------------- Conexión como servidor KERNEL----------------------
    fd_conexion_kernel = iniciar_servidor(memoria_registro.puerto_escucha);
    log_debug(memoria_log,"SOCKET LISTEN LISTO PARA RECIBIR CLIENTES");

    conexion_kernel = esperar_cliente(fd_conexion_kernel);

    //FINALIZACION DEL MODULO

    log_debug(memoria_log, "TERMINANDO MEMORIA");
//    terminar_modulo(conexion_cpu, memoria_log, memoria_config);
//    terminar_modulo(conexion_kernel, memoria_log, memoria_config);
//    terminar_modulo(conexion_filesystem, memoria_log, memoria_config);


}
