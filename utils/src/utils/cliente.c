#include <utils/cliente.h>

int crear_conexion(char *ip, char* puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_cliente = 0;

	socket_cliente = socket(server_info -> ai_family,
							server_info -> ai_socktype,
							server_info -> ai_protocol);
	
	// Ahora que tenemos el socket, vamos a conectarlo
	uint32_t handshake = 1;
	uint32_t result;

	connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);

	send(socket_cliente, &handshake, sizeof(uint32_t), 0);

	recv(socket_cliente, &result, sizeof(uint32_t), MSG_WAITALL);

	freeaddrinfo(server_info);

	return socket_cliente;
}