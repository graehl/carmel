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
#include "blocks.h"
#include "vector.h"
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#ifdef NDEBUG
static size_t blocksz = 1024 * 1024;
#else
static size_t blocksz = 1 * 1024;
#endif

char* blocks_current;
size_t blocks_currentsz;
char* blocks_endfree;

static char* blocks_current_chunk;
static vector_t blocks_tofree;

static inline void blocks_nextsz(size_t req) {
  while (req > blocksz) {
    blocksz *= 3;
    blocksz /= 2;
  }
}

static inline void blocks_new_current(void* newblock) {
  assert(vector_initialized(&blocks_tofree));
  blocks_current_chunk = blocks_current = (char*)newblock;
  blocks_endfree = blocks_current_chunk + blocksz;
}

void blocks_realloc_impl(size_t req) {
  assert(vector_initialized(&blocks_tofree) == (bool)blocks_current_chunk);
  if (blocks_current_chunk) {
    if (blocks_current + req > blocks_endfree) {
      vector_push(&blocks_tofree, blocks_current_chunk);
      blocks_nextsz(req);
      bool const solo = blocks_current_chunk == blocks_current;
      if (solo) {
        blocks_new_current(
            realloc(blocks_current, blocksz));  // even though this copies some extra bytes, a solo block is
        // probably already mostly full
      } else {
        void* n = malloc(blocksz);
        memcpy(n, blocks_current_chunk, blocks_currentsz);
        blocks_new_current(n);
      }
    }
  } else {
    vector_init(&blocks_tofree, 256);
    blocks_nextsz(req);
    blocks_new_current(malloc(blocksz));
  }
}

static inline void blocks_forget_impl() {
#if !BLOCKS_DEBUG
  assert(vector_initialized(&blocks_tofree) == (bool)blocks_current_chunk);
  vector_destroy(&blocks_tofree);
  blocks_current_chunk = blocks_endfree = 0;  // force vector_init next time
  blocks_currentsz = 0;
#endif
}

void blocks_freeall() {
#if !BLOCKS_DEBUG
  assert(vector_initialized(&blocks_tofree) == (bool)blocks_current_chunk);
  if (blocks_current_chunk) {
    free(blocks_current_chunk);
    for (int i = 0; i < blocks_tofree.n; ++i) free(blocks_tofree.x[i]);
    blocks_forget_impl();
  }
#endif
}

void blocks_protectall() {
#if !BLOCKS_DEBUG
  if (blocks_current_chunk) blocks_forget_impl();
#endif
}


extern inline void blocks_cancel();
extern inline void* blocks_malloc(size_t);
extern inline void* blocks_malloc_aligned(size_t);
extern inline void* blocks_realloc(size_t);
extern inline void* blocks_continue(size_t);
