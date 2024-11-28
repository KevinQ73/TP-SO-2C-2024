#include <filesystem.h>

int main(int argc, char* argv[]) {

    filesystem_log = iniciar_logger("./files/filesystem.log", "FILESYSTEM", 1, LOG_LEVEL_DEBUG);

    filesystem_config = iniciar_config(argv[1]);

    filesystem_registro = levantar_datos(filesystem_config);

	obtener_datos_filesystem();

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
		t_paquete *recibido = recibir_paquete(fd_memoria);
		buffer = buffer_recieve(fd_memoria);

		uint32_t tamanio_buffer=buffer_read_uint32(buffer);
	
		char* nombre = buffer_read_string(buffer,tamanio_buffer);
		uint32_t tamanio = buffer_read_uint32(buffer);
		void* contenido = buffer_read_string(buffer,tamanio_buffer);

		if(dump_memory(nombre, tamanio, contenido)){
			enviar_mensaje("OK_FS", fd_memoria, filesystem_log);
		}else {
			enviar_mensaje("ERROR", fd_memoria, filesystem_log);
		}
    } else {
        log_error(filesystem_log, "OPERACIÓN ERRONEA");
        abort();
    }

	/*
    buffer = buffer_recieve(fd_memoria);

    uint32_t pid = buffer_read_uint32(buffer);
    uint32_t tid = buffer_read_uint32(buffer);
    uint32_t size = buffer_read_uint32(buffer);

    log_debug(filesystem_log, "PID RECIBIDO: %d, TID: %d, SIZE: %d", pid, tid, size);
	*/
}

//Funciones del dump_memory
int dump_memory(char* nombre_archivo, int tamanio, void* contenido){
	int bloques_totales = cantidad_bloques(tamanio) + 1;

	u_int32_t* puntero_bloques = buscar_bloques_bitmap(bloques_totales);

	if(puntero_bloques == NULL || ((filesystem_registro.block_size/sizeof(u_int32_t)) < cantidad_bloques(tamanio))){
		printf("No hay espacio suficiente para almacenar los bloques de datos\n");
        //Aca debo enviar mensaje a Memoria de que no se puede hacer DUMP MEMORY
		return EXIT_FAILURE;
	}else{
		asignar_bloques_bitmap(puntero_bloques, bloques_totales);
		log_debug(filesystem_log, "## Bloque asignado: <%i> - Archivo: <%s> - Bloques Libres: <%i>\n", puntero_bloques[0], nombre_archivo, bloques_libres);
		escribir_datos_bloque(nombre_archivo, puntero_bloques, bloques_totales, contenido);
		crear_metadata(nombre_archivo, puntero_bloques[0], tamanio);
		log_debug(filesystem_log, "## Archivo Creado: <%s> - Tamaño: <%i>\n", nombre_archivo, tamanio);
		log_debug(filesystem_log, "## Fin de solicitud - Archivo: <%s>\n", nombre_archivo);

		free(puntero_bloques);

		return EXIT_SUCCESS;
	}
}

//Funciones generales para bitmap y bloques
char* ruta_absoluta(char* ruta_relativa){
	int tamanio_ruta = strlen(filesystem_registro.path_mount_dir) + strlen(ruta_relativa) + 2;
	char* ruta = malloc(tamanio_ruta*sizeof(char));

	strcpy(ruta, filesystem_registro.path_mount_dir);
	strcat(ruta, "/");
	strcat(ruta, ruta_relativa);

	return ruta;
}

void obtener_datos_filesystem(){
	obtener_archivo("bloques.dat", (void*) crear_bloques, (void*) abrir_bloques);
	obtener_archivo("bitmap.dat", (void*) crear_bitmap, (void*) abrir_bitmap);
}

int obtener_archivo(char* ruta, void (*tipo_creacion)(int), void (*tipo_apertura)(int)){
	char* ruta_archivo = ruta_absoluta(ruta);
	if(access(ruta_archivo, F_OK) == -1){
		crear_archivo(ruta, (*tipo_creacion));
	}else{
		abrir_archivo(ruta, (*tipo_apertura));
	}

	free(ruta_archivo);

	return EXIT_SUCCESS;
}

