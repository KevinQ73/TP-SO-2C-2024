#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

    #include <utils-filesystem.h>

    t_log* filesystem_log;
    t_config* filesystem_config;
    t_filesystem filesystem_registro;
    t_bitarray* buffer_bitmap;

    pthread_t hilo_memoria;

    int fd_escucha_memoria;
    int fd_conexion_memoria;

    int bloques_libres;

    void* buffer_bitmap_2;
    void* buffer_bloques;
    void* puntero_bitmap;




    void atender_memoria();

    void* atender_solicitudes(void* fd_conexion);

    int crear_metadata(char* ruta, int bloque_indexado, int tamanio_archivo);
    void modificar_metadata(char* ruta, int bloque_indexado, int tamanio_archivo);
    int obtener_archivo(char* ruta, void (*tipo_creacion)(int), void (*tipo_apertura)(int));

    char* ruta_absoluta(char* ruta_relativa);

    void obtener_datos_filesystem();

    int crear_archivo(char* ruta, void (*tipo_Archivo)(int));
    int abrir_archivo(char* ruta, void (*tipo_Archivo)(int));

    void crear_bitmap(int file_descriptor, int size);
    void crear_bloques(int file_descriptor, int size);

    void abrir_bitmap(int file_descriptor);
    void abrir_bloques(int file_descriptor);

    u_int32_t* buscar_bloques_bitmap(int longitud);
    int asignar_bloques_bitmap(u_int32_t* puntero_bloques, int longitud);
    int escribir_datos_bloque(char* nombre_archivo, u_int32_t* puntero_bloque, int bloques_totales, void* contenido);

    int cantidad_bloques(int tamanio);
    int inicio_bloque(int n_bloque);

    int dump_memory(char* nombre_archivo, int tamanio, void* contenido);


#endif /* FILESYSTEM_H_ */