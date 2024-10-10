#include <kernel.h>

int main(int argc, char* argv[]) {

    //---------------------------- Iniciar archivos ----------------------------

    kernel_log = iniciar_logger("./files/kernel.log", "KERNEL", 1, LOG_LEVEL_DEBUG);

    kernel_config = iniciar_config("./files/kernel.config");

    kernel_registro = levantar_datos(kernel_config);

    // ----------------------- FUNCIONES DE INICIO ------------------------

    inicializar_colas();
    t_list* proceso_creados = list_create();

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
    eliminar_listas();
    //LIBERAR COLAS Y ELEMENTOS QUE CONTIENE
}

// ------------------------- CREAR LISTAS -------------------------


   

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

    t_tcb* create_tcb(){
        t_tcb* tcb = malloc(sizeof(t_tcb));

        tcb->tid;
        tcb->prioridad;
        tcb->archivo_asociado = " ";


        return tcb;
    }

    
    
    // -------------------- FUNCIONES DE PLANIFICACIÓN --------------------

    void inicializar_colas(){
        cola_new = queue_create();
        cola_new_procesos queue_create();
        cola_ready = queue_create();
        cola_ready_procesos = queue_create();
        cola_blocked = queue_create();
        cola_exit = queue_create();
        cola_prioridad_maxima= queue_create();
        cola_prioridad_1 = queue_create();
        cola_prioridad_2 = queue_create();
        cola_prioridad_3 = queue_create();
        cola_prioridad_4 = queue_create();
        cola_prioridad_5 = queue_create();
        cola_prioridad_6 = queue_create();
        cola_prioridad_7 = queue_create();

        
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
            char* planificacion = kernel_registro.algoritmo_planificacion;


        if(planificacion == "FIFO"){

        }else if(planificacion == "PRIORIDADES"){

        }else if(planificacion == "COLAS_MULTINIVEL"){

        }else{
            log_error(kernel_log,"NO SE RECONOCE LA PLANIFICACION");
        }

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

    void poner_en_new(t_hilo_planificacion* hilo_del_proceso){

        queue_push(cola_new,hilo_del_proceso);
    }

    void poner_en_new_procesos(t_pcb* pcb){

        queue_push(cola_new_procesos,(void*)pcb);
    }

    void poner_en_ready_segun_prioridad(){
        int size_cola_new = queue_size(cola_new_procesos);
        t_pcb* pcb;
        t_hilo_planificacion hilo_del_proceso;
        pcb = queue_pop(cola_new_procesos);
        pcb->estado = READY_STATE;

        //MANDAR AL PCB A LA LISTA DE PROCESOS EN READY
        poner_en_ready_procesos(pcb);
        




    }

    void poner_en_ready_procesos(){
        int size_cola_new = queue_size(cola_new);
        t_pcb* pcb;
        pcb = queue_pop(cola_new_procesos);
        pcb->estado = READY_STATE;
        queue_push(cola_ready_procesos, pcb);
    }

    // --------------------- Creacion de procesos ----------------------


    

    

    void* peticion_crear_proceso(){
        uint32_t largo_string = 0;
        t_buffer* buffer;
        buffer = buffer_recieve( fd_conexion_dispatch );
        char* path = buffer_read_string(buffer, &largo_string);

        int tam_proceso = buffer_read_uint32(buffer);

        int prioridad_proceso = buffer_read_uint32(buffer);

        t_pcb* pcb = create_pcb();
        poner_en_new_procesos(pcb);
        log_debug(kernel_log,"PID: %d- Se crea el proceso- Estado: NEW", pcb->pid);

        t_hilo_planificacion* primer_hilo_asociado = {
                pcb->pid,
                {0,prioridad_proceso},
                NEW_STATE
            };

            list_add(pcb->tids,primer_hilo_asociado->tcb_asociado->tid);

            poner_en_new(primer_hilo_asociado);

        peticion_iniciar_proceso(pcb,primer_hilo_asociado);
        
    }


    void* peticion_iniciar_proceso(t_pcb pcb, t_hilo_planificacion* primer_hilo_asociado){
        t_paquete* paquete_proceso = crear_paquete(CREAR_PROCESO);

        agregar_a_paquete(paquete_proceso, &pcb->pid, sizeof(uint32_t));

        //ENVIA EL PEDIDO A MEMORIA PARA INICIALIZAR EL PROCESO
        enviar_paquete(paquete_proceso, conexion_memoria);
        log_debug(kernel_log, "SE ENVIO PAQUETE");

        //RECIBO EL PAQUETE CON LA RESPUESTA DE MEMORIA 
        char* respuesta_memoria = recibir_mensaje(conexion_memoria,kernel_log);

        if(respuesta_memoria == "ESPACIO_ASIGNADO"){
          
            poner_en_ready_procesos(pcb);
            poner_en_ready_segun_prioridad(primer_hilo_asociado);

        }else if(respuesta_memoria == "ESPACIO_NO_ASIGNADO"){
            pcb= queue_pop(cola_new_procesos);
            poner_en_new_procesos(pcb);
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
            //FALTA LIBERAR HILOS DE ESE PCB??
            log_debug(kernel_log,"Find de proceso:“## Finaliza el proceso <PID> %d", pcb->pid);
            inicializar_pcb_en_espera();

        }else if(respuesta_memoria == "FINALIZACION_RECHAZADA"){
            pcb= queue_pop(cola_new);
            poner_en_new(pcb);
        }
        
        
    }


    // --------------------- Creacion de hilo----------------------

    void* peticion_crear_hilo(void){

        uint32_t largo_string = 0;
        t_buffer* buffer;
        buffer = buffer_recieve( fd_conexion_dispatch );
        char* path = buffer_read_string(buffer, &largo_string);

        int prioridad_hilo = buffer_read_uint32(buffer);

        t_paquete* paquete_proceso = crear_paquete(CREAR_HILO);

        /*COMO LE MANDO A MEMORIA EL PATH A EJECUTAR*/
        agregar_a_paquete(paquete_proceso, &pcb->pid, sizeof(uint32_t));
       
        

        //ENVIA EL PEDIDO A MEMORIA PARA INICIALIZAR EL PROCESO
        enviar_paquete(paquete_proceso, conexion_memoria);
        log_debug(kernel_log, "SE ENVIO PAQUETE");
        

        t_tcb tcb= create_tcb();
        //RECIBO EL PAQUETE CON LA RESPUESTA DE MEMORIA 
        char* respuesta_memoria = recibir_mensaje(conexion_memoria,kernel_log);

        if(respuesta_memoria == "ESPACIO_ASIGNADO"){
            /*TODO la funcion aplicar_tid*/
            tcb.tid = aplicar_tid();
            /*PONGO EN READY AL HILO SEGUN SU NIVEL DE PRIORIDAD */
            if(prioridad_hilo == 0){
            
            }else if(prioridad_hilo == 1){
                
            }else if(prioridad_hilo == 2){
            
            }else if(prioridad_hilo == 3){
            
            }else if(prioridad_hilo == 4){
            
            }else if(prioridad_hilo == 5){
            
            }else if(prioridad_hilo == 6){
            
            }else if(prioridad_hilo == 7){
            
            }else{
                log_error(kernel_log,"NO SE RECONOCE LA PLANIFICACION");
            }

        }else if(respuesta_memoria == "ESPACIO_NO_ASIGNADO"){
            pcb= queue_pop(cola_new_procesos);
            poner_en_new_procesos(pcb);
        }
    
    }

    // --------------------- Finalizacion de hilo ----------------------

    // --------------------- Syscalls a atender  ----------------------

  

    void* syscalls_a_atender(char* syscall){

        
        /*LEER LA OP QUE TIENE EL PAQUETE QUE ENVIA CPU (recibir_op)*/
        switch (syscall)
        {
        case PROCESS_CREATE:
            peticion_crear_proceso();
            break;

        case PROCESS_EXIT:
            peticion_finalizar_proceso();
            break;

        case THREAD_CREATE:
            peticion_crear_hilo();
            break;

        case THREAD_JOIN:
            
            break;

        case THREAD_CANCEL:
            
            break;

        case THREAD_EXIT:
            
            break;
        
        case MUTEX_UNLOCK:
            
            break;

        case DUMP_MEMORY:
            
            break;
        
        default:
            break;
        }
    }

    