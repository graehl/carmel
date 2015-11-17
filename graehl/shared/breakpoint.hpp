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
#ifndef BREAKPOINT_HPP
#define BREAKPOINT_HPP

#ifndef BREAKPOINT

#if defined(_MSC_VER) && defined(_WIN32)
# define BREAKPOINT __asm int 3
#else
# if defined(__i386__) || defined(__x86_64__)
#  define BREAKPOINT asm("int $3")
# else
#  define BREAKPOINT do { volatile int *p = 0; *p = 0; } while (0)
# endif
#endif

#endif

#ifdef DEBUG
# define DEBUG_BREAKPOINT BREAKPOINT
#else
# define DEBUG_BREAKPOINT
#endif


#endif
