#ifndef POINTER_TRAITS_JG201266_HPP
#define POINTER_TRAITS_JG201266_HPP

//C++11: N2982 std::pointer_traits<PointerType>::element_type which lives in <memory>
//until then: boost has something

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

#endif // POINTER_TRAITS_JG201266_HPP
