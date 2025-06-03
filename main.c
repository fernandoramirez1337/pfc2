// main.c
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>   // read, close
#include <fcntl.h>    // open, O_RDONLY
#include <inttypes.h> // PRIu64

#include "SAND.h"

#define TAMANO_MAX_DATOS_A_CARGAR (2 * 1024 * 1024)  // 2 MiB

#define ARCHIVO_DATOS_POR_DEFECTO "data/data_random.bin"

static uint64_t get_time_us(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}

static void phex_line(const uint8_t *buf, size_t len, size_t bytes_por_linea) {
  for (size_t i = 0; i < len; ++i) {
    printf("%02x", buf[i]);
    if ((i + 1) % bytes_por_linea == 0 || i == len - 1) {
      printf("\n");
    } else if ((i + 1) % 2 == 0) {
        printf(" ");
    }
  }
}

static void ImprimirDatos(const char *titulo, const uint8_t *data, size_t total_len, size_t display_len) {
  size_t len_a_mostrar = display_len > total_len ? total_len : display_len;
  printf("%s (%zu bytes en total, mostrando los primeros %zu bytes):\n", titulo, total_len, len_a_mostrar);
  if (len_a_mostrar == 0) {
      printf("(No hay datos para mostrar)\n");
  } else {
      phex_line(data, len_a_mostrar, 16);
  }
  printf("\n");
}

// --- Prueba de Consistencia Cifrado/Descifrado en Modo ECB ---
/**
 * @brief Ejecuta una prueba de consistencia (cifrado seguido de descifrado) en modo ECB.
 * Verifica que el texto plano original se recupera correctamente.
 * @param texto_plano Puntero al buffer con el texto plano original.
 * @param tamano_datos Tamaño de los datos a cifrar/descifrar (debe ser múltiplo del tamaño de bloque).
 * @param clave_maestra Puntero a la clave maestra a utilizar.
 * @param nombre_test Nombre descriptivo para esta prueba de consistencia.
 * @return 1 si la prueba pasa (texto original recuperado), 0 si falla o hay error.
 */
int ejecutar_prueba_consistencia_ecb(const uint8_t* texto_plano, size_t tamano_datos, const u64* clave_maestra, const char* nombre_test) {
    printf("--- Prueba de Consistencia ECB: %s ---\n", nombre_test);
    ImprimirDatos("Clave Maestra", (uint8_t*)clave_maestra, SAND128_KEY_SIZE, SAND128_KEY_SIZE);
    ImprimirDatos("Texto Plano (inicio)", texto_plano, tamano_datos, 64);

    if (tamano_datos == 0 || tamano_datos % SAND128_BLOCK_SIZE != 0) {
        fprintf(stderr, "ERROR en '%s': El tamaño de datos (%zu) debe ser un múltiplo no nulo de SAND128_BLOCK_SIZE (%d).\n",
                nombre_test, tamano_datos, SAND128_BLOCK_SIZE);
        return 0; // Falla por configuración incorrecta
    }

    uint8_t *texto_cifrado = malloc(tamano_datos);
    uint8_t *texto_descifrado  = malloc(tamano_datos);
    if (!texto_cifrado || !texto_descifrado) {
        fprintf(stderr, "ERROR en '%s': Fallo en la asignación de memoria.\n", nombre_test);
        if (texto_cifrado) free(texto_cifrado);
        if (texto_descifrado) free(texto_descifrado);
        return 0; // Falla por error de memoria
    }

    u64 subclaves_ronda_enc[SAND128_ROUNDS];
    u64 subclaves_ronda_dec[SAND128_ROUNDS];

    // Cifrado
    KeySchedule(clave_maestra, subclaves_ronda_enc, SAND128_ROUNDS, 0);
    ECB_encrypt_Bitslice((const u64*)texto_plano, (u64*)texto_cifrado, subclaves_ronda_enc, SAND128_ROUNDS, tamano_datos);
    ImprimirDatos("Texto Cifrado (inicio)", texto_cifrado, tamano_datos, 64);

    // Descifrado
    KeySchedule(clave_maestra, subclaves_ronda_dec, SAND128_ROUNDS, 1);
    ECB_encrypt_Bitslice((const u64*)texto_cifrado, (u64*)texto_descifrado, subclaves_ronda_dec, SAND128_ROUNDS, tamano_datos);
    ImprimirDatos("Texto Plano Descifrado (inicio)", texto_descifrado, tamano_datos, 64);

    int resultado_comparacion = memcmp(texto_plano, texto_descifrado, tamano_datos);
    if (resultado_comparacion == 0) {
        printf("Resultado Prueba de Consistencia ECB (%s): PASA\n\n", nombre_test);
    } else {
        printf("Resultado Prueba de Consistencia ECB (%s): FALLA\n", nombre_test);
        // Búsqueda del primer bloque con discrepancia para depuración
        for (size_t i = 0; i < tamano_datos; i += SAND128_BLOCK_SIZE) {
            if (memcmp(texto_plano + i, texto_descifrado + i, SAND128_BLOCK_SIZE) != 0) {
                printf("Discrepancia detectada en el bloque %zu (offset %zu):\n", i / SAND128_BLOCK_SIZE, i);
                ImprimirDatos("Bloque Texto Plano Original", texto_plano + i, SAND128_BLOCK_SIZE, SAND128_BLOCK_SIZE);
                ImprimirDatos("Bloque Texto Plano Descifrado (erróneo)", texto_descifrado + i, SAND128_BLOCK_SIZE, SAND128_BLOCK_SIZE);
                ImprimirDatos("Bloque Texto Cifrado Correspondiente", texto_cifrado + i, SAND128_BLOCK_SIZE, SAND128_BLOCK_SIZE);
                break;
            }
        }
        printf("\n");
    }

    free(texto_cifrado);
    free(texto_descifrado);
    return resultado_comparacion == 0; // 1 si pasa (son iguales), 0 si falla
}

