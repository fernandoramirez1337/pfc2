# gen_data.py
import random
import os

FILE_SIZE = 2 * 1024 * 1024

DATA_DIR = "data"

def generate_random_data(filepath, size):
    """Genera un archivo con bytes aleatorios."""
    print(f"Generando {filepath} ({size // (1024*1024)}MB) con datos aleatorios...")
    with open(filepath, "wb") as f:
        for _ in range(size):
            f.write(bytes([random.randint(0, 255)]))
    print(f"Archivo {filepath} creado exitosamente.")

def generate_pattern_data(filepath, size, pattern_byte):
    """Genera un archivo lleno con un byte de patrón específico."""
    hex_pattern = hex(pattern_byte)
    print(f"Generando {filepath} ({size // (1024*1024)}MB) con patrón de bytes {hex_pattern}...")
    with open(filepath, "wb") as f:
        f.write(bytes([pattern_byte]) * size)
    print(f"Archivo {filepath} creado exitosamente.")

def generate_alternating_pattern_data(filepath, size, pattern_sequence):
    """Genera un archivo con una secuencia de bytes de patrón alternante."""
    pattern_hex_str = "".join([f"{b:02x}" for b in pattern_sequence])
    print(f"Generando {filepath} ({size // (1024*1024)}MB) con patrón de secuencia 0x{pattern_hex_str}...")
    pattern_len = len(pattern_sequence)
    with open(filepath, "wb") as f:
        for i in range(size):
            f.write(bytes([pattern_sequence[i % pattern_len]]))
    print(f"Archivo {filepath} creado exitosamente.")

def main():
    if not os.path.exists(DATA_DIR):
        os.makedirs(DATA_DIR)
        print(f"Directorio '{DATA_DIR}' creado.")

    files_to_generate = [
        ("data_random.bin", generate_random_data, None),
        ("data_zeros.bin", generate_pattern_data, 0x00),
        ("data_ones.bin", generate_pattern_data, 0xFF),
        ("data_pattern_55.bin", generate_pattern_data, 0x55), # Patrón 01010101...
        ("data_pattern_aa.bin", generate_pattern_data, 0xAA), # Patrón 10101010...
        ("data_pattern_seq_0123.bin", generate_alternating_pattern_data, bytes([0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F])), # Contador simple de 16 bytes
    ]

    for filename, generator_func, pattern_arg in files_to_generate:
        filepath = os.path.join(DATA_DIR, filename)
        if pattern_arg is not None:
            generator_func(filepath, FILE_SIZE, pattern_arg)
        else:
            generator_func(filepath, FILE_SIZE)
        print("-" * 30)

    print("Todos los archivos de datos han sido generados en la carpeta 'data'.")

if __name__ == "__main__":
    main()