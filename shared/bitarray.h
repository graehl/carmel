/** \file

 .
*/

#ifndef BITARRAY_JG_2015_01_02_H
#define BITARRAY_JG_2015_01_02_H
#pragma once

#include <stdlib.h>

typedef char * bitarray_t;

static inline unsigned bitarray_bytes(unsigned n) {
  return (n+7)>>3;
}

static inline bitarray_t bitarray_new(unsigned n) {
  return calloc(1, bitarray_bytes(n));
}

static inline void bitarray_reset_all(bitarray_t a, unsigned n) {
  memset(a, 0, bitarray_bytes(n));
}

static inline void bitarray_delete(bitarray_t a) {
  free(a);
}

static inline bool bitarray_test_set(bitarray_t a, unsigned i) {
  unsigned c = i >> 3;
  unsigned mask = 1 << (i & 7);
  bool was = a[c] & mask;
  a[c] |= mask;
  return was;
}

static inline bool bitarray_latch(bitarray_t a, unsigned i) {
  return !bitarray_test_set(a, i);
}


#endif
