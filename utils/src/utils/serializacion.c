#include <utils/serializacion.h>

/*-------------------TADs de Buffer para serializar más fácil-------------*/

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

void buffer_add(t_buffer* buffer, void* data, uint32_t size){

    if (buffer->offset + size > buffer->size) {
        // Redimensionamos el buffer en caso de ser necesario
        buffer->size = buffer->offset + size;  // Incrementamos el tamaño del buffer
        buffer->stream = realloc(buffer->stream, buffer->size);  // Redimensionamos el stream
    }
    
    // Copiamos los datos en la posición actual del buffer
    memcpy(buffer->stream + buffer->offset, data, size);
    
    // Avanzamos el offset
    buffer->offset += size;
}

void buffer_read(t_buffer* buffer, void* data, uint32_t size){
	
	// Copiamos size datos en data desde el buffer
	memcpy(data, buffer->stream, size);
	// Avanzamos el offset
	buffer->offset = size;
}

void buffer_add_uint32(t_buffer* buffer, uint32_t data){
	buffer_add(buffer, &data, sizeof(uint32_t));
}

uint32_t buffer_read_uint32(t_buffer* buffer){
	uint32_t data = 0;
	buffer_read(buffer, &data, sizeof(uint32_t));

	return data;
}

void buffer_add_string(t_buffer* buffer, uint32_t length, char* string){
	// Indico el largo del string
	buffer_add_uint32(buffer, length);

	//Agrego el string en el buffer
	buffer_add(buffer, string, length);
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

void buffer_add_tlist(t_buffer* buffer, void* data, t_list* list){
	// Añadimos al buffer el tamaño total de la lista
	uint32_t size_lista = list_size(list);
	buffer_add_uint32(buffer, size_lista);

	// Iteramos la lista, añadiendo uno por uno todos los elementos de la misma
	for (int i = 0; i < size_lista; i++)
	{
		data = list_get(list, i);
		buffer_add(buffer, data, (uint32_t)sizeof(data));
	}
}

void buffer_add_tids(t_buffer* buffer, t_tid* tid, t_list* list){
	buffer_add_tlist(buffer, tid, list);
}

void buffer_add_mutex(t_buffer* buffer, sem_t* mutex, t_list* list){
	buffer_add_tlist(buffer, mutex, list);
}

t_list* buffer_read_tlist(t_buffer* buffer, void* data, t_list* list, uint32_t* length_lista, uint32_t size_data){
	//Leemos el tamaño de la lista
	*length_lista = buffer_read_uint32(buffer);

	// Ahora que sabemos el tamaño, añadimos uno por uno los elementos del buffer en la lista
	for (int i = 0; i < *length_lista; i++)
	{
		buffer_read(buffer, data, size_data);
		list_add_in_index(list, i, data);
	}
	// Devolvemos la lista
	return list;
}

t_list* buffer_read_tids(t_buffer* buffer){
	// Creamos variables auxiliares para usarlos como direcciones de memoria
	uint32_t largo_lista = 0;
	t_tid* tid = malloc(sizeof(t_tid));
	t_list* lista_tids = list_create();

	//Leemos del buffer la lista de tids
	lista_tids = buffer_read_tlist(buffer, tid, lista_tids, &largo_lista, sizeof(t_tid));

	//Devolvemos la lista
	return lista_tids;
}

t_list* buffer_read_mutex(t_buffer* buffer){
	// Creamos variables auxiliares para usarlos como direcciones de memoria
	uint32_t largo_lista = 0;
	sem_t* mutex = malloc(sizeof(sem_t));
	t_list* lista_mutex = list_create();

	//Leemos del buffer la lista de mutex
	lista_mutex = buffer_read_tlist(buffer, mutex, lista_mutex, &largo_lista, sizeof(sem_t));

	//Devolvemos la lista
	return lista_mutex;
}

void* serializar_pcb(t_pcb* data){ // Nota para el autor: Probar si es necesario agregar t_buffer* buffer como parámetro
	// Creamos el buffer que vamos a utilizar
	t_buffer* buffer = buffer_create(sizeof(t_pcb));
	
	// Creamos variables auxiliares para usarlos como direcciones de memoria
	t_tid* tid = malloc(sizeof(t_tid));
	sem_t* mutex = malloc(sizeof(sem_t));

	// Añadimos uno por uno los elementos que componen el PCB
	buffer_add_uint32(buffer, data->pid);						// PID
	buffer_add_tids(buffer, tid, data->tids);					// Lista de TIDs
	buffer_add_mutex(buffer, mutex, data->mutex_asociados);		// Lista de mutex
	buffer_add_uint32(buffer, data->program_counter);			// Program Counter
	buffer_add_uint32(buffer, data->estado);					// Estado del PCB

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

/*------------------------------------------------------------------------*/

t_paquete* crear_paquete(cod_inst instruccion){

	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = instruccion;
	paquete->buffer = buffer_create(sizeof(t_buffer));
	return paquete;
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

void enviar_mensaje(char* mensaje, int socket_cliente){

    t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->offset = 0;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

void* recibir_buffer(int* size, int socket_cliente){
    
	void* buffer;
	int err;

	err = recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	printf("\n %d", err);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void recibir_mensaje(int socket_cliente, t_log* logger_servidor){

	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger_servidor, "Me llego el mensaje %s", buffer);
	free(buffer);
}

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

void enviar_paquete(t_paquete* paquete, int socket_cliente){

	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

t_list* recibir_paquete(int socket_cliente){

	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		char* valor = malloc(tamanio);
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

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio){

	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void agregar_a_paquete_pcb(t_paquete* paquete_a_enviar, t_pcb* pcb){

    void* pcb_a_enviar;
    pcb_a_enviar = serializar_pcb(pcb);

    agregar_a_paquete(paquete_a_enviar, pcb_a_enviar, sizeof(pcb_a_enviar));
}

void enviar_pcb(t_pcb* pcb_a_enviar, estado_proceso estado_pcb, cod_inst codigo_instruccion, int socket_destino){
	pcb_a_enviar->estado = estado_pcb;

	t_paquete* paquete_pcb = crear_paquete(codigo_instruccion);

    agregar_a_paquete_pcb(paquete_pcb, pcb_a_enviar);
    enviar_paquete(paquete_pcb, socket_destino);

    eliminar_paquete(paquete_pcb);
}

t_pcb* recibir_pcb(int socket_cliente){
	int size;
	void* buffer;
	
	buffer = recibir_buffer(&size, socket_cliente);

	t_pcb* pcb = malloc(sizeof(t_pcb));

	pcb = deserializar_pcb(buffer);

	return pcb;
}