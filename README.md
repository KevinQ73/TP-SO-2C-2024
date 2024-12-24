# The Last of C - TP Sistemas Operativos

## Autores
- Kevin Quiñones
- Elias Olivencia
- Fabricio Lopez
- Cristian Claros Choque
- Matias Hernandez

## Documentos importantes

[Enunciado del TP](https://docs.google.com/document/d/1KloOXfXQb_c1S3PKDEEywnArX9YB3aApqGVmpNgv9_Q/edit?tab=t.0)

[Documento de pruebas](https://docs.google.com/document/d/10uUK0feflEI5HQVqDOSnEZaWcfi9fapvLcLCsmIgu9Y/edit?tab=t.0)

## Construído con:

- [Oracle VM VirtualBox](https://www.virtualbox.org/wiki/Downloads) - Virtual Machine
- [Visual Studio Code](https://code.visualstudio.com/download) - IDE utilizado
- [Xubuntu - 64 bit](https://faq.utnso.com.ar/vm-gui) - Versión para desarrollo con entorno gráfico liviano (XFCE). Desarrollado por la cátedra de Sistemas Operativos
- [Ubuntu Server (64-bit)](https://faq.utnso.com.ar/vm-server) - Versión de pruebas, sin entorno gráfico. En esta VM se evaluó el TP.
- C - Lenguaje utilizado
- Valgrind y Helgrind - Analizadores de uso de memoria dinámica y sincronización entre hilos.

## Dependencias

Para poder compilar y ejecutar el proyecto, es necesario tener instalada la
biblioteca [so-commons-library](https://github.com/sisoputnfrba/so-commons-library) de la cátedra:

```bash
git clone https://github.com/sisoputnfrba/so-commons-library
cd so-commons-library
make debug
make install
```

## Compilación y ejecución

Cada módulo del proyecto se compila de forma independiente a través de un
archivo `makefile`. Para compilar un módulo, es necesario ejecutar el comando
`make` desde la carpeta correspondiente.

El ejecutable resultante de la compilación se guardará en la carpeta `bin` del
módulo. Ejemplo:

```sh
cd kernel
make
./bin/kernel
```

## Importar desde Visual Studio Code

Para importar el workspace, debemos abrir el archivo `tp.code-workspace` desde
la interfaz o ejecutando el siguiente comando desde la carpeta raíz del
repositorio:

```bash
code tp.code-workspace
```

## Links útiles

- [Documentación oficial de la cátedra](https://docs.utnso.com.ar/)
- [Guía de primeros pasos](https://raniagus.github.io/so-project-template/)
- [Documentación oficial Linux](https://man7.org/index.html)