// --- Benchmark de Rendimiento ---
/**
 * @brief Ejecuta un benchmark de rendimiento para las operaciones de cifrado y descifrado.
 * Mide tiempos para KeySchedule, cifrado ECB y descifrado ECB.
 * @param datos_para_bench Puntero a los datos a utilizar. Este buffer será modificado.
 * @param tamano_datos Tamaño de los datos (debe ser múltiplo del tamaño de bloque).
 * @param clave_maestra Puntero a la clave maestra.
 */
void ejecutar_benchmark_rendimiento(uint8_t* datos_para_bench, size_t tamano_datos, const u64* clave_maestra) {
    printf("--- Ejecutando Benchmark de Rendimiento ---\n");
    printf("Tamaño de datos para benchmark: %.2f MiB (%zu bytes)\n", (double)tamano_datos / (1024.0 * 1024.0), tamano_datos);
    ImprimirDatos("Clave Maestra para Benchmark", (uint8_t*)clave_maestra, SAND128_KEY_SIZE, SAND128_KEY_SIZE);

    if (tamano_datos == 0 || tamano_datos % SAND128_BLOCK_SIZE != 0) {
        fprintf(stderr, "ERROR en Benchmark: El tamaño de datos (%zu) debe ser múltiplo no nulo de SAND128_BLOCK_SIZE (%d).\n",
                tamano_datos, SAND128_BLOCK_SIZE);
        return;
    }

    uint8_t *buffer_salida_temporal = malloc(tamano_datos);
    if (!buffer_salida_temporal) {
        fprintf(stderr, "ERROR en Benchmark: Fallo en la asignación de memoria para buffer temporal.\n");
        return;
    }

    u64 subclaves_ronda_buffer[SAND128_ROUNDS]; // Buffer para almacenar subclaves

    // Benchmark de KeySchedule (Generación de Subclaves)
    uint64_t t_inicio_ks_enc = get_time_us();
    KeySchedule(clave_maestra, subclaves_ronda_buffer, SAND128_ROUNDS, 0);
    uint64_t t_fin_ks_enc = get_time_us();
    printf("\nTiempo de Generación de Subclaves (Cifrado): %" PRIu64 " us\n", t_fin_ks_enc - t_inicio_ks_enc);

    uint64_t t_inicio_ks_dec = get_time_us();
    KeySchedule(clave_maestra, subclaves_ronda_buffer, SAND128_ROUNDS, 1);
    uint64_t t_fin_ks_dec = get_time_us();
    printf("Tiempo de Generación de Subclaves (Descifrado): %" PRIu64 " us\n", t_fin_ks_dec - t_inicio_ks_dec);

    // Benchmark de Cifrado
    u64 subclaves_cifrado[SAND128_ROUNDS];
    KeySchedule(clave_maestra, subclaves_cifrado, SAND128_ROUNDS, 0); // Generar subclaves de cifrado

    uint64_t t_inicio_enc = get_time_us();
    ECB_encrypt_Bitslice((const u64*)datos_para_bench, (u64*)buffer_salida_temporal, subclaves_cifrado, SAND128_ROUNDS, tamano_datos);
    uint64_t t_fin_enc = get_time_us();

    uint64_t usec_enc = t_fin_enc - t_inicio_enc;
    double sec_enc = usec_enc / 1e6;
    double mbps_enc = (sec_enc > 0) ? ((double)tamano_datos / (1024.0 * 1024.0) / sec_enc) : 0;
    size_t num_bloques = tamano_datos / SAND128_BLOCK_SIZE;
    double us_por_bloque_enc = (num_bloques > 0) ? ((double)usec_enc / num_bloques) : 0;

    printf("\nBenchmark de Cifrado (Modo ECB):\n");
    printf("  Tiempo Total de Ejecución: %" PRIu64 " us (%.3f seg)\n", usec_enc, sec_enc);
    printf("  Rendimiento (Throughput):  %.2f MB/s\n", mbps_enc);
    printf("  Latencia por Bloque:       %.3f us/bloque\n", us_por_bloque_enc);

    // Benchmark de Descifrado
    u64 subclaves_descifrado[SAND128_ROUNDS];
    KeySchedule(clave_maestra, subclaves_descifrado, SAND128_ROUNDS, 1); // Generar subclaves de descifrado

    uint64_t t_inicio_dec = get_time_us();
    ECB_encrypt_Bitslice((const u64*)buffer_salida_temporal, (u64*)datos_para_bench, subclaves_descifrado, SAND128_ROUNDS, tamano_datos);
    uint64_t t_fin_dec = get_time_us();

    uint64_t usec_dec = t_fin_dec - t_inicio_dec;
    double sec_dec = usec_dec / 1e6;
    double mbps_dec = (sec_dec > 0) ? ((double)tamano_datos / (1024.0 * 1024.0) / sec_dec) : 0;
    double us_por_bloque_dec = (num_bloques > 0) ? ((double)usec_dec / num_bloques) : 0;

    printf("\nBenchmark de Descifrado (Modo ECB):\n");
    printf("  Tiempo Total de Ejecución: %" PRIu64 " us (%.3f seg)\n", usec_dec, sec_dec);
    printf("  Rendimiento (Throughput):  %.2f MB/s\n", mbps_dec);
    printf("  Latencia por Bloque:       %.3f us/bloque\n\n", us_por_bloque_dec);

    free(buffer_salida_temporal);
}

