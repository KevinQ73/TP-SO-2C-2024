#include <utils/serializacion.h>

/*----------------------------- Miscelaneo ------------------------------*/

char* nombres_registros[11] = {
        "PC",
        "AX",
        "BX",
        "CX",
        "DC",
        "EX",
        "FX",
        "GX",
        "HX",
		"BASE",
		"LIMITE"
    };

/*-----------------------------------------------------------------------*/

/*--------------------------- TADs de Buffer ----------------------------*/

t_buffer* buffer_create(uint32_t size){
	t_buffer* buffer = malloc(size);
	buffer->size = size;
	buffer->offset = 0;
	buffer->stream = NULL;
	return buffer;
}

void buffer_destroy(t_buffer* buffer){
	free(buffer->stream);
	free(buffer);
}

uint32_t buffer_size(t_buffer* buffer){
	return buffer->size;
}

void buffer_add(t_buffer* buffer, void* data, uint32_t size){
    // Redimensionamos el buffer
    buffer->size = buffer->offset + size;  // Incrementamos el tamaño del buffer
    buffer->stream = realloc(buffer->stream, buffer->size);  // Redimensionamos el stream
    
    // Copiamos los datos en la posición actual del buffer
    memcpy(buffer->stream + buffer->offset, data, size);
    
    // Avanzamos el offset
    buffer->offset += size;
}

void buffer_read(t_buffer* buffer, void* data, uint32_t size){
	// Copiamos size datos en data desde el buffer
	memcpy(data, buffer->stream + buffer->offset, size);
	// Avanzamos el offset
	buffer->offset += size;
}

void buffer_add_uint32(t_buffer* buffer, uint32_t* data, t_log* log){
	buffer_add(buffer, data, sizeof(uint32_t));
}

void buffer_add_string(t_buffer* buffer, uint32_t length, char* string, t_log* log){
	uint32_t length_string = length;
	// Indico el largo del string
	buffer_add_uint32(buffer, &length_string, log);

	//Agrego el string en el buffer
	buffer_add(buffer, string, length);
}

uint32_t buffer_read_uint32(t_buffer* buffer){
	uint32_t data = 0;
	buffer_read(buffer, &data, sizeof(uint32_t));

	return data;
}

char* buffer_read_string(t_buffer* buffer, uint32_t* length){
	// Leo el tamaño del string
	*length = buffer_read_uint32(buffer);

	// Malloqueo la cantidad de bytes necesarios para cada char, sin olvidarme del '\0'
	char* string = (char *)malloc(*length + 1);

	// Leo el string del buffer
	buffer_read(buffer, string, *length);

	// Me aseguro que el último byte sea el caracter nulo
	string[*length] = '\0';

	return string;
}

t_buffer* buffer_recieve(int socket_cliente){

	t_buffer* buffer = buffer_create(sizeof(t_buffer));
	int err;

	err = recv(socket_cliente, &(buffer->size), sizeof(uint32_t), MSG_WAITALL);
	printf("\n %d", err);
	buffer->stream = malloc(buffer->size);
	recv(socket_cliente, buffer->stream, buffer->size, 0);

	return buffer;
}

