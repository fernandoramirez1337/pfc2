#ifndef SAND_H
#define SAND_H

#include <stdio.h>
#include <stdint.h>

typedef uint64_t u64;

#define SAND128_ROUNDS 54
#define SAND128_BLOCK_SIZE 16 
#define SAND128_KEY_SIZE 16 

#define SWAP(x,y) ((x) ^= (y) , (y) ^= (x) , (x) ^= (y))

u64 ROTL(u64 x, int shift);
u64 P(u64 x);
u64 A16x3(u64 x);
void KeySchedule(const u64 MasterKey[], u64 RoundKey[], int Round, int Dec);

u64 G0(u64 x);
u64 G1(u64 x);
void Bitslice_Round(const u64 PlainText[], u64 CipherText[], const u64 RoundKey[], int Round);

void ECB_encrypt_Bitslice(const u64 PlainText[], u64 CipherText[], const u64 RoundKey[], int Round, size_t data_size);

#endif /* SAND_H */