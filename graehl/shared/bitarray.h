// Copyright 2014 Jonathan Graehl - http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
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
