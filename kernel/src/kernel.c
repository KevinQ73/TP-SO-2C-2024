#include <kernel.h>

int main(int argc, char* argv[]) {

    //---------------------------- Iniciar archivos ----------------------------

    kernel_log = iniciar_logger("./files/kernel.log", "KERNEL", 1, LOG_LEVEL_DEBUG);

    kernel_config = iniciar_config("./files/kernel.config");

    kernel_registro = levantar_datos(kernel_config);

    // ----------------------- FUNCIONES DE INICIO ------------------------

    inicializar_colas();

    // --------------------- Conexión como cliente de CPU ----------------------

    fd_conexion_dispatch = crear_conexion(kernel_log, kernel_registro.ip_cpu, kernel_registro.puerto_cpu_dispatch);
    log_debug(kernel_log, "ME CONECTÉ A CPU DISPATCH");

    fd_conexion_interrupt = crear_conexion(kernel_log, kernel_registro.ip_cpu, kernel_registro.puerto_cpu_interrupt);
    log_debug(kernel_log, "ME CONECTÉ A CPU INTERRUPT");

    // --------------------- Conexión como cliente de MEMORIA ----------------------

    conexion_memoria = crear_conexion(kernel_log, kernel_registro.ip_memoria, kernel_registro.puerto_memoria);
    log_debug(kernel_log, "ME CONECTÉ A MEMORIA");

    // --------------------- Finalizacion del modulo----------------------
    
    //eliminar_paquete(paquete_proceso);
    //eliminar_paquete(paquete_fin_proceso);

    log_debug(kernel_log, "TERMINANDO KERNEL");
    eliminar_logger(kernel_log);

    //LIBERAR COLAS Y ELEMENTOS QUE CONTIENE
}

    // ------------------------- FUNCIONES DE PCB -------------------------

    t_pcb* create_pcb(){
        t_pcb* pcb = malloc(sizeof(t_pcb));
        
        pcb->pid = 0;
        pcb->tids = list_create();
        pcb->mutex_asociados = list_create();
        pcb->program_counter = 0;
        pcb->estado = NEW_STATE;
    
        return pcb;
    }
    
    // -------------------- FUNCIONES DE PLANIFICACIÓN --------------------

    void inicializar_colas(){
        cola_new = queue_create();
        cola_ready = queue_create();
        cola_blocked = queue_create();
        cola_exit = queue_create();
    }

    void* inicializar_pcb_en_espera(){
        t_pcb* pcb;
        
        if(queue_size(cola_new) != 0){
            pcb= queue_pop(cola_new);
            t_paquete* paquete_proceso = crear_paquete(CREAR_PROCESO);

            agregar_a_paquete(paquete_proceso, &pcb->pid, sizeof(uint32_t));

            //ENVIA EL PEDIDO A MEMORIA PARA INICIALIZAR EL PROCESO
            enviar_paquete(paquete_proceso, conexion_memoria);
            log_debug(kernel_log, "SE ENVIO PAQUETE");

            //RECIBO EL PAQUETE CON LA RESPUESTA DE MEMORIA 
            char* respuesta_memoria = recibir_mensaje(conexion_memoria,kernel_log);

            
            if(strcmp(respuesta_memoria, "ESPACIO_ASIGNADO")){

                //list_add(pcb->pid,); FALTA TID DEL HILO ASOCIADO AL PROCESO
                poner_en_ready(pcb);
                
            }else if(strcmp(respuesta_memoria, "ESPACIO_NO_ASIGNADO")){
                pcb= queue_pop(cola_new);
                poner_en_new(pcb);
            }
        }
        
    }
    void* planificador_corto_plazo(){
    
    }

    void* planificador_largo_plazo(){

    }

    void iniciar_planificacion(){
        proceso_ejecutando = 0;

        pthread_create(&hiloNew, NULL, planificador_largo_plazo, NULL);
        pthread_detach(hiloNew);

        pthread_create(&hiloPlanifCortoPlazo, NULL, planificador_corto_plazo, NULL);
        pthread_detach(hiloPlanifCortoPlazo);
    }

    void poner_en_new(t_pcb* pcb){

        queue_push(cola_new,(void*)pcb);
    }

    void poner_en_ready(){
        int size_cola_new = queue_size(cola_new);
        t_pcb* pcb;
        pcb = queue_pop(cola_new);
        pcb->estado = READY_STATE;
        queue_push(cola_ready, pcb);
    }

    // --------------------- Creacion de procesos ----------------------
    void* peticion_crear_proceso(char* path){
        t_pcb* pcb = create_pcb();
        poner_en_new(pcb);
        log_debug(kernel_log,"PID: %d- Se crea el proceso- Estado: NEW", pcb->pid);
        t_paquete* paquete_proceso = crear_paquete(CREAR_PROCESO);

        agregar_a_paquete(paquete_proceso, &pcb->pid, sizeof(uint32_t));

        //ENVIA EL PEDIDO A MEMORIA PARA INICIALIZAR EL PROCESO

        enviar_paquete(paquete_proceso, conexion_memoria);
        log_debug(kernel_log, "SE ENVIO PAQUETE");

        //RECIBO EL PAQUETE CON LA RESPUESTA DE MEMORIA 
        char* respuesta_memoria = recibir_mensaje(conexion_memoria,kernel_log);

        if(respuesta_memoria == "ESPACIO_ASIGNADO"){

            //list_add(pcb->pid,); FALTA TID DEL HILO ASOCIADO AL PROCESO
            poner_en_ready(pcb);

        }else if(respuesta_memoria == "ESPACIO_NO_ASIGNADO"){
            pcb= queue_pop(cola_new);
            poner_en_new(pcb);
        }
    }


    // --------------------- Finalizacion de procesos ----------------------

    void* peticion_finalizar_proceso( t_pcb* pcb){

        t_paquete* paquete_fin_proceso = crear_paquete(FINALIZAR_PROCESO);
        agregar_a_paquete(paquete_fin_proceso, &pcb->pid, sizeof(uint32_t));

        //ENVIA EL PEDIDO A MEMORIA PARA FINALIZAR EL PROCESO
        enviar_paquete(paquete_fin_proceso, conexion_memoria);
        log_debug(kernel_log, "SE ENVIO AVISO DE FINALIZACION DE PROCESO");

        //RECIBO EL PAQUETE CON LA RESPUESTA DE MEMORIA 
        char* respuesta_memoria = recibir_mensaje(conexion_memoria,kernel_log);

        if(respuesta_memoria == "FINALIZACION_ACEPTADA"){

            free(pcb);
            
            inicializar_pcb_en_espera();

        }else if(respuesta_memoria == "FINALIZACION_RECHAZADA"){
            pcb= queue_pop(cola_new);
            poner_en_new(pcb);
        }
        
        log_debug(kernel_log,"Find de proceso:“## Finaliza el proceso <PID> %d", pcb->pid);
    }


    // --------------------- Creacion de hilo----------------------

    void* peticion_crear_hilo(void){
        
    }

    // --------------------- Finalizacion de hilo ----------------------