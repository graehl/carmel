// Copyright 2014 Jonathan Graehl-http://graehl.org/
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

    add thread safety to the thread-unsafe trick of saving/restoring globals in local holders to simulate
   dynamic scoping.

    this implementation relies on the correctness of compiler thread-local attributes for globals. this isn't
   safe across DLLs in windows.

    also, destructors won't necessarily be called for thread specific data. so use THREADLOCAL only on
   pointers and plain-old-data
*/

#ifndef GRAEHL_SHARED__THREADLOCAL_HPP
#define GRAEHL_SHARED__THREADLOCAL_HPP
#pragma once
#include <boost/utility.hpp>  // for BOOST_NO_MT
#include <graehl/shared/cpp11.hpp>

#ifdef BOOST_NO_MT
#define THREADLOCAL
#else

#if GRAEHL_CPP11
#define THREADLOCAL thread_local
#define GRAEHL_HAVE_THREADLOCAL 1
#else

#ifdef _MSC_VER
// MSVC-see http://msdn.microsoft.com/en-us/library/9w1sdazb.aspx
/// WARNING: doesn't work with DLLs ... use boost thread specific pointer instead
/// (http://www.boost.org/libs/thread/doc/tss.html)
#define THREADLOCAL __declspec(thread)
#define GRAEHL_HAVE_THREADLOCAL 1  // TODO: test that it works
#else
#if __APPLE__
// APPLE: __thread even with gcc -pthread isn't allowed:
/* quoting mailing list:

  __thread is not supported. The specification is somewhat ELF-
  specific, which makes it a bit of a challenge.
*/
#define THREADLOCAL
#else
// GCC-see http://gcc.gnu.org/onlinedocs/gcc-4.6.2/gcc/Thread_002dLocal.html
#define THREADLOCAL __thread
#define GRAEHL_HAVE_THREADLOCAL 1
#endif
#endif
#endif

#ifndef GRAEHL_HAVE_THREADLOCAL
#define GRAEHL_HAVE_THREADLOCAL 0
#endif

#endif

namespace graehl {

template <class D, class Old = D>
struct SaveLocal {
  D& value;
  Old old_value;
  SaveLocal(D& val) : value(val), old_value(val) {}
  ~SaveLocal() { value = old_value; }
};

template <class D, class Old = D>
struct SetLocal {
  D& value;
  Old old_value;
  template <class NewValue>
  SetLocal(D& val, NewValue const& new_value) : value(val), old_value(val) {
    value = new_value;
  }
  ~SetLocal() { value = old_value; }
};

template <class D>
struct SaveLocalSwap {
  D& value;
  D old_value;
  SaveLocalSwap(D& val) : value(val), old_value(val) {}
  ~SaveLocalSwap() {
    using namespace std;
    swap(value, old_value);
  }
};

template <class D>
struct SetLocalSwap {
  D& value;
  D old_value;
  template <class NewValue>
  SetLocalSwap(D& val, NewValue const& new_value) : value(val), old_value(new_value) {
    using namespace std;
    swap(value, old_value);
  }
  ~SetLocalSwap() {
    using namespace std;
    swap(value, old_value);
  }
};

template <class Val, class Tag = void>
struct ThreadLocalSingleton {
  static THREADLOCAL Val* val;
  static inline Val& get() {
    if (!val) val = new Val;
    return *val;
  }
};

template <class Val, class Tag>
THREADLOCAL Val* ThreadLocalSingleton<Val, Tag>::val;


}

#endif