/* 
void buffer_add_tlist(t_buffer* buffer, t_list* list, t_log* log){
	// Añadimos al buffer el tamaño total de la lista
	uint32_t size_lista = malloc(list);
	buffer_add_uint32(buffer, &size_lista, log);

	buffer_add(buffer, list, size_lista);

	// Obtengo el tamaño del valor a serializar
	void* data = malloc(size);

	// Iteramos la lista, añadiendo uno por uno todos los elementos de la misma
	for (int i = 0; i < size_lista; i++)
	{
		data = list_get(list, i);
		int x = *(int*)data;
		log_debug(log, "DATA: %d", x);
		buffer_add(buffer, data, size);
	}
}

void buffer_add_tcbs(t_buffer* buffer, t_list* list, t_log* log){
	buffer_add_tlist(buffer, sizeof(t_tcb), list, log);
}

void buffer_add_mutex(t_buffer* buffer, t_list* list, t_log* log){
	buffer_add_tlist(buffer, sizeof(sem_t), list, log);
}

void buffer_add_tids(t_buffer* buffer, t_list* list, t_log* log){
	buffer_add_tlist(buffer, sizeof(uint32_t), list, log);
}

t_list* buffer_read_tlist(t_buffer* buffer, t_list* list){
	//Leemos el tamaño de la lista
	uint32_t size_lista = buffer_read_uint32(buffer);

	// Armamos mini buffer para guardar lo leido
	
	list = malloc(size_lista);
	buffer_read(buffer, list, size_lista);

	// Ahora que sabemos el tamaño, añadimos uno por uno los elementos del buffer en la lista
	for (int i = 0; i < *length_lista; i++)
	{
		void* data = malloc(size_data);
		buffer_read(buffer, data, size_data);
		int x = *(int*)data;
		printf("DATA: %d", x);
		list_add(list, data);
		free(data);
	}	

	// Devolvemos la lista
	return list;
}

t_list* buffer_read_tcbs(t_buffer* buffer){
	// Creamos variables auxiliares para usarlos como direcciones de memoria
	uint32_t largo_lista = 0;
	t_list* lista_tcbs = list_create();

	//Leemos del buffer la lista de tcbs
	lista_tcbs = buffer_read_tlist(buffer, lista_tcbs, &largo_lista, sizeof(t_tcb));
	lista_tcbs = buffer_read_tlist(buffer, lista_tcbs, &largo_lista, sizeof(t_tcb));
	
	//Devolvemos la lista
	return lista_tcbs;
}

t_list* buffer_read_mutex(t_buffer* buffer){
	// Creamos variables auxiliares para usarlos como direcciones de memoria
	uint32_t largo_lista = 0;
	t_list* lista_mutex = list_create();

	//Leemos del buffer la lista de mutex
	lista_mutex = buffer_read_tlist(buffer, lista_mutex, &largo_lista, sizeof(sem_t));

	//Devolvemos la lista
	return lista_mutex;
}

t_list* buffer_read_tids(t_buffer* buffer){
	uint32_t largo_lista = 0;
	t_list* lista_tids = list_create();

	//Leemos del buffer la lista de tids
	lista_tids = buffer_read_tlist(buffer, lista_tids, &largo_lista, sizeof(uint32_t));

	//Devolvemos la lista
	return lista_tids;
}
*/

/*-----------------------------------------------------------------------*/

/*--------------------------- TADs de Paquete ---------------------------*/

