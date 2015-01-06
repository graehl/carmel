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

    force inlining on or off:

    ALWAYS_INLINE void f() { g(); }
    NEVER_INLINE void f2() { g(); }
*/

#ifndef INLINE_JG_2014_11_12_HPP
#define INLINE_JG_2014_11_12_HPP
#pragma once

#ifndef ALWAYS_INLINE
#if defined(__GNUC__) || defined(__clang__)
#define ALWAYS_INLINE inline __attribute__((__always_inline__))
#elif defined(_MSC_VER)
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE inline
#endif
#endif

#ifndef NEVER_INLINE
#if defined(__GNUC__) || defined(__clang__)
#define NEVER_INLINE __attribute__((__noinline__))
#elif defined(_MSC_VER)
#define NEVER_INLINE __declspec(noinline)
#else
#define NEVER_INLINE
#endif
#endif

#endif
