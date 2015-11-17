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

    usage:

    ALLOC_STACK(buf,n,256); // declares void *buf, initialized good for exactly n bytes

    // note: n is an arbitrary size_t expression; 256 is some constant (that can go in char buf[256]);

    memcpy(buf,from,n); // buf may be passed to other fns

    // on function return, memory is automatically freed

    MUST NOT USE in exception handler

    if your max size is too large, you can expect a segmentation fault depending
    on stack size limit (with multithreading, often a small per-thread stack is
    used). don't use ALLOC_STACK in a deeply recursive fn.
*/

#ifndef GRAEHL__ALLOC_SMALL_HPP
#define GRAEHL__ALLOC_SMALL_HPP

/**
   equivalent to variable sized (C99/gcc): char anon_buf[sizeExpr]; void *voidPtrVarName=anon_buf;
*/
#define ALLOC_STACK(voidPtrVarName, sizeExpr, maxsizeConst) ALLOC_STACK_LINE( __LINE__ , voidPtrVarName, sizeExpr, maxsizeConst)

#ifndef GRAEHL_USE_ALLOCA
// if false, waste a fixed (maximum) sized stack block always, whether actual size is smaller or larger (heap)
// if true, use alloca when possible, otherwise waste no space and go straight to heap
# define GRAEHL_USE_ALLOCA 1
#endif

#if GRAEHL_USE_ALLOCA
#if _MSC_VER
# define GRAEHL_ALLOCA ::_alloca
# include <malloc.h>
#elif __APPLE__
# include <stdlib.h>
# define GRAEHL_ALLOCA ::alloca
#elif __linux__
# include <stdlib.h>
# include <alloca.h>
# define GRAEHL_ALLOCA ::alloca
#else
# undef GRAEHL_USE_ALLOCA
# define GRAEHL_USE_ALLOCA 0
#endif
#endif

#include <cstdlib>

/**
   does X##Y (preprocessor token concatenation) with full macro
   expansion. without this indirection, you cannot do var##__LINE__ (it expands
   to var__LINE__ and not var1298).
*/
#define GRAEHL_JOIN( X, Y ) GRAEHL_IMPL_JOIN( X, Y )
#define GRAEHL_IMPL_JOIN( X, Y ) GRAEHL_IMPL_JOIN2(X, Y)
#define GRAEHL_IMPL_JOIN2( X, Y ) X##Y

#if GRAEHL_USE_ALLOCA
#define ALLOC_STACK_LINE(line, name, sizeExpr, maxsizeConst)               \
  const std::size_t GRAEHL_JOIN(stackAllocSz, line) = (sizeExpr);          \
  graehl::alloc_large GRAEHL_JOIN(stackAllocBig, line);                  \
  void *name = (GRAEHL_JOIN(stackAllocSz, line) > maxsizeConst) ?          \
      GRAEHL_JOIN(stackAllocBig, line).alloc(GRAEHL_JOIN(stackAllocSz, line)) \
      : GRAEHL_ALLOCA(GRAEHL_JOIN(stackAllocSz, line))
#else
#define ALLOC_STACK_LINE(line, name, sizeExpr, maxsizeConst)               \
  char stackAllocBuf ## line [maxsizeConst];                            \
  const std::size_t GRAEHL_JOIN(stackAllocSz, line) = (sizeExpr);          \
  graehl::alloc_large GRAEHL_JOIN(stackAllocBig, line);                  \
  void *name = (GRAEHL_JOIN(stackAllocSz, line) > maxsizeConst) ?          \
      GRAEHL_JOIN(stackAllocBig, line).alloc(GRAEHL_JOIN(stackAllocSz, line)) \
      : stackAllocBuf
#endif


namespace graehl {

/**
   used (with alloc(largeSize) in case stack is insufficient for request. user
   may call clear() early; otherwise clear() is called when alloc_large goes out
   of scope
*/
struct alloc_large {
  void *p;

  /**
     no storage allocated.
  */
  alloc_large() : p() {}

  /**
     like calling alloc(sz) on an alloc_large()
  */
  alloc_large(std::size_t sz) : p(malloc(sz)) {}

  /**
     we may replace these with something other than malloc/free later.
  */

  static inline void *malloc(std::size_t sz) { return ::malloc(sz); }
  /**
     if alloced is 0, do nothing. else it must have been returned by
     malloc. every malloc must be freed exactly once
  */
  static inline void free(void *alloced) { ::free(alloced); }

  /**
     set this->p to a malloc of size z.
  */
  void *alloc(std::size_t sz) {
    return (p = malloc(sz));
  }
  /**
     free this->p if it was alloc since last clear (can call repeatedly).
  */
  void clear() {
    free(p);
    p = 0;
  }
  /**
     clear; alloc(sz);
  */
  void *realloc(std::size_t sz) {
    clear();
    return alloc(sz);
  }
  ~alloc_large() {
    free(p);
  }
};

}


#if GRAEHL_TEST
#include <boost/test/unit_test.hpp>

namespace graehltest {

BOOST_AUTO_TEST_CASE( test_graehl_alloc_stack ) {
  const unsigned sz1 = 10;
  const unsigned sz2 = 1000000;
  ALLOC_STACK(buf1, sz1*sizeof(unsigned), 256);
  ALLOC_STACK(buf2, sz2*sizeof(unsigned), 256);
  unsigned *c1 = (unsigned *)buf1;
  unsigned *c2 = (unsigned *)buf2;
  for (unsigned i = 0; i<sz2; ++i) {
    unsigned i1 = i%sz1;
    c1[i1] = sz2-i1;
    c2[i] = i1;
  }
  for (unsigned i = 0; i<sz2; ++i) {
    unsigned i1 = i%sz1;
    BOOST_REQUIRE_EQUAL(c1[i1], sz2-i1);
    BOOST_REQUIRE_EQUAL(c2[i], i1);
  }
}

}

#endif

#endif
