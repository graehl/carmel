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

    C++11: N2982 std::pointer_traits<PointerType>::element_type which lives in <memory>

    until then: boost has us covered
*/

#ifndef POINTER_TRAITS_JG201266_HPP
#define POINTER_TRAITS_JG201266_HPP
#pragma once

//

#include <boost/intrusive/pointer_traits.hpp>

namespace graehl {
using boost::intrusive::pointer_traits;
/*
  // for shared_ptr, intrusive_ptr, and regular pointer

  typedef T            element_type;
  typedef T*           pointer;
  typedef std::ptrdiff_t difference_type;
  typedef T &          reference;

   template <class U> struct rebind_pointer
   {  typedef U* type;  };

   //! <b>Returns</b>: addressof(r)
   //!
   static pointer pointer_to(reference r)
   { return boost::intrusive::detail::addressof(r); }

   //! <b>Returns</b>: static_cast<pointer>(uptr)
   //!
   template<class U>
   static pointer static_cast_from(U *uptr)
   {  return static_cast<pointer>(uptr);  }

   //! <b>Returns</b>: const_cast<pointer>(uptr)
   //!
   template<class U>
   static pointer const_cast_from(U *uptr)
   {  return const_cast<pointer>(uptr);  }

   //! <b>Returns</b>: dynamic_cast<pointer>(uptr)
   //!
   template<class U>
   static pointer dynamic_cast_from(U *uptr)
   {  return dynamic_cast<pointer>(uptr);  }
*/


}

#endif
