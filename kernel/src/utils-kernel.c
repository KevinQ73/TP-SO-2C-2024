#include <utils-kernel.h>

t_kernel levantar_datos(t_config* config){

    t_kernel datos_config;

    datos_config.ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    datos_config.puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    datos_config.ip_cpu = config_get_string_value(config, "IP_CPU");
    datos_config.puerto_cpu_dispatch = config_get_string_value(config,"PUERTO_CPU_DISPATCH");
    datos_config.puerto_cpu_interrupt = config_get_string_value(config,"PUERTO_CPU_INTERRUPT");
    datos_config.algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    datos_config.quantum = config_get_int_value(config, "QUANTUM");
    datos_config.log_level = config_get_string_value(config, "LOG_LEVEL");
    
    return datos_config;
}