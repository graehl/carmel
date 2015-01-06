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

    as blocks.h but simpler and per-thread.

    TODO: allow free_1 -> freelist
*/

#ifndef FIXED_POOL_JG_2014_12_31_H
#define FIXED_POOL_JG_2014_12_31_H
#pragma once

#include <stdlib.h>
#include <assert.h>

typedef struct {
  char *p; // position in [curbegin, curlast]
  char *curlast;
  size_t allocsz, blocksz;
  char *curbegin;
  //char **freelist; //TODO: allow freeing individual items for reuse before using more of blocks
} fixed_pool_t;

static inline void fixed_pool_new_block(fixed_pool_t *p, char *prevblock) {
  assert(p->blocksz >= sizeof(char*) + p->allocsz);
  size_t sz = p->blocksz;
  char *curbegin = malloc(sz);
  *(char **)(curbegin) = prevblock;
  p->curbegin = curbegin;
  p->curlast = curbegin + sz - p->allocsz;
  p->p = curbegin + sizeof(char*);
}

static inline void fixed_pool_init_blocksz(fixed_pool_t *p, size_t allocsz, size_t blocksz) {
  p->allocsz = allocsz;
  p->blocksz = blocksz;
  fixed_pool_new_block(p, 0);
}

static inline void fixed_pool_init_n(fixed_pool_t *p, size_t allocsz, size_t nperblock) {
  p->blocksz = sizeof(char*) + (p->allocsz = allocsz) * nperblock;
  fixed_pool_new_block(p, 0);
}

static inline void fixed_pool_destroy(fixed_pool_t *p) {
  char *curbegin = p->curbegin;
  while (curbegin) {
    void *freeing = curbegin;
    curbegin = *(char**)curbegin;
    free(freeing);
  }
}

static inline void* fixed_pool_get(fixed_pool_t *pool) {
  char *p = pool->p;
  if (p > pool->curlast) {
    fixed_pool_new_block(pool, pool->curbegin);
    p = pool->p;
  }
  pool->p = p + pool->allocsz;
  assert(pool->p <= pool->curbegin + pool->blocksz);
  return p;
}

/// can reuse pool (which is now empty)
static inline void fixed_pool_freeall(fixed_pool_t *p) {
  fixed_pool_destroy(p);
  fixed_pool_new_block(p, 0);
}

static inline fixed_pool_t *fixed_pool_new(size_t allocsz, size_t nperblock) {
  fixed_pool_t *r = malloc(sizeof(fixed_pool_t));
  fixed_pool_init_n(r, allocsz, nperblock);
  return r;
}

static inline void fixed_pool_delete(fixed_pool_t *p) {
  fixed_pool_destroy(p);
  free(p);
}


#endif
