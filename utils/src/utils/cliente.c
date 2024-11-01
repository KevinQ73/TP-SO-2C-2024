#include <utils/cliente.h>

int crear_conexion(t_log* logger, char* ip, char* puerto){
    int codigo_handshake = 1;
    int result;

    struct addrinfo hints, *servinfo;

    // Init de hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // Recibe addrinfo
    getaddrinfo(ip, puerto, &hints, &servinfo);

    // Crea un socket con la informacion recibida (del primero, suficiente)
    int socket_cliente = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    // Fallo en crear el socket
    if(socket_cliente == -1) {
        log_error(logger, "Error creando el socket para %s:%s", ip, puerto);
        return 0;
    }

    // Error conectando
    if(connect(socket_cliente, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        log_error(logger, "Error al conectar (a %s)\n", "a");
        freeaddrinfo(servinfo);
        return 0;
    } else
        send(socket_cliente, &codigo_handshake, sizeof(int), NULL);
	    recv(socket_cliente, &result, sizeof(uint32_t), MSG_WAITALL);
        log_info(logger, "Cliente conectado en %s:%s (a %s)\n", ip, puerto, "a");

    freeaddrinfo(servinfo); //free

    return socket_cliente;
}