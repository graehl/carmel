#ifndef GRAEHL_SHARED__STATIC_ITOA_H
#define GRAEHL_SHARED__STATIC_ITOA_H

#include <graehl/shared/itoa.hpp>
#include <graehl/shared/threadlocal.hpp>


namespace graehl {

namespace {
static const int utoa_bufsize=40; // 64bit safe.
static const int utoa_bufsizem1=utoa_bufsize-1; // 64bit safe.
THREADLOCAL char utoa_buf[utoa_bufsize]; // note: 0 initialized
}

inline char *static_utoa(unsigned n) {
  assert(utoa_buf[utoa_bufsizem1]==0);
  return utoa(utoa_buf+utoa_bufsizem1,n);
}

inline char *static_itoa(int n) {
  assert(utoa_buf[utoa_bufsizem1]==0);
  return itoa(utoa_buf+utoa_bufsizem1,n);
}

}//graehl

#endif
