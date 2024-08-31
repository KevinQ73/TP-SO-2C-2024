//#include <utils/hello.h>
#include <utils/cliente.h>
#include <utils/files.h>





int main(int argc, char* argv[]) {
    int conexion_cpu;
    int conexion_memoria;
    int conexion_cpu_interrupt;

	char* ip_cpu;
	char* ip_memoria;

    char* puerto_memoria;
	char* puerto_cpu_dispatch;
    char* puerto_cpu_interrupt;

    t_config* config;

    config= iniciar_config("/home/utnso/Desktop/tp-2024-2c-The-Ultimate-Kernel/kernel/files/kernel.config");

    //CONEXIO CON CPU DISPATCH
    ip_cpu= config_get_string_value(config,"IP_CPU");
    puerto_cpu_dispatch= config_get_string_value(config,"PUERTO_CPU_DISPATCH");


    conexion_cpu= crear_conexion(ip_cpu,puerto_cpu_dispatch);

    //CONEXION CON CPU INTERRUPT
    puerto_cpu_interrupt= config_get_string_value(config,"PUERTO_CPU_INTERRUPT");


    conexion_cpu_interrupt= crear_conexion(ip_cpu,puerto_cpu_interrupt);

    //CONEXION CON MEMORIA  
    ip_memoria= config_get_string_value(config,"IP_MEMORIA");
    puerto_memoria= config_get_string_value(config,"PUERTO_MEMORIA");

    conexion_memoria = crear_conexion(ip_memoria,puerto_memoria);




    //FINALIZACION DEL MODULO

    config_destroy(config);
    return 0;
}
