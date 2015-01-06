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

    single (global) memory pool: large mallocs of arrays of relocatable stuff
    (i.e. no pointers into it) that will only be freed all at once
*/

#ifndef BLOCKS_JG_2014_12_09_H
#define BLOCKS_JG_2014_12_09_H
#pragma once

#define BLOCKS_DEBUG 0
// clang analayzer / valgrind may be confused by custom allocation - so set to 1 if you're bug-hunting

#include <stdlib.h>
#include <stddef.h>

extern char* blocks_current;
extern size_t blocks_currentsz;
extern char* blocks_endfree;

/**
   optimal_align can be a power of 2 only. 8 should be sufficient (for double
   alignment). it's on you to align all of your requests, though.
*/
enum { optimal_align = 8, optimal_align_mask = optimal_align - 1 };

static inline size_t optimal_aligned_size(size_t req) {
  req += optimal_align_mask;
  req &= ~(size_t)optimal_align_mask;
  return req;
}

static inline void* optimal_align_up(char* p) {
  p += optimal_align_mask;
  p -= optimal_align_mask & (ptrdiff_t)p;
  return p;
}


/**
   current blocks_malloc/blocks_realloc content:
*/
static inline void* blocks_begin(void) {
  return (void*)blocks_current;
}

static inline void* blocks_end(void) {
  return (void*)(blocks_current + blocks_currentsz);
}

void blocks_realloc_impl(size_t req_larger_total);

/**
   free all storage so far (you can continue to blocks_malloc etc. after but
   previously returned values are invalid)
*/
void blocks_freeall();

/**
   protect anything so far from freeall()
*/
void blocks_protectall();

static inline void blocks_new() {
#if BLOCKS_DEBUG
  blocks_current = 0;
#else
  blocks_current += blocks_currentsz;
#endif
  blocks_currentsz = 0;
}

/**
   start a new block; you can write to the req bytes starting at return.
*/
inline void* blocks_malloc(size_t req) {
#if BLOCKS_DEBUG
  return blocks_current = malloc(blocks_currentsz = req);
#endif
  blocks_current += blocks_currentsz;
  if (blocks_current + req > blocks_endfree) {
    blocks_currentsz = 0;
    blocks_realloc_impl(req);
  }
  blocks_currentsz = req;
  return blocks_current;
}

inline void* blocks_malloc_aligned(size_t req) {
  return blocks_malloc(optimal_aligned_size(req));
}

/**
   allocate req bytes after block_start. returns the position of the *start*
   bytes. if you don't realloc any more, you're done and can start storing
   pointers into the block.
*/
inline void* blocks_realloc(size_t req) {
#if BLOCKS_DEBUG
  return blocks_current = realloc(blocks_current, blocks_currentsz = req);
#endif
  blocks_realloc_impl(req);
  blocks_currentsz = req;
  return blocks_current;
}

/**
   drop the current block_malloc item.
*/
inline void blocks_cancel() {
#if BLOCKS_DEBUG
  free(blocks_current);
#endif
  blocks_currentsz = 0;
}

/**
   grow currently open allocation.
*/
inline void* blocks_continue(size_t req_additional) {
#if BLOCKS_DEBUG
  return blocks_realloc(blocks_currentsz + req_additional);
#endif
  size_t prevsz = blocks_currentsz;
  return (char*)blocks_realloc(prevsz + req_additional) + prevsz;
}


#endif