t_paquete* crear_paquete(cod_inst instruccion){

	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = instruccion;
	paquete->buffer = malloc(sizeof(t_buffer));
	return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio){

	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void* serializar_paquete(t_paquete* paquete, int bytes){

	void* magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

void enviar_paquete(t_paquete* paquete, int socket_cliente){

	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

t_list* recibir_paquete(int socket_cliente){
	int desplazamiento = 0;
	t_buffer* buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = buffer_recieve(socket_cliente);
	while(desplazamiento < buffer->size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		void* valor = malloc(tamanio);
		memcpy(valor, buffer+desplazamiento, tamanio);
		desplazamiento+=tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}

void eliminar_paquete(t_paquete* paquete){

	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

/*
void agregar_a_paquete_pcb(t_paquete* paquete_a_enviar, t_pcb* pcb, t_log* log){

    void* pcb_a_enviar;
    pcb_a_enviar = serializar_pcb(pcb, log);

    agregar_a_paquete(paquete_a_enviar, pcb_a_enviar, sizeof(pcb_a_enviar));
}
*/

/*-----------------------------------------------------------------------*/

/*-------------- Funciones de envio de datos entre módulos --------------*/

int recibir_operacion(int socket_cliente){
    
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void enviar_mensaje(char* mensaje, int socket_server, t_log* log){
	uint32_t length = strlen(mensaje) + 1;
    t_paquete* paquete = crear_paquete(MENSAJE);

	t_buffer* buffer;

	buffer = buffer_create(length);
	buffer_add_string(buffer, length, mensaje, log);
	log_debug(log, "STRING A ENVIAR: %s", mensaje);

	paquete->buffer = buffer;

	enviar_paquete(paquete, socket_server);
	eliminar_paquete(paquete);
}

char* recibir_mensaje(int socket_cliente, t_log* log){
	t_buffer* buffer;
	uint32_t length = 0;
	char* string;
	int op = recibir_operacion(socket_cliente);

	if (op == MENSAJE)
    {
        log_debug(log, "SE RECIBIÓ UN MENSAJE");
    } else {
        log_warning(log, "ERROR EN EL MENSAJE ENVIADO");
        abort();
    }

	buffer = buffer_recieve(socket_cliente);

	string = buffer_read_string(buffer, &length);

	buffer_destroy(buffer);
	return string;
}

void enviar_pid_tid(uint32_t* pid, uint32_t* tid, int socket_servidor, t_log* log){
	t_buffer* buffer;
	
	t_paquete* paquete = crear_paquete(PID_TID);
	buffer = buffer_create(
		(sizeof(uint32_t)*2)
		);

	buffer_add_uint32(buffer, pid, log);
	log_debug(log, "PID A ENVIAR: %d", *pid);
    buffer_add_uint32(buffer, tid, log);
	log_debug(log, "TID A ENVIAR: %d", *tid);

	paquete->buffer = buffer;
	enviar_paquete(paquete, socket_servidor);

	eliminar_paquete(paquete);
}

t_pid_tid recibir_pid_tid(int socket_cliente, t_log* log){
	uint32_t pid = 0;
    uint32_t tid = 0;

	t_buffer* buffer;
    buffer = buffer_recieve(socket_cliente);

	pid = buffer_read_uint32(buffer);
    tid = buffer_read_uint32(buffer);

    log_debug(log, "EL PID ES: %d Y EL TID ES: %d", pid, tid);

	t_pid_tid pid_tid;
	pid_tid.pid = pid;
	pid_tid.tid = tid;

	buffer_destroy(buffer);
	return pid_tid;
}

cod_inst obtener_codigo_instruccion(char* operacion){

    // SET
    if (strcmp(operacion, "SET") == 0){
        return SET;
    }

    // READ_MEM
    else if (strcmp(operacion, "READ_MEM") == 0){
        return READ_MEM;
    }

    // WRITE_MEM
    else if (strcmp(operacion, "WRITE_MEM") == 0){
        return WRITE_MEM;
    }

    // SUM
    else if (strcmp(operacion, "SUM") == 0){
        return SUM;
    }

    // SUB
    else if (strcmp(operacion, "SUB") == 0){
        return SUB;
    }

    // JNZ
    else if (strcmp(operacion, "JNZ") == 0){
        return JNZ;
    }

    // LOG
    else if (strcmp(operacion, "LOG") == 0){
        return LOG;
    }

    // DUMP_MEMORY
    else if (strcmp(operacion, "DUMP_MEMORY") == 0){
        return DUMP_MEMORY;
    }

    // IO
    else if (strcmp(operacion, "IO") == 0){
        return IO;
    }

    // PROCESS_CREATE
    else if (strcmp(operacion, "PROCESS_CREATE") == 0){
        return PROCESS_CREATE;
    }

    // THREAD_CREATE
    else if (strcmp(operacion, "THREAD_CREATE") == 0){
        return THREAD_CREATE;
    }

    // THREAD_JOIN
    else if (strcmp(operacion, "THREAD_JOIN") == 0){
        return THREAD_JOIN;
    }

    // THREAD_CANCEL
    else if (strcmp(operacion, "THREAD_CANCEL") == 0){
        return THREAD_CANCEL;
    }

    // MUTEX_CREATE
    else if (strcmp(operacion, "MUTEX_CREATE") == 0){
        return MUTEX_CREATE;
    }

    // MUTEX_LOCK
    else if (strcmp(operacion, "MUTEX_LOCK") == 0){
        return MUTEX_LOCK;
    }

	//MUTEX_UNLOCK
    else if (strcmp(operacion, "MUTEX_UNLOCK") == 0){
        return MUTEX_UNLOCK;
    }

    // THREAD_EXIT
    else if (strcmp(operacion, "THREAD_EXIT") == 0){
        return THREAD_EXIT;
    }

	// PROCESS_EXIT
	else if (strcmp(operacion, "PROCESS_EXIT") == 0){
        return PROCESS_EXIT;
    }
	// El código no es válido
    else {
       return ERROR_CODE;
    }
	/*
	// EXIT
    else if (strcmp(operacion, "EXIT") == 0){
        return EXIT;
    }

	// LADO CONSOLA

	// EJECUTAR_SCRIPT
    else if (strcmp(operacion, "EJECUTAR_SCRIPT") == 0){
        return EJECUTAR_SCRIPT;
    }

	// INICIAR_PROCESO
    else if (strcmp(operacion, "INICIAR_PROCESO") == 0){
        return INICIAR_PROCESO;
    }

	// FINALIZAR_PROCESO
    else if (strcmp(operacion, "FINALIZAR_PROCESO") == 0){
        return FINALIZAR_PROCESO;
    }

	// DETENER_PLANIFICACION
    else if (strcmp(operacion, "DETENER_PLANIFICACION") == 0){
        return DETENER_PLANIFICACION;
    }

	// INICIAR_PLANIFICACION
    else if (strcmp(operacion, "INICIAR_PLANIFICACION") == 0){
        return INICIAR_PLANIFICACION;
    }

	// MULTIPROGRAMACION
    else if (strcmp(operacion, "MULTIPROGRAMACION") == 0){
        return MULTIPROGRAMACION;
    }

	// PROCESO_ESTADO
    else if (strcmp(operacion, "PROCESO_ESTADO") == 0){
        return PROCESO_ESTADO;
    }
	*/
}

/*

void* serializar_pcb(t_pcb* data, t_log* log){ // Nota para el autor: Probar si es necesario agregar t_buffer* buffer como parámetro
	// Creamos el buffer que vamos a utilizar
	t_buffer* buffer = buffer_create(sizeof(t_buffer));
	
	// Creamos variables auxiliares para usarlos como direcciones de memoria
	//uint32_t tid = 0;
	//sem_t* mutex = malloc(sizeof(sem_t));

	// Añadimos uno por uno los elementos que componen el PCB
	buffer_add_uint32(buffer, &data->pid, log);						// PID
	buffer_add_tids(buffer, data->tids, log);					// Lista de TIDs
	buffer_add_mutex(buffer, data->mutex_asociados, log);		// Lista de mutex
	buffer_add_uint32(buffer, &data->program_counter, log);			// Program Counter
	buffer_add_uint32(buffer, &data->estado, log);					// Estado del PCB

	return buffer;
}

t_pcb* deserializar_pcb(t_buffer* buffer){
	// Reservamos memoria para el PCB
	t_pcb* pcb = malloc(sizeof(t_pcb));

	// Leemos cada elemento del PCB
	pcb->pid = buffer_read_uint32(buffer);						// PID
	pcb->tids = buffer_read_tids(buffer);						// Lista de TIDs
	pcb->mutex_asociados = buffer_read_mutex(buffer);			// Lista de mutex
	pcb->program_counter = buffer_read_uint32(buffer);			// Program Counter
	pcb->estado = buffer_read_uint32(buffer);					// Estado del PCB

	// Devolvemos el PCB
	return pcb;
}

void enviar_pcb(t_pcb* pcb_a_enviar, estado_proceso estado_pcb, cod_inst codigo_instruccion, int socket_destino, t_log* log){
	pcb_a_enviar->estado = estado_pcb;

	t_paquete* paquete_pcb = crear_paquete(codigo_instruccion);

    agregar_a_paquete_pcb(paquete_pcb, pcb_a_enviar, log);
    enviar_paquete(paquete_pcb, socket_destino);

    eliminar_paquete(paquete_pcb);
}

t_pcb* recibir_pcb(int socket_cliente){
	t_buffer* buffer = malloc(sizeof(t_buffer));
	buffer = buffer_recieve(socket_cliente);

	t_pcb* pcb = malloc(sizeof(t_pcb));

	pcb = deserializar_pcb(buffer);

	return pcb;
}

*/