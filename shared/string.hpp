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
// string: as opposed to a tree.
#ifndef STRING_HPP
#define STRING_HPP

#include <graehl/shared/dynarray.h>

#include <graehl/tt/ttconfig.hpp>
#include <iostream>
#include <graehl/shared/myassert.h>
#include <graehl/shared/genio.h>
//#include <vector>
#include <graehl/shared/dynarray.h>
#include <algorithm>
#include <functional>

#include <graehl/shared/tree.hpp>

namespace graehl {

template <class L, class Alloc=std::allocator<L> > struct String : public array<L,Alloc> {
  typedef L Label;
};

}

#endif
