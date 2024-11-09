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

int mmu(uint32_t base, uint32_t limite, int registros_guardados){

    
}

t_pid_tid recibir_paquete_kernel(int socket_kernel, t_log* cpu_log){
    t_buffer* buffer = buffer_recieve(socket_kernel);
    t_pid_tid pid_tid = recibir_pid_tid(buffer, cpu_log);
    
    return pid_tid;
}

void enviar_paquete_kernel(t_buffer* buffer, int socket_memoria, cod_inst codigo_instruccion){
    t_paquete* paquete = crear_paquete(codigo_instruccion);
    paquete->buffer = buffer;

    enviar_paquete(paquete, socket_memoria);
}

void recibir_contexto_memoria(t_contexto* registros_cpu, int fd_memoria, t_log* cpu_log){
    t_buffer* buffer;
    uint32_t valor = 0;
    int i = 0;

    int op = recibir_operacion(fd_memoria);

    if (op == ENVIAR_CONTEXTO_EJECUCION)
    {
        log_debug(cpu_log, "SE RECIBIÓ EL CONTEXTO DE EJECUCIÓN DE MEMORIA");
    } else {
        log_warning(cpu_log, "ERROR EN EL ENVÍO DEL CONTEXTO DE EJECUCIÓN");
        abort();
    }

    buffer = buffer_recieve(fd_memoria);
    actualizar_registros_cpu(registros_cpu, buffer, cpu_log);
    buffer_destroy(buffer);
}

void solicitar_contexto_ejecucion(t_contexto* registros_cpu, t_pid_tid pid_tid, int fd_memoria, t_log* log_cpu){    
    enviar_pid_tid(&(pid_tid.pid), &(pid_tid.tid), fd_memoria, log_cpu);

    recibir_contexto_memoria(registros_cpu, fd_memoria, log_cpu);

    log_info(log_cpu, "## TID: <%d> - Solicito Contexto Ejecución", pid_tid.tid);
}

void actualizar_registros_cpu(t_contexto* registros_cpu, t_buffer* buffer, t_log* log){
    registros_cpu->base = buffer_read_uint32(buffer);
    log_debug(log, "BASE RECIBIDO: %d", registros_cpu->base);
    registros_cpu->limite = buffer_read_uint32(buffer);
    log_debug(log, "LIMITE RECIBIDO: %d", registros_cpu->limite);
    registros_cpu->pc = buffer_read_uint32(buffer);
    log_debug(log, "PC RECIBIDO: %d", registros_cpu->pc);
    registros_cpu->ax = buffer_read_uint32(buffer);
    log_debug(log, "AX RECIBIDO: %d", registros_cpu->ax);
    registros_cpu->bx = buffer_read_uint32(buffer);
    log_debug(log, "BX RECIBIDO: %d", registros_cpu->bx);
    registros_cpu->cx = buffer_read_uint32(buffer);
    log_debug(log, "CX RECIBIDO: %d", registros_cpu->cx);
    registros_cpu->dx = buffer_read_uint32(buffer);
    log_debug(log, "DX RECIBIDO: %d", registros_cpu->dx);
    registros_cpu->ex = buffer_read_uint32(buffer);
    log_debug(log, "EX RECIBIDO: %d", registros_cpu->ex);
    registros_cpu->fx = buffer_read_uint32(buffer);
    log_debug(log, "FX RECIBIDO: %d", registros_cpu->fx);
    registros_cpu->gx = buffer_read_uint32(buffer);
    log_debug(log, "GX RECIBIDO: %d", registros_cpu->gx);
    registros_cpu->hx = buffer_read_uint32(buffer);
    log_debug(log, "HX RECIBIDO: %d", registros_cpu->hx);
}

uint32_t get_register(t_contexto* registro_cpu, char* registro){
    if (strcmp(registro, "BASE") == 0){
        return registro_cpu->base;
    }

    if (strcmp(registro, "LIMITE") == 0){
        return registro_cpu->limite;
    }

    if (strcmp(registro, "PC") == 0){
        return registro_cpu->base;
    }

    if (strcmp(registro, "AX") == 0){
        return registro_cpu->ax;
    }

    if (strcmp(registro, "BX") == 0){
        return registro_cpu->bx;
    }

    if (strcmp(registro, "CX") == 0){
        return registro_cpu->cx;
    }

    if (strcmp(registro, "DX") == 0){
        return registro_cpu->dx;
    }

    if (strcmp(registro, "EX") == 0){
        return registro_cpu->ex;
    }

    if (strcmp(registro, "FX") == 0){
        return registro_cpu->fx;
    }

    if (strcmp(registro, "GX") == 0){
        return registro_cpu->gx;
    }

    if (strcmp(registro, "HX") == 0){
        return registro_cpu->hx;
    }
}

t_registro_handler get_register_id(char* registro){
    if (strcmp(registro, "BASE") == 0){
        return BASE;
    }

    if (strcmp(registro, "LIMITE") == 0){
        return LIMITE;
    }

    if (strcmp(registro, "PC") == 0){
        return PC;
    }

    if (strcmp(registro, "AX") == 0){
        return AX;
    }

    if (strcmp(registro, "BX") == 0){
        return BX;
    }

    if (strcmp(registro, "CX") == 0){
        return CX;
    }

    if (strcmp(registro, "DX") == 0){
        return DX;
    }

    if (strcmp(registro, "EX") == 0){
        return EX;
    }

    if (strcmp(registro, "FX") == 0){
        return FX;
    }

    if (strcmp(registro, "GX") == 0){
        return GX;
    }

    if (strcmp(registro, "HX") == 0){
        return HX;
    }
}

