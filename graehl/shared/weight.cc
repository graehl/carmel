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
#include <graehl/shared/glibc_memcpy.hpp>
#include <graehl/shared/weight.h>

namespace graehl {

template <class Real>
const int logweight<Real>::base_index = std::ios_base::xalloc();
// xalloc gives a unique global handle with per-ios space handled by the ios
template <class Real>
const int logweight<Real>::thresh_index = std::ios_base::xalloc();
template <class Real>
WEIGHT_THREADLOCAL int logweight<Real>::default_base = logweight<Real>::EXP;
template <class Real>
WEIGHT_THREADLOCAL int logweight<Real>::default_thresh = logweight<Real>::ALWAYS_LOG;
}