// --- Configuración y Ejecución de Pruebas de Consistencia ---
/**
 * @brief Define un escenario para una prueba de consistencia.
 */
typedef struct {
    const char* nombre_escenario; ///< Nombre descriptivo del escenario.
    const u64* clave_maestra;     ///< Puntero a la clave maestra para este escenario.
    int usar_datos_completos;     ///< 1 para usar todos los datos disponibles, 0 para usar una porción parcial.
} EscenarioConsistencia_t;

// --- Gestión de Datos de Prueba ---
/**
 * @brief Carga datos desde el archivo especificado.
 * Ajusta el tamaño de los datos para que sea múltiplo del tamaño de bloque del cifrador.
 * @param nombre_archivo_datos Ruta al archivo de datos a cargar.
 * @param buffer_destino Puntero a un `uint8_t*` que recibirá la dirección del buffer asignado con los datos.
 * @param tamano_buffer_solicitado Tamaño del buffer que se intentará asignar y llenar.
 * @param tamano_datos_cargados Puntero a una variable `size_t` donde se almacenará el tamaño real de los datos cargados y ajustados.
 * @return 1 si la preparación de datos fue exitosa y hay datos para probar, 0 en caso de error crítico.
 */
int preparar_datos_de_prueba(const char* nombre_archivo_datos, uint8_t** buffer_destino, size_t tamano_buffer_solicitado, size_t* tamano_datos_cargados) {
    printf("--- Preparando Datos para Pruebas desde '%s' ---\n", nombre_archivo_datos);
    *buffer_destino = malloc(tamano_buffer_solicitado);
    if (!(*buffer_destino)) {
        fprintf(stderr, "CRÍTICO: Fallo al asignar memoria para el buffer de datos de prueba (%zu bytes) para '%s'.\n",
                tamano_buffer_solicitado, nombre_archivo_datos);
        return 0;
    }

    int fd = open(nombre_archivo_datos, O_RDONLY);
    if (fd < 0) {
        char msg_error[256];
        snprintf(msg_error, sizeof(msg_error), "  Error al abrir '%s'", nombre_archivo_datos);
        perror(msg_error);
        fprintf(stderr, "CRÍTICO: No se pudo abrir el archivo de datos especificado '%s'.\n", nombre_archivo_datos);
        free(*buffer_destino);
        *buffer_destino = NULL;
        return 0;
    } else {
        ssize_t bytes_leidos = read(fd, *buffer_destino, tamano_buffer_solicitado);
        close(fd);

        if (bytes_leidos < 0) {
            char msg_error[256];
            snprintf(msg_error, sizeof(msg_error), "  Error de lectura desde '%s'", nombre_archivo_datos);
            perror(msg_error);
            fprintf(stderr, "CRÍTICO: Error al leer desde '%s'. No se pueden continuar las pruebas.\n", nombre_archivo_datos);
            free(*buffer_destino);
            *buffer_destino = NULL;
            return 0;
        } else if (bytes_leidos == 0) {
            fprintf(stderr, "CRÍTICO: El archivo '%s' está vacío. No hay datos para probar.\n", nombre_archivo_datos);
            free(*buffer_destino);
            *buffer_destino = NULL;
            return 0;
        } else if ((size_t)bytes_leidos < tamano_buffer_solicitado) {
             printf("  ADVERTENCIA: El archivo '%s' (%zd bytes) es más pequeño que el tamaño de buffer solicitado (%zu bytes).\n",
                    nombre_archivo_datos, bytes_leidos, tamano_buffer_solicitado);
             *tamano_datos_cargados = bytes_leidos;
        } else {
            *tamano_datos_cargados = tamano_buffer_solicitado;
             printf("  INFO: Se leyeron %zu bytes desde '%s'.\n", *tamano_datos_cargados, nombre_archivo_datos);
        }
    }

    // Ajustar el tamaño real de los datos para que sea un múltiplo del tamaño de bloque
    *tamano_datos_cargados = (*tamano_datos_cargados / SAND128_BLOCK_SIZE) * SAND128_BLOCK_SIZE;
    if (*tamano_datos_cargados == 0) {
        fprintf(stderr, "CRÍTICO: Después del ajuste, no hay suficientes datos (%zu bytes) de '%s' para formar siquiera un bloque.\n",
                *tamano_datos_cargados, nombre_archivo_datos);
        if (*buffer_destino) free(*buffer_destino);
        *buffer_destino = NULL;
        return 0;
    }
    printf("  Datos listos desde '%s'. Tamaño final para pruebas: %zu bytes.\n\n", nombre_archivo_datos, *tamano_datos_cargados);
    return 1; // Éxito en la preparación de datos
}

