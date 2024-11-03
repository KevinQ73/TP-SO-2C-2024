#include <utils/files.h>

t_log* iniciar_logger(char* ruta_log, char* nombre_logger, int flag_consola, t_log_level log_level) {

	t_log* logger = log_create(ruta_log, nombre_logger, flag_consola, log_level);

	if (logger == NULL) {
		error_show("ERROR AL CREAR EL ARCHIVO .LOG");
		exit(EXIT_FAILURE);
	}

	return logger;
}

void eliminar_logger(t_log* logger){
    if (logger != NULL){
        log_destroy(logger);
    }
}

t_config* iniciar_config(char *path_config){
    
    t_config* config = config_create(path_config);

	if (config == NULL) {
		error_show("ERROR AL CREAR EL ARCHIVO .CONFIG");
		exit(EXIT_FAILURE);
	}

	return config;
}

void eliminar_config(t_config* config){
    if(config != NULL){
        config_destroy(config);
    }
}

t_list* leer_instrucciones(char* path, t_log* log_modulo){
	t_list* lista_instrucciones_a_devolver = list_create();
    FILE* archivo_instrucciones = fopen(path, "rb");

    if (archivo_instrucciones == NULL) {
        log_error(log_modulo,"ARCHIVO DE INSTRUCCIONES INEXISTENTE");
        fclose(archivo_instrucciones);
        abort();
    }

	char* instruccion = NULL;
	size_t len = 0;

    while (getline(&instruccion, &len, archivo_instrucciones) != -1) {
		int largo = strlen(instruccion);
		if(instruccion[largo - 1] == '\n') 
		{
			instruccion[largo - 1] = '\0';
		}
		log_debug(log_modulo, "Se parseó la siguiente instrucción: %s", instruccion);
        list_add(lista_instrucciones_a_devolver, instruccion);
	}
    free(instruccion);
    fclose(archivo_instrucciones);
    log_debug(log_modulo, "SE LEYERON %d INSTRUCCIONES", list_size(lista_instrucciones_a_devolver));
    return lista_instrucciones_a_devolver;
}

char** parsear_instruccion(char* instruccion){
	char** instruccion_parseada = string_split(instruccion, " ");

	return instruccion_parseada;
}