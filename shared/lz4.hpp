#ifndef GRAEHL__SHARED__LZ4_H
#define GRAEHL__SHARED__LZ4_H

#ifndef LZ4__INLINE
#if defined(GRAEHL__SINGLE_MAIN)
# define LZ4__INLINE 1
#else
# define LZ4__INLINE 0
#endif
#endif

namespace lz4 {
#include "lz4.h"
#include "lz4.c"
}

#endif
