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

 vector of word (unsigned).
*/

#ifndef WORDS_JG_2014_12_31_H
#define WORDS_JG_2014_12_31_H
#pragma once

#include <stdlib.h>

typedef unsigned word_t;

typedef struct {
  word_t* x;
  unsigned n;
  unsigned cap;
} words_t;

static inline void words_init(words_t* a) {
  a->x = 0;
  a->n = 0;
}

static inline size_t words_bytes(unsigned n) {
  return sizeof(word_t) * n;
}

static inline void words_push(words_t* a, word_t v) {
  if (!a->x) a->x = (word_t *)malloc(words_bytes(a->cap = 16));
  if (a->n >= a->cap) {
    a->cap *= 3;
    a->cap /= 2;
    a->x = (word_t *)realloc(a->x, words_bytes(a->cap));
  }
  a->x[a->n++] = v;
}

static inline unsigned words_pop(words_t* a) {
  assert(a->n);
  return a->x[--a->n];
}

static inline void words_clear(words_t* a) {
  a->n = 0;
}

static inline void words_destroy(words_t* a) {
  free(a->x);
}

static inline void words_compact(words_t* a) {
  if (a->x) a->x = (word_t *)realloc(a->x, words_bytes(a->cap = a->n));
}

#endif
