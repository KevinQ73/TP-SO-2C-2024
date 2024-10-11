#include <kernel.h>

int main(int argc, char* argv[]) {

    //---------------------------- Iniciar archivos ----------------------------

    kernel_log = iniciar_logger("./files/kernel.log", "KERNEL", 1, LOG_LEVEL_DEBUG);

    kernel_config = iniciar_config("./files/kernel.config");

    kernel_registro = levantar_datos(kernel_config);

    // ----------------------- FUNCIONES DE INICIO ------------------------

    inicializar_colas();
    t_list* proceso_creados = list_create();
    t_list* hilos_en_espera = list_create();
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
    //TODO
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
        cola_execute= queue_create();
        cola_exit = queue_create();

        

        
    }

    
    void* planificador_corto_plazo(){
            char* planificacion = kernel_registro.algoritmo_planificacion;


        if(planificacion == "FIFO"){

            cola_ready_fifo= queue_create();

        }else if(planificacion == "PRIORIDADES"){

            cola_prioridad_maxima= queue_create();
            cola_prioridad_1 = queue_create();
            cola_prioridad_2 = queue_create();
            cola_prioridad_3 = queue_create();
            cola_prioridad_4 = queue_create();
            cola_prioridad_5 = queue_create();
            cola_prioridad_6 = queue_create();
            cola_prioridad_7 = queue_create();


        }else if(planificacion == "COLAS_MULTINIVEL"){
            cola_prioridad_maxima= queue_create();
            cola_prioridad_1 = queue_create();
            cola_prioridad_2 = queue_create();
            cola_prioridad_3 = queue_create();
            cola_prioridad_4 = queue_create();
            cola_prioridad_5 = queue_create();
            cola_prioridad_6 = queue_create();
            cola_prioridad_7 = queue_create();

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

/*TODO No se si es con * o sin en t_hilo_planificacion*/
    void* poner_en_ready_segun_prioridad(t_hilo_planificacion* hilo_del_proceso;){
       
        
        /*PONGO EN READY AL HILO SEGUN SU NIVEL DE PRIORIDAD */


        if(planificacion == "FIFO"){
            queue_pop(cola_new);
            queue_push(cola_ready, hilo_del_proceso);

        }else if(planificacion == "PRIORIDADES"){
            if(prioridad_hilo == 0){
                queue_push(cola_prioridad_maxima, hilo_del_proceso);
            }else if(prioridad_hilo == 1){
                queue_push(cola_prioridad_1, hilo_del_proceso);
                
            }else if(prioridad_hilo == 2){
                queue_push(cola_prioridad_2, hilo_del_proceso);
                
            }else if(prioridad_hilo == 3){
                queue_push(cola_prioridad_3, hilo_del_proceso);
                
            }else if(prioridad_hilo == 4){
                queue_push(cola_prioridad_4, hilo_del_proceso);
                
            }else if(prioridad_hilo == 5){
                queue_push(cola_prioridad_5, hilo_del_proceso);
                
            }else if(prioridad_hilo == 6){
                queue_push(cola_prioridad_6,hilo_del_proceso);
                
            }else if(prioridad_hilo == 7){
                queue_push(cola_prioridad_7,hilo_del_proceso);
                
            }
            


        }else if(planificacion == "COLAS_MULTINIVEL"){

            if(prioridad_hilo == 0){
                queue_push(cola_prioridad_maxima, hilo_del_proceso);
            }else if(prioridad_hilo == 1){
                queue_push(cola_prioridad_1, hilo_del_proceso);
                
            }else if(prioridad_hilo == 2){
                queue_push(cola_prioridad_2, hilo_del_proceso);
                
            }else if(prioridad_hilo == 3){
                queue_push(cola_prioridad_3, hilo_del_proceso);
                
            }else if(prioridad_hilo == 4){
                queue_push(cola_prioridad_4, hilo_del_proceso);
                
            }else if(prioridad_hilo == 5){
                queue_push(cola_prioridad_5, hilo_del_proceso);
                
            }else if(prioridad_hilo == 6){
                queue_push(cola_prioridad_6,hilo_del_proceso);
                
            }else if(prioridad_hilo == 7){
                queue_push(cola_prioridad_7,hilo_del_proceso);
                
            }

        }
        
    }

    void poner_en_ready_procesos(){
        int size_cola_new = queue_size(cola_new);
        t_pcb* pcb;
        pcb = queue_pop(cola_new_procesos);
        
        pcb->estado = READY_STATE;

        list_add(proceso_creados,pcb);
        
    }

    // --------------------- Creacion de procesos ----------------------
    t_hilo_planificacion* find_by_pid(t_list* hilos_en_espera, unit_32* pid) {

        bool hilo_pid(void* ptr) {
            t_hilo_planificacion* hilo = (t_hilo_planificacion*) ptr;
            return compare_pid(hilo->pid, pid);
        }
        //TODO por que no encuetra la funcion ?
        return list_find(hilos_en_espera,pid);
    }

    
    void* inicializar_pcb_en_espera(){
        t_pcb* pcb;
        
            
            if(queue_size(cola_new_procesos) != 0){
                pcb= queue_pop(cola_new_procesos);
                
              
                t_paquete* paquete_proceso = crear_paquete(CREAR_PROCESO);

                agregar_a_paquete(paquete_proceso, &pcb->pid, sizeof(uint32_t));

                //ENVIA EL PEDIDO A MEMORIA PARA INICIALIZAR EL PROCESO
                enviar_paquete(paquete_proceso, conexion_memoria);
                log_debug(kernel_log, "SE ENVIO PAQUETE");

                //RECIBO EL PAQUETE CON LA RESPUESTA DE MEMORIA 
                char* respuesta_memoria = recibir_mensaje(conexion_memoria,kernel_log);

                
                if(strcmp(respuesta_memoria, "ESPACIO_ASIGNADO")){

                    
                   t_hilo_planificacion* hilo_encontrado = find_by_pid(hilos_en_espera,pcb->pid);
                    bool removed = list_remove_element(hilos_en_espera, hilo_encontrado);
                    poner_en_ready_procesos(pcb);
                    poner_en_ready_segun_prioridad(hilo_encontrado);
                    
                }else if(strcmp(respuesta_memoria, "ESPACIO_NO_ASIGNADO")){
                    pcb= queue_pop(cola_new);
                    poner_en_new(pcb);
                }
            }
            
    }
    

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

        t_tcb tcb= create_tcb();
        tcb.tid= aplicar_tid();
        tcb.prioridad= prioridad_proceso;
        /*TODO DONDE LO LIBERO EL TCB?*/

        /*TODO esta bien inicializado?*/
        t_hilo_planificacion* primer_hilo_planificacion;
        


        primer_hilo_planificacion->pid = pcb->pid;
        primer_hilo_planificacion->tcb_asociado = tcb;
        primer_hilo_planificacion->estado = NEW_STATE;

    
        list_add(pcb->tids,primer_hilo_asociado->tcb_asociado->tid);

        

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

        

        if(strcmp(respuesta_memoria, "ESPACIO_ASIGNADO")){
          
            poner_en_ready_procesos(pcb);
            poner_en_ready_segun_prioridad(primer_hilo_asociado);

        }else if(strcmp(respuesta_memoria, "ESPACIO_NO_ASIGNADO")){
            pcb= queue_pop(cola_new_procesos);
            poner_en_new_procesos(pcb);
            list_add(hilos_en_espera,primer_hilo_asociado);
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
            //TODO FALTA LIBERAR HILOS DE ESE PCB??
            log_debug(kernel_log,"Find de proceso:“## Finaliza el proceso <PID> %d", pcb->pid);
            inicializar_pcb_en_espera();

        }else if(respuesta_memoria == "FINALIZACION_RECHAZADA"){
            pcb= queue_pop(cola_new);
            poner_en_new(pcb);
        }
        
        
    }


    // --------------------- Creacion de hilo----------------------
    aplicar_tid(){
        //TODO
    }

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

        
        /*TODO la funcion aplicar_tid*/
        tcb.tid  = aplicar_tid();
        poner_en_ready_segun_prioridad(tcb);
        
    
    }

    // --------------------- Finalizacion de hilo ----------------------

    void* finalizar_hilo(int tid){
        t_paquete* paquete_fin_hilo = crear_paquete(FINALIZAR_HILO);
        agregar_a_paquete(paquete_fin_hilo, &tid sizeof(uint32_t));

        //ENVIA EL PEDIDO A MEMORIA PARA FINALIZAR EL PROCESO
        enviar_paquete(paquete_fin_proceso, conexion_memoria);
        log_debug(kernel_log, "SE ENVIO AVISO DE FINALIZACION DE PROCESO");

        //RECIBO EL PAQUETE CON LA RESPUESTA DE MEMORIA 
        char* respuesta_memoria = recibir_mensaje(conexion_memoria,kernel_log);

        if(respuesta_memoria == "FINALIZACION_ACEPTADA"){
            //TODO ver como buscar segun que planificacion sea
            if(planificacion == "FIFO"){

            }else if(planificacion == "PRIORIDADES"){

            }else if(planificacion == "COLAS_MULTINIVEL"){

            }
            

        }else if(respuesta_memoria == "FINALIZACION_RECHAZADA"){
           
        }
    }

    // --------------------- Syscalls a atender  ----------------------

  

    void* syscalls_a_atender(){

/*LEER LA OP QUE TIENE EL PAQUETE QUE ENVIA CPU (recibir_op)*/

        int syscall = recibir_operacion(fd_conexion_interrupt);
        
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

    