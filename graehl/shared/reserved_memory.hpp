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
#ifndef GRAEHL__SHARED__RESERVED_MEMORY_HPP
#define GRAEHL__SHARED__RESERVED_MEMORY_HPP

#include <cstddef>
#include <new>

#ifndef GRAEHL__DEFAULT_SAFETY_SIZE
# define GRAEHL__DEFAULT_SAFETY_SIZE (1024*1024)
# endif
// graehl: when you handle exceptions, you might be out of memory - this little guy grabs extra memory on startup, and frees it on request - of course, if you recover from an exception, you'd better restore the safety (note: you can check the return on consume to see if you really got anything back)

namespace graehl {

struct reserved_memory {
  std::size_t safety_size;
  void *safety;
  reserved_memory(std::size_t safety_size = GRAEHL__DEFAULT_SAFETY_SIZE) : safety_size(safety_size)
  {
    init_safety();
  }
  ~reserved_memory()
  {
    use();
  }
  bool use() {
    if (safety) {
      ::operator delete(safety); // ok if NULL
      safety = 0;
      return true;
    } else
      return false;
  }
  void restore(bool allow_exception = true) {
    if (safety)
      return;
    if (allow_exception)
      init_safety();
    else
      try {
        safety=::operator new(safety_size);
      } catch(...) {
        safety = 0;
      }
  }
 private:
  void init_safety()
  {
    safety=::operator new(safety_size);
  }
};

}//graehl

#endif
