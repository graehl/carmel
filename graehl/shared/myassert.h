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
//workaround for difficulties I've had getting a debugger breakpoint from a failed assert
#ifndef GRAEHL_SHARED__ASSERT_H
#define GRAEHL_SHARED__ASSERT_H


#include <graehl/shared/breakpoint.hpp>
#include <graehl/shared/config.h>

#ifdef DEBUG

#define GRAEHL_ASSERT 1
#endif
#ifndef GRAEHL_ASSERT
# define GRAEHL_ASSERT 0
#endif

#ifndef DEBUG
# ifndef NDEBUG
//# define NDEBUG
#endif
#endif

#include <cassert>


inline static void _my_assert(const char *file, unsigned line, const char *expr)
{
  Config::err() << file << "(" << line << ") Assertion failed: " << expr << std::endl;
  BREAKPOINT;
}

template <class T>
inline static void _my_assert(const char *file, unsigned line, const T&t, const char *expr, const char *expect)
{
  Config::err() << file << "(" << line << ") Assertion failed: (" << expr << ") was " << t << "; expected " << expect << std::endl;
  BREAKPOINT;
}

#if GRAEHL_ASSERT
# define GRAEHL_IF_ASSERT(x) x
# define Assert(expr) (expr) ? (void)0 :        \
  _my_assert(__FILE__, __LINE__, #expr)
// WARNING: expr occurs twice (repeated computation)
# define Assert2(expr, expect) do {                                     \
    /* Config::log() << #expr << ' ' << #expect << " = " << (expr expect) << std::endl;*/ \
    if (!((expr) expect)) _my_assert(__FILE__, __LINE__, expr, #expr, #expect); \
  } while (0)
# define Paranoid(a) do { a; } while (0)
#else
# define GRAEHL_IF_ASSERT(x)
# define Assert(a)
# define Assert2(a, b)
# define Paranoid(a)
#endif

#endif
