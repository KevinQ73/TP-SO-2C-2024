#include <utils/servidor.h>

int iniciar_servidor(char* puerto)
{
	int socket_servidor = 0;
	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, puerto, &hints, &servinfo);

	// Creamos el socket de escucha del servidor
	socket_servidor = socket(servinfo -> ai_family,
							 servinfo -> ai_socktype,						
							 servinfo -> ai_protocol);

	// Asociamos el socket a un puerto
	if (bind(socket_servidor, servinfo->ai_addr, servinfo-> ai_addrlen) != 0){
            perror("FALLO DEL BIND");
            exit(1);
    }

	// Escuchamos las conexiones entrantes
	listen(socket_servidor, SOMAXCONN);
	freeaddrinfo(servinfo);

	return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
	// Aceptamos un nuevo cliente
	int socket_cliente = 0;
	uint32_t handshake;
	uint32_t resultOk = 0;
	uint32_t resultError = -1;

	socket_cliente = accept(socket_servidor, NULL, NULL);

	recv(socket_cliente, &handshake, sizeof(uint32_t), MSG_WAITALL);

	if(handshake == 1){
		send(socket_cliente, &resultOk, sizeof(uint32_t), NULL);
	}
	else{
		send(socket_cliente, &resultError, sizeof(uint32_t), NULL);
	}

	return socket_cliente;
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