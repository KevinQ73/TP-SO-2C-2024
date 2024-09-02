#include <utils-filesystem.h>

t_filesystem levantar_datos(t_config* config){

    t_filesystem datos_config;

    datos_config.puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    datos_config.path_mount_dir = config_get_string_value(config, "MOUNT_DIR");
    datos_config.block_size = config_get_int_value(config,"BLOCK_SIZE");
    datos_config.block_count = config_get_int_value(config,"BLOCK_COUNT");
    datos_config.retardo_acceso_bloque = config_get_int_value(config,"RETARDO_ACCESO_BLOQUE");
    datos_config.log_level = config_get_string_value(config, "LOG_LEVEL");

    return datos_config;
}