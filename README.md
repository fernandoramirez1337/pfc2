# Proyecto SAND-128

Este repositorio contiene una implementación en C del cifrado ligero SAND-128, junto con scripts y herramientas para la generación de datos de prueba, compilación y ejecución de pruebas de rendimiento y consistencia, incluyendo la ejecución en un entorno Docker con recursos limitados.

## Estructura de Archivos

* **`SAND.h`**: Cabecera con las declaraciones públicas: tipos, constantes, prototipos de funciones y macros necesarias para la implementación del cifrado.
* **`SAND.c`**: Implementación de las funciones definidas en `SAND.h`: rotaciones, subcapas `G0`/`G1`, permutación `P`, expansión de clave (`KeySchedule`), rondas (`Bitslice_Round`) y modo ECB (`ECB_encrypt_Bitslice`).
* **`main.c`**: Programa principal que:
    1. Acepta un archivo de datos como argumento de línea de comandos. Si no se proporciona ninguno, usa la ruta definida en `ARCHIVO_DATOS_POR_DEFECTO` (ej. `../data/data_random.bin` cuando se ejecuta desde `build/` localmente, o `data/data_random.bin` cuando se ejecuta desde el `WORKDIR /app` en Docker con el volumen montado correctamente).
    2. Carga los datos del archivo especificado.
    3. Carga claves de prueba (ej. clave de referencia, clave de ceros).
    4. Ejecuta múltiples escenarios de pruebas de consistencia (cifrado/descifrado) sobre porciones parciales y completas de los datos.
    5. Muestra los primeros 64 bytes de texto plano, texto cifrado y texto descifrado para cada prueba.
    6. Realiza un benchmark de rendimiento (KeySchedule, Cifrado, Descifrado), midiendo tiempo y throughput (MB/s), y latencia por bloque.
    7. Verifica la integridad de los datos después del descifrado.
    8. (Nota: La funcionalidad de Known Answer Tests (KATs) ha sido eliminada de esta versión de `main.c` para simplificar, pero la estructura para KATs existía en versiones previas y puede ser reincorporada si se obtienen vectores de prueba oficiales).
* **`CMakeLists.txt`**: Archivo de configuración para CMake que define la construcción de la biblioteca estática `sand` y el ejecutable `sand_bench`.
* **`gen_data.py`**: Script en Python que crea un directorio `data/` (si no existe) y genera múltiples ficheros de prueba (2 MiB cada uno por defecto) con diferentes patrones (aleatorio, ceros, unos, secuencias específicas) dentro de él.
* **`Dockerfile`**: Define un entorno contenedorizado para compilar y ejecutar `sand_bench`, permitiendo simular condiciones de recursos limitados (CPU, memoria). Su `ENTRYPOINT` es `/app/build/sand_bench`.
* **`.dockerignore`**: Especifica los archivos y directorios que deben ser ignorados al construir la imagen Docker, optimizando el contexto de compilación.
* **`data/` (carpeta)**: Creada por `gen_data.py`. Contiene los archivos binarios con datos de prueba (ej. `data_random.bin`, `data_zeros.bin`, etc.). Esta carpeta es ignorada por Git (si se añade a `.gitignore`) y por Docker (debido a `.dockerignore` para la copia inicial, pero se monta como volumen en tiempo de ejecución).
* **`build/` (carpeta)**: Directorio donde CMake genera sus archivos de compilación (`Makefile`, `CMakeFiles/`, etc.). No se incluye en el repositorio gracias a `.gitignore` (recomendado).

## Instrucciones de Uso

### 1. Generar Datos de Prueba

Ejecuta el script de Python para crear la carpeta `data/` y generar los diversos archivos de datos de prueba:

 ```bash
 python3 gen_data.py
 ```

### 2. Compilación Local (macOS/Linux)

Para compilar el proyecto en tu máquina local usando CMake:

 ```bash
 # Crear el directorio de compilación (si no existe) y entrar en él
 mkdir -p build && cd build
 
 # Ejecutar CMake para configurar el proyecto (desde el directorio build)
 cmake ..
 
 # Compilar el proyecto
 make
 ```