void modificar_registro(t_contexto* registro_destino, char* registro, int nuevo_valor, t_log* log){
    t_registro_handler reg = get_register_id(registro);
    
    switch (reg)
    {
    case BASE:
        log_info(log, "## Registro %s cambia valor %d a %d", registro, registro_destino->base, nuevo_valor);
        registro_destino->base = nuevo_valor;
        break;
    
    case LIMITE:
        log_info(log, "## Registro %s cambia valor %d a %d", registro, registro_destino->limite, nuevo_valor);
        registro_destino->limite = nuevo_valor;
        break;

    case PC:
        log_info(log, "## Registro %s cambia valor %d a %d", registro, registro_destino->pc, nuevo_valor);
        registro_destino->pc = nuevo_valor;
        break;
    
    case AX:
        log_info(log, "## Registro %s cambia valor %d a %d", registro, registro_destino->ax, nuevo_valor);
        registro_destino->ax = nuevo_valor;
        break;

    case BX:
        log_info(log, "## Registro %s cambia valor %d a %d", registro, registro_destino->bx, nuevo_valor);
        registro_destino->bx = nuevo_valor;
        break;

    case CX:
        log_info(log, "## Registro %s cambia valor %d a %d", registro, registro_destino->cx, nuevo_valor);
        registro_destino->cx = nuevo_valor;
        break;

    case DX:
        log_info(log, "## Registro %s cambia valor %d a %d", registro, registro_destino->dx, nuevo_valor);
        registro_destino->dx = nuevo_valor;
        break;

    case EX:
        log_info(log, "## Registro %s cambia valor %d a %d", registro, registro_destino->ex, nuevo_valor);
        registro_destino->ex = nuevo_valor;
        break;

    case FX:
        log_info(log, "## Registro %s cambia valor %d a %d", registro, registro_destino->fx, nuevo_valor);
        registro_destino->fx = nuevo_valor;
        break;

    case GX:
        log_info(log, "## Registro %s cambia valor %d a %d", registro, registro_destino->gx, nuevo_valor);
        registro_destino->gx = nuevo_valor;
        break;

    case HX:
        log_info(log, "## Registro %s cambia valor %d a %d", registro, registro_destino->hx, nuevo_valor);
        registro_destino->hx = nuevo_valor;
        break;

    default:
        abort();
        break;
    }
}

void program_counter_update(t_contexto* registro_cpu, t_log* log){
    uint32_t valor_inicial = registro_cpu->pc;
    uint32_t valor_final = valor_inicial + 1;
    modificar_registro(registro_cpu, "PC", valor_final, log);
}

void program_counter_jump(t_contexto* registro_cpu, uint32_t salto, t_log* log){
    uint32_t valor_final = salto;
    modificar_registro(registro_cpu, "PC", valor_final, log);
}

char* fetch(t_pid_tid pid_tid_recibido, uint32_t* program_counter, int fd_memoria, t_log* log){
    t_buffer* buffer;

    t_paquete* paquete_memoria = crear_paquete(PETICION_INSTRUCCION);
    buffer = buffer_create(
        (sizeof(uint32_t) * 3)
    );

    buffer_add_uint32(buffer, &(pid_tid_recibido.pid), log);
    log_debug(log, "PID A ENVIAR: %d", pid_tid_recibido.pid);
    buffer_add_uint32(buffer, &(pid_tid_recibido.tid), log);
    log_debug(log, "TID A ENVIAR: %d", pid_tid_recibido.tid);
    buffer_add_uint32(buffer, program_counter, log);
    log_debug(log, "PROGRAM COUNTER A ENVIAR: %d", *program_counter);

    paquete_memoria->buffer = buffer;
    enviar_paquete(paquete_memoria, fd_memoria);
    eliminar_paquete(paquete_memoria);

    char* instruccion = recibir_mensaje(fd_memoria, log);
    return instruccion;
}

char** decode(char* instruccion){
    char** instruccion_parseada = parsear_instruccion(instruccion);
    return instruccion_parseada;
}

void enviar_registros_memoria(t_contexto* registro_cpu, t_pid_tid pid_tid_recibido, int conexion_memoria, t_log* log){
    t_paquete* paquete = crear_paquete(ACTUALIZAR_CONTEXTO_EJECUCION);
    
    t_buffer* buffer = buffer_create(
        sizeof(uint32_t)*13
    );

    buffer_add_uint32(buffer, &pid_tid_recibido.pid, log);
    buffer_add_uint32(buffer, &pid_tid_recibido.tid, log);
    buffer_add_uint32(buffer, &registro_cpu->base, log);
    buffer_add_uint32(buffer, &registro_cpu->limite, log);
    buffer_add_uint32(buffer, &registro_cpu->pc, log);
    buffer_add_uint32(buffer, &registro_cpu->ax, log);
    buffer_add_uint32(buffer, &registro_cpu->bx, log);
    buffer_add_uint32(buffer, &registro_cpu->cx, log);
    buffer_add_uint32(buffer, &registro_cpu->dx, log);
    buffer_add_uint32(buffer, &registro_cpu->ex, log);
    buffer_add_uint32(buffer, &registro_cpu->fx, log);
    buffer_add_uint32(buffer, &registro_cpu->gx, log);
    buffer_add_uint32(buffer, &registro_cpu->hx, log);

    paquete->buffer = buffer;

    enviar_paquete(paquete, conexion_memoria);
}