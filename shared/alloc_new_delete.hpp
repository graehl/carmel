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
