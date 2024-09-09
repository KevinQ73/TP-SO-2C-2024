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
        dictionary_put(registros, array_registros[i], (void*)make_registers);
    }
    return registros;
}

void iniciar_cpu(){
    /* 
        Inicio proceso de ejecución de un proceso y un hilo mandado a la CPU

        1) Recibo PCB con PID y TIDs asociados al proceso que quiere ejecutar
        2) FETCH -> DECODE -> EXECUTE -> CHECK INTERRUPT

    */

    t_pcb* pcb_recibido = recibir_pcb();

    fetch(pcb_recibido);
    
    /*
    decode(); IN CONSTRUCTION
    */
    /*
    execute(); IN CONSTRUCTION
    */
    /*
    check_interrupt(); IN CONSTRUCTION
    */
}

void fetch(t_pcb* pcb_recibido){

}