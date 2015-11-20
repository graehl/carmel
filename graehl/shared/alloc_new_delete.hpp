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

    boost user allocator concept - new/delete or malloc/free
*/

#ifndef GRAEHL__ALLOC_NEW_DELETE_JG_2014_11_12_HPP
#define GRAEHL__ALLOC_NEW_DELETE_JG_2014_11_12_HPP
#pragma once

#include <cstdlib>
#include <memory>

namespace graehl {

/// unlike the new char[bytes] used by boost::default_user_allocator_new_delete,
/// you can free pointers you got with new T() provided you didn't overload
/// T::operator new to do something incompatible
struct alloc_new_delete {
  static char* malloc(const std::size_t bytes) { return (char*)::operator new(bytes); }
  static inline void free(char* p) { ::operator delete((void*)p); }
};

struct malloc_free {
  static char* malloc(const std::size_t bytes) { return (char*)std::malloc(bytes); }
  static inline void free(char* p) { std::free(p); }
};


}

#endif
