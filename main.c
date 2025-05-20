// main.c
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include "SAND.h"

#define DATA_SIZE (2 * 1024 * 1024)  // 2 MB

static uint64_t get_time_us(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

// Print 16 bytes in hex
static void phex(const uint8_t *buf) {
  for (int i = 0; i < 16; i++) {
    printf("%02x", buf[i]);
  }
  printf("\n");
}

static void PrintData(const char *title, const uint8_t *data, size_t len) {
  printf("%s:\n", title);
  for (size_t i = 0; i < len; i += 16) {
    phex(data + i);
  }
  printf("\n");
}

int main(void) {
  uint8_t *plaintext = malloc(DATA_SIZE);
  uint8_t *ciphertext = malloc(DATA_SIZE);
  uint8_t *decrypted = malloc(DATA_SIZE);
  if (!plaintext || !ciphertext || !decrypted) {
    fprintf(stderr, "Allocation failed\n");
    return EXIT_FAILURE;
  }

  int fd = open("data.bin", O_RDONLY);
  if (fd < 0) {
    perror("open");
    return EXIT_FAILURE;
  }
  if (read(fd, plaintext, DATA_SIZE) != DATA_SIZE) {
    fprintf(stderr, "Read error\n");
    close(fd);
    return EXIT_FAILURE;
  }
  close(fd);

  // NIST key
  u64 master_key[2] = {
    0xa6d2ae2816157e2bULL,
    0x3c4fcf098815f7abULL
  };
  u64 round_key[SAND128_ROUNDS];

  // Encryption
  KeySchedule(master_key, round_key, SAND128_ROUNDS, 0);
  PrintData("Key (16 bytes)", (uint8_t*)master_key, 16);
  PrintData("Plaintext (first 64B)", plaintext, 64);

  uint64_t t0 = get_time_us();
  ECB_encrypt_Bitslice((const u64*)plaintext,
                       (u64*)ciphertext,
                       round_key,
                       SAND128_ROUNDS,
                       DATA_SIZE);
  uint64_t t1 = get_time_us();

  PrintData("Ciphertext (first 64B)", ciphertext, 64);

  uint64_t usec = t1 - t0;
  double sec = usec / 1e6;
  double mbps = (double)DATA_SIZE / (1024.0*1024.0) / sec;
  size_t blocks = DATA_SIZE / SAND128_BLOCK_SIZE;
  double us_per_block = (double)usec / blocks;

  printf("Exec Time:    %" PRIu64 " us\n", usec);
  printf("Throughput:   %.2f MB/s\n", mbps);
  printf("Latency:      %.3f us/block\n\n", us_per_block);

  // Decryption
  KeySchedule(master_key, round_key, SAND128_ROUNDS, 1);
  ECB_encrypt_Bitslice((const u64*)ciphertext,
                       (u64*)decrypted,
                       round_key,
                       SAND128_ROUNDS,
                       DATA_SIZE);
  PrintData("Decrypted (first 64B)", decrypted, 64);

  if (memcmp(plaintext, decrypted, DATA_SIZE) == 0) {
    printf("Decryption test: PASS\n");
  } else {
    printf("Decryption test: FAIL\n");
  }

  free(plaintext);
  free(ciphertext);
  free(decrypted);
  return EXIT_SUCCESS;
}