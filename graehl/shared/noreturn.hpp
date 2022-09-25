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

    avoid no-return-value compiler warnings for infinite loops and throw that
    will never return from a fn

    usage: void f() NORETURN

    c++11 alternative: void f() [[noreturn]] didn't work for me

    perhaps you can put NORETURN before or after the decl; after works
*/

#ifndef NORETURN_JG2012613_HPP
#define NORETURN_JG2012613_HPP
#pragma once

#if defined(__GNUC__) && __GNUC__ >= 3 || defined(__clang__)
#define NORETURN __attribute__((noreturn))
#else
#define NORETURN
#endif

#if defined(__clang__)
#define ANALYZER_NORETURN _attribute__((analyzer_noreturn))
#else
#define ANALYZER_NORETURN
#endif

#if defined(_MSC_VER)
#define NORETURNPRE __declspec(noreturn)
#else
#define NORETURNPRE
#endif

#endif
