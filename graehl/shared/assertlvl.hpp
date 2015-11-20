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

    a continuum of asserts (finer than all-off-for-release)
*/


#ifndef GRAEHL__SHARED__ASSERTLVL_HPP
#define GRAEHL__SHARED__ASSERTLVL_HPP
#pragma once

#ifndef ASSERT_LEVEL
#define ASSERT_LEVEL 9999
#endif

#define IF_ASSERT(level) if (ASSERT_LEVEL >= level)
#define UNLESS_ASSERT(level) if (ASSERT_LEVEL < level)
#ifndef assertlvl
#include <cassert>
#define assertlvl(level, assertion)         \
  do {                                      \
    IF_ASSERT(level) { assert(assertion); } \
  } while (0)
#endif

#endif
