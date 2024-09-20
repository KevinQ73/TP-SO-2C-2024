#include <utils-cpu.h>

t_cpu levantar_datos(t_config* config){

    t_cpu datos_config;

    datos_config.ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    datos_config.puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    datos_config.puerto_cpu_dispatch = config_get_string_value(config,"PUERTO_ESCUCHA_DISPATCH");
    datos_config.puerto_cpu_interrupt = config_get_string_value(config,"PUERTO_ESCUCHA_INTERRUPT");
    datos_config.log_level = config_get_string_value(config, "LOG_LEVEL");

    return datos_config;
}

t_registro* make_registers(){
    t_registro* registro = malloc(sizeof(t_registro));
    registro->valor = 0;
    registro->tamanio = 4;
    return registro;
}

t_dictionary* inicializar_registros(){
    t_dictionary* registros = dictionary_create();
    for (int i = 0; i < 9; i++) {
        dictionary_put(registros, nombres_registros[i], (void*)make_registers);
    }
    return registros;
}

t_pid_tid recibir_paquete_kernel(int socket_kernel, t_log* cpu_log){

    t_pid_tid pid_tid = recibir_pid_tid(socket_kernel, cpu_log);
    
    return pid_tid;
}

void fetch(t_pcb* pcb_recibido){
    
}