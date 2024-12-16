#include <filesystem.h>

int main(int argc, char* argv[]) {

    //filesystem_log = iniciar_logger("./files/filesystem.log", "FILESYSTEM", 1, LOG_LEVEL_DEBUG);
	
	filesystem_log = iniciar_logger("./files/filesystem_obligatorio.log", "FILESYSTEM", 1, LOG_LEVEL_INFO);
    
	filesystem_config = iniciar_config(argv[1]);

    filesystem_registro = levantar_datos(filesystem_config);

	inicializar_fs();

    // --------------------- Conexión como servidor de MEMORIA ----------------------

    fd_escucha_memoria = iniciar_servidor(filesystem_log, "FILESYSTEM", filesystem_registro.puerto_escucha);
    log_debug(filesystem_log,"SOCKET LISTEN LISTO PARA RECIBIR CLIENTES");

    atender_memoria();

	//Finalizar todo
	close(fd_escucha_memoria);
	munmap(puntero_bitmap, strlen(puntero_bitmap));
	munmap(buffer_bloques, strlen(buffer_bloques));
	log_destroy(filesystem_log);
	config_destroy(filesystem_config);
	free(filesystem_registro.path_mount_dir);
	free(filesystem_registro.log_level);
	free(filesystem_registro.puerto_escucha);

    log_debug(filesystem_log, "TERMINANDO MEMORIA");
}