El ejecutable se encontrará en `build/sand_bench`.

### 3. Ejecución Local

Puedes ejecutar el programa de pruebas desde el directorio `build/`:

* **Usando el archivo de datos por defecto (definido en `main.c` como `../data/data_random.bin` relativo a `build/`):**

    ```bash
    ./sand_bench
    ```

* **Especificando un archivo de datos diferente:**

    ```bash
    ./sand_bench ../data/data_zeros.bin
    ```

    o

    ```bash
    ./sand_bench ../data/data_pattern_55.bin
    ```

El programa mostrará las claves usadas, los primeros bloques de texto plano y cifrado para cada prueba, los resultados de las pruebas de consistencia, y métricas de rendimiento.

### 4. Uso con Docker (para simular IoT y entorno controlado)

#### a. Construir la Imagen Docker

Desde el directorio raíz del proyecto (donde se encuentra el `Dockerfile`):

 ```bash
 docker build -t sand-cipher-iot .
 ```

#### b. Ejecutar el Contenedor Docker

* **Ejecución usando el archivo de datos por defecto de `main.c` (que es `data/data_random.bin` relativo al `WORKDIR /app` en Docker) y límites de recursos:**
    Asegúrate de estar en el directorio raíz del proyecto para que `$(pwd)/data` resuelva correctamente.

    ```bash
    docker run --rm --cpus=".5" --memory="128m" -v "$(pwd)/data:/app/data" sand-cipher-iot
    ```

    Esto ejecuta `sand_bench` usando `/app/data/data_random.bin` dentro del contenedor (asumiendo que `ARCHIVO_DATOS_POR_DEFECTO` en `main.c` es `data/data_random.bin` para el contexto del contenedor), con 0.5 CPU y 128MB de RAM.

* **Ejecución con un archivo de datos específico y diferentes límites:**

    ```bash
    docker run --rm --cpus=".25" --memory="64m" -v "$(pwd)/data:/app/data" sand-cipher-iot data/data_ones.bin
    ```

    Esto ejecuta `sand_bench` usando `/app/data/data_ones.bin` dentro del contenedor, con 0.25 CPU y 64MB de RAM.

    **Explicación del comando `docker run`:**

  * `--rm`: Elimina el contenedor automáticamente cuando el programa termina.
  * `--cpus=".X"`: Asigna X fracción de un núcleo de CPU al contenedor.
  * `--memory="YM"`: Limita la memoria RAM a Y Megabytes.
  * `-v "$(pwd)/data:/app/data"`: Monta el directorio `data` de tu máquina anfitriona (host) en el directorio `/app/data` dentro del contenedor. Esto hace que los archivos generados por `gen_data.py` estén disponibles para el programa que corre en Docker.
  * `sand-cipher-iot`: Nombre de la imagen Docker a ejecutar.
  * `data/data_XXXX.bin` (opcional): Argumento pasado al `ENTRYPOINT` (`/app/build/sand_bench`) dentro del contenedor, especificando el archivo de datos a utilizar (relativo a `/app` que es el `WORKDIR`).

## Salida del Programa

El programa `sand_bench` mostrará:

* El archivo de datos que se está utilizando.
* Para cada escenario de prueba de consistencia:
* La clave maestra.
  * Los primeros 64 bytes del texto plano original.
  * Los primeros 64 bytes del texto cifrado resultante.
  * Los primeros 64 bytes del texto plano descifrado.
  * Un resultado de `PASA` o `FALLA` para la prueba de consistencia.
* Para el benchmark de rendimiento:
  * Tiempos de generación de subclaves (KeySchedule).
  * Tiempo total de ejecución para cifrado y descifrado.
  * Rendimiento (Throughput) en MB/s.
  * Latencia por bloque en microsegundos (µs/bloque).
* Un resumen final indicando si todas las pruebas críticas de correctitud pasaron.
