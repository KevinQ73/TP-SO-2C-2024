#include <filesystem.h>

int main(int argc, char* argv[]) {

    filesystem_log = iniciar_logger("./files/filesystem.log", "FILESYSTEM", 1, LOG_LEVEL_DEBUG);

    filesystem_config = iniciar_config(argv[1]);

    filesystem_registro = levantar_datos(filesystem_config);

    // --------------------- Conexión como servidor de MEMORIA ----------------------

    fd_escucha_memoria = iniciar_servidor(filesystem_log, "FILESYSTEM", filesystem_registro.puerto_escucha);
    log_debug(filesystem_log,"SOCKET LISTEN LISTO PARA RECIBIR CLIENTES");

    atender_memoria();

    //log_debug(filesystem_log, "Esperando MEMORIA...");
    //fd_conexion_memoria = esperar_cliente(filesystem_log, "MEMORIA", fd_escucha_memoria);

    log_debug(filesystem_log, "TERMINANDO MEMORIA");
}

void atender_memoria(){
    bool liberar_hilo_memoria = true;

    while(liberar_hilo_memoria){

        log_info(filesystem_log, "Esperando solicitud de MEMORIA...\n");
        fd_conexion_memoria = esperar_cliente(filesystem_log, "MEMORIA", fd_escucha_memoria);

        if (fd_conexion_memoria < 0) {
            // Llamar a la función de manejo de errores
            manejar_error_accept(errno, filesystem_log);
            if (errno == EINTR || errno == ECONNABORTED) {
                continue;  // Reintentar la operación en caso de interrupción o conexión abortada
            } else {
                abort();
            }
        } else {
            log_info(filesystem_log, "## Memoria Conectado - FD del socket: <%d>", fd_conexion_memoria);
            pthread_create(&hilo_memoria, NULL, atender_solicitudes, (void*)fd_conexion_memoria);
            pthread_detach(hilo_memoria);
        }
    }
}

void* atender_solicitudes(void* fd_conexion){
    int fd_memoria = (int*)fd_conexion;
    log_debug(filesystem_log, "FD RECIBIDO: %d", fd_memoria);
    t_buffer* buffer;
    //uint32_t pid;
    //int size = 0;
    //char* path = string_new();

    int operacion_memoria = recibir_operacion(fd_memoria);

    if(operacion_memoria == DUMP_MEMORY){
        log_info(filesystem_log, "MEMORY DUMP LEIDO");
    } else {
        log_error(filesystem_log, "OPERACIÓN ERRONEA");
        abort();
    }

    buffer = buffer_recieve(fd_memoria);

    uint32_t pid = buffer_read_uint32(buffer);
    uint32_t tid = buffer_read_uint32(buffer);
    uint32_t size = buffer_read_uint32(buffer);

    log_debug(filesystem_log, "PID RECIBIDO: %d, TID: %d, SIZE: %d", pid, tid, size);

    enviar_mensaje("OK_FS", fd_memoria, filesystem_log);
}