int crear_archivo(char* ruta, void (*tipo_Archivo)(int)){
	int fd;

	char* ruta_archivo = ruta_absoluta(ruta);

	if((fd = open(ruta_archivo, O_CREAT|O_RDWR|O_TRUNC, S_IRWXU)) == -1){
		perror("No se pudo crear el archivo!!!\n");
		return EXIT_FAILURE;
	}else{
		(*tipo_Archivo)(fd);
		close(fd);
	}

	free(ruta_archivo);

	return EXIT_SUCCESS;
}


int abrir_archivo(char* ruta, void (*tipo_Archivo)(int)){
	int fd;

	char* ruta_archivo = ruta_absoluta(ruta);

	if((fd = open(ruta_archivo, O_RDWR)) == -1){
		perror("No se pudo abrir el archivo!!!\n");
		return EXIT_FAILURE;
	}else{
		(*tipo_Archivo)(fd);
		close(fd);
	}

	free(ruta_archivo);

	return EXIT_SUCCESS;
}

//Funciones de bloques.dat
void crear_bloques(int file_descriptor){
	int tamanio_archivo = filesystem_registro.block_count * filesystem_registro.block_size;

	ftruncate(file_descriptor, tamanio_archivo);

	struct stat buf;

	fstat(file_descriptor, &buf);
	buffer_bloques = mmap(NULL, buf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, file_descriptor, 0);

	int posicion = 0;
	for(int i=0;i<tamanio_archivo;i++){
		memset(buffer_bloques + posicion, 0x00, sizeof(char));
		posicion++;
	}
	msync(buffer_bloques, buf.st_size, MS_SYNC);

}

void abrir_bloques(int file_descriptor){
	struct stat buf;

	fstat(file_descriptor, &buf);
	buffer_bloques = mmap(NULL, buf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, file_descriptor, 0);

}

int inicio_bloque(int n_bloque){
	int bytes;

	bytes = n_bloque * filesystem_registro.block_size;

	return bytes;
}

int cantidad_bloques(int tamanio){
	int cantidad = ((tamanio+(filesystem_registro.block_size-1))/ filesystem_registro.block_size); //Ira ceil(tamanioi/BLOCK_SIZE)
	if(cantidad == 0){
		return 1;
	}
	return cantidad;
}

int escribir_datos_bloque(char* nombre_archivo, u_int32_t* puntero_bloque, int bloques_totales, void* contenido){
	int posicion = 0;
	int size = 0;

	//Escribir bloque indice
	for(int i=1;i<bloques_totales;i++){
		memcpy(buffer_bloques + inicio_bloque(puntero_bloque[0]) + posicion, &puntero_bloque[i], sizeof(u_int32_t));
		posicion += sizeof(u_int32_t);
	}

	log_debug(filesystem_log, "## Acceso Bloque - Archivo: <%s> - Tipo Bloque: <INDICE> - Bloque File System <%i>\n", nombre_archivo, puntero_bloque[0]);
	usleep(filesystem_registro.retardo_acceso_bloque * 1000);

	//Escribir bloque de datos
	for(int i=1;i<bloques_totales;i++){
		memcpy(buffer_bloques + inicio_bloque(puntero_bloque[i]), contenido + size, filesystem_registro.block_size);

		log_debug(filesystem_log, "## Acceso Bloque - Archivo: <%s> - Tipo Bloque: <DATOS> - Bloque File System <%i>\n", nombre_archivo, puntero_bloque[i]);
		usleep(filesystem_registro.retardo_acceso_bloque * 1000);
		size += filesystem_registro.block_size;
	}

	msync(buffer_bloques, strlen(buffer_bloques),MS_SYNC);
	free(contenido);
	return EXIT_SUCCESS;
}