// --- Función Principal (main) ---
// Acepta argumentos de línea de comandos: ./sand_bench [ruta_al_archivo_de_datos]
int main(int argc, char *argv[]) {
    int estado_general_pruebas = EXIT_SUCCESS; // Refleja el resultado de las pruebas críticas

    const char* archivo_datos_a_usar;
    if (argc > 1) {
        archivo_datos_a_usar = argv[1];
        printf("INFO: Se utilizará el archivo de datos especificado por argumento: '%s'\n", archivo_datos_a_usar);
    } else {
        archivo_datos_a_usar = ARCHIVO_DATOS_POR_DEFECTO;
        printf("INFO: No se especificó archivo de datos. Usando por defecto: '%s'\n", archivo_datos_a_usar);
    }
    printf("\n");

    // 1. Preparación de Datos Globales para Pruebas
    uint8_t *buffer_datos_global = NULL;
    size_t tamano_datos_real = 0;

    if (!preparar_datos_de_prueba(archivo_datos_a_usar, &buffer_datos_global, TAMANO_MAX_DATOS_A_CARGAR, &tamano_datos_real)) {
        // Mensaje de error crítico ya impreso en preparar_datos_de_prueba
        return EXIT_FAILURE;
    }

    // Definición de Claves de Prueba
    const u64 clave_maestra_referencia[SAND128_KEY_SIZE / sizeof(u64)] = {0xa6d2ae2816157e2bULL, 0x3c4fcf098815f7abULL};
    const u64 clave_maestra_ceros[SAND128_KEY_SIZE / sizeof(u64)] = {0}; // Clave compuesta solo de ceros

    // 2. Ejecución de Pruebas de Consistencia Cifrado/Descifrado
    printf("\n--- Ejecutando Escenarios de Pruebas de Consistencia (usando datos de '%s') ---\n", archivo_datos_a_usar);
    // Define el tamaño para las pruebas "parciales" (más rápidas)
    size_t tamano_datos_parciales = tamano_datos_real > (64 * 1024) ? (64 * 1024) : tamano_datos_real; // Usa hasta 64KB o menos si no hay tantos datos
    tamano_datos_parciales = (tamano_datos_parciales / SAND128_BLOCK_SIZE) * SAND128_BLOCK_SIZE; // Ajustar a múltiplo de bloque

    EscenarioConsistencia_t escenarios_consistencia[] = {
        {"Clave de Referencia (Datos Parciales)", clave_maestra_referencia, 0}, // 0 indica usar tamano_datos_parciales
        {"Clave de Ceros (Datos Parciales)",      clave_maestra_ceros,      0},
        {"Clave de Referencia (Datos Completos)", clave_maestra_referencia, 1}  // 1 indica usar tamano_datos_real
    };
    size_t num_escenarios = sizeof(escenarios_consistencia) / sizeof(EscenarioConsistencia_t);

    for (size_t i = 0; i < num_escenarios; ++i) {
        EscenarioConsistencia_t escenario_actual = escenarios_consistencia[i];
        size_t tamano_para_escenario = escenario_actual.usar_datos_completos ? tamano_datos_real : tamano_datos_parciales;
        char nombre_test_completo[256];
        snprintf(nombre_test_completo, sizeof(nombre_test_completo), "%s - Archivo: %s", escenario_actual.nombre_escenario, archivo_datos_a_usar);

        if (tamano_para_escenario > 0) {
            if (!ejecutar_prueba_consistencia_ecb(buffer_datos_global,
                                                  tamano_para_escenario,
                                                  escenario_actual.clave_maestra,
                                                  nombre_test_completo)) {
                printf("ERROR CRÍTICO: La Prueba de Consistencia '%s' falló.\n", nombre_test_completo);
                estado_general_pruebas = EXIT_FAILURE;
            }
        } else {
            printf("INFO: Omitiendo escenario de consistencia '%s' porque el tamaño de datos aplicable es cero.\n",
                   nombre_test_completo);
        }
    }

    // 3. Ejecución del Benchmark de Rendimiento
    if (tamano_datos_real > 0) {
        // Es importante usar una copia de los datos para el benchmark si las operaciones de benchmark
        // modifican el buffer de entrada (como lo hace nuestra función de descifrado).
        uint8_t *datos_para_benchmark = malloc(tamano_datos_real);
        if (!datos_para_benchmark) {
            fprintf(stderr, "\nERROR CRÍTICO: Fallo al asignar memoria para los datos del benchmark. Omitiendo benchmark.\n");
        } else {
            memcpy(datos_para_benchmark, buffer_datos_global, tamano_datos_real); // Copiar datos originales
            printf("\n--- Benchmark usando datos de '%s' ---\n", archivo_datos_a_usar);
            ejecutar_benchmark_rendimiento(datos_para_benchmark, tamano_datos_real, clave_maestra_referencia);
            free(datos_para_benchmark);
        }
    } else {
        printf("\nINFO: Omitiendo benchmark de rendimiento porque no hay datos cargados (tamano_datos_real es cero).\n");
    }

    // 4. Limpieza Final de Recursos Globales
    printf("\n--- Liberando Recursos Globales ---\n");
    if (buffer_datos_global) {
        free(buffer_datos_global);
        buffer_datos_global = NULL;
        printf("Buffer de datos global liberado.\n");
    }

    // --- Conclusión de las Pruebas ---
    printf("\n--- Todas las Pruebas y Benchmarks Han Concluido para el archivo '%s' ---\n", archivo_datos_a_usar);
    if (estado_general_pruebas == EXIT_SUCCESS) {
        printf("RESULTADO FINAL: Todas las pruebas críticas de correctitud PASARON.\n");
    } else {
        printf("RESULTADO FINAL: Al menos una prueba crítica de correctitud FALLÓ.\n");
    }

    return estado_general_pruebas;
}