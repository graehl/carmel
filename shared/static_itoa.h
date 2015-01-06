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
#ifndef GRAEHL_SHARED__STATIC_ITOA_H
#define GRAEHL_SHARED__STATIC_ITOA_H

#include <graehl/shared/itoa.hpp>
#include <graehl/shared/threadlocal.hpp>


namespace graehl {

namespace {
static const int utoa_bufsize = 40; // 64bit safe.
static const int utoa_bufsizem1 = utoa_bufsize-1; // 64bit safe.
THREADLOCAL char utoa_buf[utoa_bufsize]; // note: 0 initialized
}

inline char *static_utoa(unsigned n) {
  assert(utoa_buf[utoa_bufsizem1]==0);
  return utoa(utoa_buf+utoa_bufsizem1, n);
}

inline char *static_itoa(int n) {
  assert(utoa_buf[utoa_bufsizem1]==0);
  return itoa(utoa_buf+utoa_bufsizem1, n);
}

}//graehl

#endif