void inicializar_fs(){
	char* name_bitmap = string_duplicate(filesystem_registro.path_mount_dir);
	char* name_bloques = string_duplicate(filesystem_registro.path_mount_dir);
	string_append(&name_bitmap, "/bitmap.dat");
	string_append(&name_bloques, "/bloques.dat");

	int size_bitmap = ceil((double)filesystem_registro.block_count/(double)8);
	int bitmap_fd;
	if(access(name_bitmap, F_OK) == -1){
		bitmap_fd = open(name_bitmap, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
		crear_bitmap(bitmap_fd, size_bitmap);
		log_info(filesystem_log, "## [FILESYSTEM] Archivo Creado: <BITMAP.DAT> - Tamaño: <%d> PATH: %s", size_bitmap, name_bitmap);
	}else{
		bitmap_fd = open(name_bitmap, O_RDWR);
		abrir_bitmap(bitmap_fd);
		log_info(filesystem_log, "## [FILESYSTEM] Archivo Abierto: <BITMAP.DAT> - Tamaño: <%d> PATH: %s", size_bitmap, name_bitmap);
	}

	int size_bloques = filesystem_registro.block_count * filesystem_registro.block_size;
	int bloques_fd;
	if(access(name_bloques, F_OK) == -1){
		bloques_fd = open(name_bloques, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
		crear_bloques(bloques_fd, size_bloques);
		log_info(filesystem_log, "## [FILESYSTEM] Archivo Creado: <BLOQUES.DAT> - Tamaño: <%d> PATH: %s", size_bloques, name_bloques);
	}else{
		bloques_fd = open(name_bloques, O_RDWR);
		abrir_bloques(bloques_fd);
		log_info(filesystem_log, "## [FILESYSTEM] Archivo Abierto: <BLOQUES.DAT> - Tamaño: <%d> PATH: %s", size_bloques, name_bloques);
	}

	free(name_bitmap);
	free(name_bloques);
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

    int operacion_memoria = recibir_operacion(fd_memoria);

	buffer = buffer_recieve(fd_memoria);
	switch(operacion_memoria){
		case DUMP_MEMORY:
			log_info(filesystem_log, "MEMORY DUMP LEIDO");

			uint32_t tamanio_buffer;
			uint32_t tamanio;

			char* nombre = buffer_read_string(buffer, &tamanio_buffer);
			void* contenido = buffer_read_string(buffer,&tamanio);

			if( dump_memory(nombre, tamanio, contenido) == EXIT_SUCCESS){
				enviar_mensaje("OK_FS", fd_memoria, filesystem_log);
			}else {
				enviar_mensaje("ERROR", fd_memoria, filesystem_log);
			}
		break;
		default:
			log_debug(filesystem_log, "## [FILESYSTEM:MEMORIA] OPERACIÓN DE MEMORIA ERRONEA");
        break;
	}
	close(fd_memoria);
	close(fd_conexion);
}

//Funciones del dump_memory
int dump_memory(char* nombre_archivo, int tamanio, void* contenido){
	int bloques_totales = cantidad_bloques(tamanio) + 1;
	u_int32_t* puntero_bloques = buscar_bloques_bitmap(bloques_totales);

	if(puntero_bloques == NULL || ((filesystem_registro.block_size/sizeof(u_int32_t)) < cantidad_bloques(tamanio))){
		log_debug(filesystem_log, "No hay espacio suficiente para almacenar los bloques de datos\n");
		return EXIT_FAILURE;
	}else{
		asignar_bloques_bitmap(puntero_bloques, bloques_totales);
		log_info(filesystem_log, "## Bloque asignado: <%i> - Archivo: <%s> - Bloques Libres: <%i>\n", puntero_bloques[0], nombre_archivo, bloques_libres);
		escribir_datos_bloque(nombre_archivo, puntero_bloques, bloques_totales, contenido);
		crear_metadata(nombre_archivo, puntero_bloques[0], tamanio);
		log_info(filesystem_log, "## Archivo Creado: %s - Tamaño: <%i>\n", nombre_archivo, tamanio);
		log_info(filesystem_log, "## Fin de solicitud - Archivo: %s\n", nombre_archivo);
		free(puntero_bloques);
		return EXIT_SUCCESS;
	}
}

//Funciones de bloques.dat
void crear_bloques(int file_descriptor, int size){
	ftruncate(file_descriptor, size);

	buffer_bloques = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, file_descriptor, 0);

	int posicion = 0;
	for(int i=0;i<size;i++){
		memset(buffer_bloques + posicion, 0, sizeof(char));
		posicion++;
	}
	msync(buffer_bloques, size, MS_SYNC);
}

void abrir_bloques(int file_descriptor){
	struct stat buf;

	fstat(file_descriptor, &buf);
	buffer_bloques = mmap(NULL, buf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, file_descriptor, 0);

	close(file_descriptor);
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

	log_info(filesystem_log, "## Acceso Bloque - Archivo: %s - Tipo Bloque: <INDICE> - Bloque File System <%i>\n", nombre_archivo, puntero_bloque[0]);
	usleep(filesystem_registro.retardo_acceso_bloque * 1000);

	//Escribir bloque de datos
	for(int i=1;i<bloques_totales;i++){
		memcpy(buffer_bloques + inicio_bloque(puntero_bloque[i]), contenido + size, filesystem_registro.block_size);

		log_info(filesystem_log, "## Acceso Bloque - Archivo: %s - Tipo Bloque: <DATOS> - Bloque File System <%i>\n", nombre_archivo, puntero_bloque[i]);
		usleep(filesystem_registro.retardo_acceso_bloque * 1000);
		size += filesystem_registro.block_size;
	}

	msync(buffer_bloques, strlen(buffer_bloques),MS_SYNC);
	free(contenido);
}

//Funciones de bitmap.dat
void crear_bitmap(int file_descriptor, int size){
    if (ftruncate(file_descriptor, size) == -1) {
        log_error(filesystem_log, "Error al establecer el tamaño del archivo bitmap.dat\n");
        close(file_descriptor);
        abort();
    }

    puntero_bitmap = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor, 0);
    if (puntero_bitmap == MAP_FAILED) {
        log_error(filesystem_log, "Error al mapear el archivo bitmap.dat en memoria\n");
        close(file_descriptor);
        abort();
    }
	bloques_libres = 0;
    memset(puntero_bitmap, 0, size);

    buffer_bitmap = bitarray_create_with_mode(puntero_bitmap, size, LSB_FIRST);
    if (!buffer_bitmap) {
        log_error(filesystem_log, "Error al asignar memoria para el objeto bitarray\n");
        munmap(puntero_bitmap, size);
        close(file_descriptor);
        abort();
    }

	for(int i=0;i<buffer_bitmap->size;i++){
		bitarray_clean_bit(buffer_bitmap, i);
		bloques_libres++;
	}

    if (msync(puntero_bitmap, buffer_bitmap->size, MS_SYNC) == -1) {
        log_error(filesystem_log, "Error en la sincronización con msync()\n");
    }

    close(file_descriptor);

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
	close(file_descriptor);
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
}
int asignar_bloques_bitmap(u_int32_t* puntero_bloques, int longitud){

	for(int i = 0;i<longitud;i++){
		bitarray_set_bit(buffer_bitmap, puntero_bloques[i]);
		bloques_libres--; //Va descontando los bloques de bitmap
	}

	msync(puntero_bitmap, buffer_bitmap->size, MS_SYNC);
}

//Archivos de metadata

int crear_metadata(char* ruta, int bloque_indexado, int tamanio_archivo){
	printf("RUTA(%s)\n", ruta);

	char* ruta_archivo = string_duplicate(filesystem_registro.path_mount_dir);
	string_append(&ruta_archivo, "/files/");
	string_append(&ruta_archivo, ruta);

	int fd_metadata = open(ruta_archivo, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

	log_info(filesystem_log, "## [FILESYSTEM] Archivo Creado: <BLOQUES.DAT> - Tamaño: <%d>", tamanio_archivo);

	char* bloque_string = malloc(50*sizeof(char));
	char* tamanio_string = malloc(50*sizeof(char));

	sprintf("%i", bloque_indexado);
	sprintf(tamanio_string, "%i", tamanio_archivo);

	modificar_metadata(ruta_archivo, bloque_indexado, tamanio_archivo);

	free(bloque_string);
	free(tamanio_string);
	free(ruta_archivo);
}

void modificar_metadata(char* ruta, int bloque_indexado, int tamanio_archivo){
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
}