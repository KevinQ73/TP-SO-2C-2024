#include <utils/servidor.h>

int iniciar_servidor(t_log* logger, const char* name, char* puerto) {
    int socket_servidor;
    struct addrinfo hints, *servinfo;

    // Inicializando hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // Recibe los addrinfo
    getaddrinfo(NULL, puerto, &hints, &servinfo);

    bool conecto = false;

    // Itera por cada addrinfo devuelto
    for (struct addrinfo *p = servinfo; p != NULL; p = p->ai_next) {
        socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (socket_servidor == -1) // fallo de crear socket
            continue;

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
            // Si entra aca fallo el bind
            close(socket_servidor);
            continue;
        }
        // Ni bien conecta uno nos vamos del for
        conecto = true;
        break;
    }

    if(!conecto) {
        free(servinfo);
        return 0;
    }

    listen(socket_servidor, SOMAXCONN); // Escuchando (hasta SOMAXCONN conexiones simultaneas)

    // Aviso al logger
    log_info(logger, "Escuchando en: %s (%s)\n", puerto, name);

    freeaddrinfo(servinfo); //free

    return socket_servidor;
}

int esperar_cliente(t_log* logger, const char* name, int socket_servidor) {
    int handshake;
    int resultOk = HANDSHAKE_OK;
    int resultError = HANDSHAKE_ERROR;

	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor, NULL, NULL);

    recv(socket_cliente, &handshake, sizeof(uint32_t), MSG_WAITALL);

	if(handshake == 1){
		send(socket_cliente, &resultOk, sizeof(uint32_t), NULL);
	}
	else{
		send(socket_cliente, &resultError, sizeof(uint32_t), NULL);
	}

	log_info(logger, "Se conecto: %s", name);

	return socket_cliente;
}

void manejar_error_accept(int errnum, t_log* log) {
    switch (errnum) {
        // Activar si hay algún socket no bloqueante: case EAGAIN:
        case EWOULDBLOCK:
            log_error(log, "No hay conexiones pendientes (EAGAIN/EWOULDBLOCK)\n");
            break;
        case EBADF:
            log_error(log, "El descriptor de archivo no es válido (EBADF)");
            abort();
        case ECONNABORTED:
            log_error(log, "Conexión abortada (ECONNABORTED)");
            break;  // Podemos continuar, intentar aceptar otra conexión
        case EFAULT:
            log_error(log, "Dirección de socket no válida (EFAULT)");
            abort();
        case EINTR:
            log_error(log, "accept interrumpido por señal (EINTR), reintentando");
            break;  // Reintentamos el accept
        case EINVAL:
            log_error(log, "El socket no está en modo escucha o addrlen inválido (EINVAL)");
            abort();
        case EMFILE:
            log_error(log, "Demasiados descriptores de archivos abiertos por el proceso (EMFILE)");
            abort();
        case ENFILE:
            log_error(log, "Demasiados archivos abiertos en el sistema (ENFILE)");
            abort();
        case ENOBUFS:
            log_error(log, "Falta de memoria o buffers de red. (ENOBUFS)");
            abort();
        case ENOMEM:
            log_error(log, "Falta de memoria del sistema (ENOBUFS/ENOMEM)");
            abort();
        case EPROTO:
            log_error(log, "Error en el protocolo de transporte (EPROTO)");
            abort();
        case EPERM:
            log_error(log, "Permisos insuficientes o filtro bloqueó la solicitud (EPERM)");
            abort();
        default:
            log_error(log, "Error desconocido en accept");
            abort();
    }
}

void terminar_modulo(int socket, t_log* logger, t_config* config)
{
	if (logger != NULL){
		log_destroy(logger);
	}
	if (config != NULL){
		config_destroy(config);
	}
	if(socket != 0){
		close(socket);
	}
}