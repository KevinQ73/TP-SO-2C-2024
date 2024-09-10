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

void recibir_paquete_kernel(int socket_kernel, t_log* cpu_log){
    uint32_t pid = 0;
    uint32_t tid = 0;

    int op = recibir_operacion(socket_kernel);

    if (op == PID_TID)
    {
        log_debug(cpu_log, "SE RECIBIÓ UNA PETICIÓN DE KERNEL POR DISPATCH");
    } else {
        log_warning(cpu_log, "ERROR EN EL PAQUETE ENVIADO POR KERNEL");
        abort();
    }

    t_buffer* buffer;
    buffer = recibir_buffer(socket_kernel);
    
    pid = buffer_read_uint32(buffer);
    tid = buffer_read_uint32(buffer);

    log_debug(cpu_log, "EL PID ES: %d Y EL TID ES: %d", pid, tid);
}

void fetch(t_pcb* pcb_recibido){
    
}