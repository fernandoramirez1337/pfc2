#include "SAND.h"

u64 ROTL(u64 x, int shift) {
  return (x << shift) | (x >> (64 - shift));
}

u64 P(u64 x) {
  return ROTL(x & 0x00FF00FF00FF00FF, 56)|ROTL(x & 0xFF00FF00FF00FF00, 24);
}

u64 A16x3(u64 x) {
  for (int i = 0; i < 3; i ++) {
    x = ROTL (x , 60) ;
    u64 t = x >> 56 & 0xF ;
    x ^= (((t << 1)|(t >> 3)) << 60) ^ ((t << 3 & 0xF) << 56);
  }
  return x ;
}

void KeySchedule(const u64 MasterKey[], u64 RoundKey[], int Round, int Dec) {
  RoundKey[1] = MasterKey[1], RoundKey[0] = MasterKey[0];
  for (int r = 0; r < Round - 2; r++)
    RoundKey[r + 2] = A16x3(RoundKey[r + 1]) ^ RoundKey[r] ^ (r + 1);
  if (Dec == 1) {
    for (int r = 0; r < (int)(Round / 2); r++)
      SWAP(RoundKey[r], RoundKey[Round - r - 1]);
  }
}

u64 G0(u64 x) {
  x ^= (x >> 3) & (x >> 2) & 0x1111111111111111;
  x ^= (x << 3) & (x << 2) & 0x8888888888888888;
  return x ;
}

u64 G1 ( u64 x ) {
  x ^= (x >> 1) & (x << 1) & 0x4444444444444444;
  x ^= (x << 1) & (x >> 1) & 0x2222222222222222;
  return x;
}

void Bitslice_Round(const u64 PlainText[], u64 CipherText[], const u64 RoundKey[], int Round) {
  u64 x = PlainText[1], y = PlainText[0];
  for (int r = 0; r < Round; r ++) {
    y ^= P(G0(x) ^ G1(ROTL(x,4))) ^ RoundKey[r];
    SWAP(x,y);
  }
  CipherText [1] = y , CipherText [0] = x ;
}

void ECB_encrypt_Bitslice(const u64 PlainText[], u64 CipherText[], const u64 RoundKey[], int Round, size_t data_size) {
  size_t blocks = data_size / SAND128_BLOCK_SIZE;
  for (size_t i = 0; i < blocks; ++i) {
    Bitslice_Round(PlainText + i * 2, CipherText + i * 2, RoundKey, Round);  // 2 * u64 = 128 bits
  }
}