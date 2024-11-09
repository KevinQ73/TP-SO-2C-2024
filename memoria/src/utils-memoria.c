#include <utils-memoria.h>

t_memoria levantar_datos_memoria(t_config* config){

    t_memoria datos_config;

    datos_config.puerto_escucha = strdup(config_get_string_value(config, "PUERTO_ESCUCHA"));
    datos_config.ip_filesystem = strdup(config_get_string_value(config, "IP_FILESYSTEM"));
    datos_config.puerto_filesystem = strdup(config_get_string_value(config, "PUERTO_FILESYSTEM"));
    datos_config.tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
    datos_config.path_instrucciones = strdup(config_get_string_value(config, "PATH_INSTRUCCIONES"));
    datos_config.retardo_respuesta = config_get_int_value(config, "RETARDO_RESPUESTA");
    datos_config.esquema = strdup(config_get_string_value(config, "ESQUEMA"));
    datos_config.algoritmo_busqueda = strdup(config_get_string_value(config, "ALGORITMO_BUSQUEDA"));
    datos_config.particiones = config_get_array_value(config, "PARTICIONES");
    datos_config.log_level = strdup(config_get_string_value(config, "LOG_LEVEL"));

    return datos_config;
}

int crear_conexion_con_fs(t_log* memoria_log, char* ip, char* puerto){
    int conexion_memoria = crear_conexion(memoria_log, ip, puerto);
    log_debug(memoria_log, "ME CONECTÉ A FS");

    return conexion_memoria;
}