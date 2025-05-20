# Proyecto SAND-128

Este repositorio contiene una implementación en C del cifrado ligero SAND-128, junto con un script de generación de datos aleatorios y herramientas para compilar y ejecutar pruebas de rendimiento.

## Estructura de archivos

* **SAND.h**
  Cabecera con las declaraciones públicas: tipos, constantes, prototipos de funciones y macros necesarias para la implementación del cifrado.

* **SAND.c**
  Implementación de las funciones definidas en `SAND.h`: rotaciones, subcapas `G0`/`G1`, permutación `P`, expansión de clave (`KeySchedule`), rondas Feistel (`Bitslice_Round`) y modo ECB (`ECB_encrypt_Bitslice`).

* **main.c**
  Programa principal que:

  1. Lee 2 MiB de datos desde `data.bin`.
  2. Carga la clave NIST y genera las rondas de cifrado/descifrado.
  3. Muestra los primeros 64 bytes de texto plano, cifra, mide tiempo y rendimiento, luego descifra y verifica la integridad.

* **CMakeLists.txt**
  Archivo de configuración para CMake que define la construcción de la biblioteca estática `sand`, el ejecutable `sand_bench` y sus dependencias.

* **gen\_data.py**
  Script en Python que genera un fichero `data.bin` de 2 MiB con bytes aleatorios para usar como entrada en las pruebas.

* **data.bin**
  Archivo binario de 2 MiB con datos de prueba aleatorios. Se genera ejecutando `python3 gen_data.py`.

* **.gitignore**
  Define patrones de archivos y carpetas que no deben versionarse: binarios, objetos intermedios, directorios de compilación, archivos de editor y datos generados.

* **build/** (carpeta)
  Directorio donde CMake genera sus archivos de compilación (`Makefile`, `CMakeFiles/`, etc.). No se incluye en el repositorio gracias a `.gitignore`.

## Instrucciones de uso

1. **Generar datos de prueba**

   ```bash
   python3 gen_data.py
   ```

2. **Compilar con CMake**

   ```bash
   mkdir -p build && cd build
   cmake ..
   make
   ```

3. **Ejecutar el benchmark**

   ```bash
   ./sand_bench
   ```

El programa mostrará la clave usada, los primeros bloques de texto plano y cifrado, además de métricas de rendimiento como tiempo de ejecución, rendimiento en MB/s y latencia por bloque, y un test de descifrado para verificar la corrección.
