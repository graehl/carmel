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
#ifndef GRAEHL__SHARED__LZ4_H
#define GRAEHL__SHARED__LZ4_H
#pragma once

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
