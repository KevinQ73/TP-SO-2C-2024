#include <filesystem.h>

int main(int argc, char* argv[]) {

    filesystem_log = iniciar_logger("./files/filesystem.log", "FILESYSTEM", 1, LOG_LEVEL_DEBUG);

    filesystem_config = iniciar_config("./files/filesystem.config");

    filesystem_registro = levantar_datos(filesystem_config);

    // --------------------- Conexión como servidor de MEMORIA ----------------------

    fd_escucha_memoria = iniciar_servidor(filesystem_log, "FILESYSTEM", filesystem_registro.puerto_escucha);
    log_debug(filesystem_log,"SOCKET LISTEN LISTO PARA RECIBIR CLIENTES");

    log_debug(filesystem_log, "Esperando MEMORIA...");
    fd_conexion_memoria = esperar_cliente(filesystem_log, "MEMORIA", fd_escucha_memoria);

    log_debug(filesystem_log, "TERMINANDO MEMORIA");
}