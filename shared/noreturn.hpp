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

    avoid no-return-value compiler warnings for infinite loops and throw that
    will never return from a fn
*/

#ifndef NORETURN_JG2012613_HPP
#define NORETURN_JG2012613_HPP
#pragma once

#if defined(__GNUC__) && __GNUC__>=3
# define NORETURN __attribute__ ((noreturn))
#elif defined(__clang__)
# define ANALYZER_NORETURN _attribute__((analyzer_noreturn))
# define NORETURN
#else
# define NORETURN
#endif

#ifndef ANALYZER_NORETURN
# define ANALYZER_NORETURN
#endif

#endif
