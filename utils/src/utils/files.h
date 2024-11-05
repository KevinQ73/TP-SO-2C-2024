#ifndef FILES_H_
#define FILES_H_

    #include <stdlib.h>
    #include <string.h>
    #include <commons/string.h>
    #include <commons/log.h>
    #include <commons/config.h>
	#include <commons/error.h>
    #include <commons/bitarray.h>

    /**
    * @fn    iniciar_logger
    * @brief Crea la estructura del logger
    * 
    * @param ruta_log      La ruta hacia el archivo donde se van a generar los logs
    * @param nombre_logger El nombre a ser mostrado en los logs
    * @param flag_consola  Si lo que se loguea debe mostrarse por consola
    * @param log_level     El nivel de detalle mínimo a loguear (Ver definición de t_log_level en commons/log.h)
    * @return              Retorna una instancia de logger, o NULL en caso de error.
    */
    t_log*      iniciar_logger(char* ruta_log, char* nombre_logger, int flag_consola, t_log_level log_level);

    /**
    * @fn    eliminar_logger
    * @brief Elimina la estructura del logger
    * 
    * @param logger Logger a destruir.
    */
    void        eliminar_logger(t_log* logger);

    /**
    * @fn    iniciar_config
    * @brief Crea la estructura del config
    * 
    * @param path_config La ruta hacia el archivo config deseado.
    * @return            Retorna un puntero hacia la estructura creada, o NULL
	*                    en caso de no encontrar el archivo en el path especificado.
    */
    t_config*   iniciar_config(char* path_config);

    /**
    * @fn    eliminar_config
    * @brief Elimina la estructura del config
    * 
    * @param config Config a destruir.
    */
    void        eliminar_config(t_config* config);

    t_list* leer_instrucciones(char* path, char* carpeta, t_log* log_modulo);

    char** parsear_instruccion(char* instruccion);

#endif /* FILES_H_ */