//Funciones de bitmap.dat
void crear_bitmap(int file_descriptor){
	int tamanio_archivo = filesystem_registro.block_count;

	ftruncate(file_descriptor, tamanio_archivo);

	struct stat buf;

	fstat(file_descriptor, &buf);
	puntero_bitmap = mmap(NULL, buf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, file_descriptor, 0);
	buffer_bitmap = bitarray_create_with_mode(puntero_bitmap, buf.st_size, LSB_FIRST);
	bloques_libres = 0; //Para limpiar los bloques de bitmap

	for(int i=0;i<buffer_bitmap->size;i++){
		bitarray_clean_bit(buffer_bitmap, i);
		bloques_libres++;
	}
}

void abrir_bitmap(int file_descriptor){
	struct stat buf;

	fstat(file_descriptor, &buf);
	puntero_bitmap = mmap(NULL, buf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, file_descriptor, 0);
	buffer_bitmap = bitarray_create_with_mode(puntero_bitmap, buf.st_size, LSB_FIRST);

	bloques_libres = 0; //Para limpiar los bloques de bitmap
	//Verifica al abrir el archivo los bloques libres
	for(int i=0;i<buffer_bitmap->size;i++){
		if(!bitarray_test_bit(buffer_bitmap, i)){
			bloques_libres++;
		}
	}
}

u_int32_t* buscar_bloques_bitmap(int longitud){
	u_int32_t* puntero_bloques = malloc(longitud*sizeof(u_int32_t)); //Puntero de bloques
	int contador_espacios = 0;
	for(int i=0;i<buffer_bitmap->size;i++){
		if(!bitarray_test_bit(buffer_bitmap, i)){
			puntero_bloques[contador_espacios] = i;
			contador_espacios++;
			if(contador_espacios == longitud){
				return puntero_bloques;
			}
		}
	}
	return NULL;
}
int asignar_bloques_bitmap(u_int32_t* puntero_bloques, int longitud){

	for(int i = 0;i<longitud;i++){
		bitarray_set_bit(buffer_bitmap, puntero_bloques[i]);
		bloques_libres--; //Va descontando los bloques de bitmap
	}

	msync(puntero_bitmap, buffer_bitmap->size, MS_SYNC);

	return EXIT_SUCCESS;
}

//Archivos de metadata

int crear_metadata(char* ruta, int bloque_indexado, int tamanio_archivo){
	int fd;
	char* ruta_archivo = ruta_absoluta("files/");
	ruta_archivo = realloc(ruta_archivo, strlen(ruta_archivo) + strlen(ruta) + 1);
	strcat(ruta_archivo, ruta);

	if((fd = open(ruta_archivo, O_CREAT, S_IRWXU)) == -1){
		perror("No se pudo crear el archivo!!!\n");
		return EXIT_FAILURE;
	}else{
		close(fd);

		char* bloque_string = malloc(50*sizeof(char));
		char* tamanio_string = malloc(50*sizeof(char));

		sprintf(bloque_string, "%i", bloque_indexado);
		sprintf(tamanio_string, "%i", tamanio_archivo);

		modificar_metadata(ruta_archivo, bloque_indexado, tamanio_archivo);

		free(bloque_string);
		free(tamanio_string);
		free(ruta_archivo);
	}

	return EXIT_SUCCESS;
}

int modificar_metadata(char* ruta, int bloque_indexado, int tamanio_archivo){
	t_config* config = config_create(ruta);

	char* bloque_string = malloc(50*sizeof(char));
	char* tamanio_string = malloc(50*sizeof(char));

	sprintf(tamanio_string, "%i", tamanio_archivo);
	sprintf(bloque_string, "%i", bloque_indexado);

	config_set_value(config, "SIZE", tamanio_string);
	config_set_value(config, "INDEX_BLOCK", bloque_string);
	config_save(config);

	free(bloque_string);
	free(tamanio_string);
	config_destroy(config);

	return EXIT_SUCCESS;
}