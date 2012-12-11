#ifndef GRAEHL__SHARED__INTRUSIVE_REFCOUNT_HPP
#define GRAEHL__SHARED__INTRUSIVE_REFCOUNT_HPP

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/detail/atomic_count.hpp>
#include <boost/pool/pool.hpp>
#include <boost/utility/enable_if.hpp>
#include <cassert>
#include <memory>

/**

   usage:

   namespace my {

     struct mine : private boost::instrusive_refcount<mine> {
       typedef mine self_type;
       friend void intrusive_ptr_add_ref(self_type *p) { p->add_ref();  }
       friend void intrusive_ptr_release(self_type *p) { p->release(p); }
     };

          //   (a non-virtual dtor requires the CRTP <mine> template)

     boost::intrusive_ptr<mine> p(new mine());
   }

*/

namespace graehl {

using boost::detail::atomic_count;


// note: the free functions need to be in boost namespace, OR namespace of involved type. it looks like the friend functions will go into T's namespace, so I put this in graehl instead of boost. for now using decls in boost should maintain backward compat (untested)

// unlike the new char[bytes] using boost::default_user_allocator_new_delete, you can free pointers you got with new T() provided you didn't overload new for type T.
struct alloc_new_delete
{
  static char* malloc (const std::size_t bytes)
  {
    return (char *)::operator new(bytes);
  }
  static inline void free(char * p)
  {
    ::operator delete((void*)p);
  }
};


//or U=boost::default_user_allocator_new_delete - but make sure to construct objects in space provided by U::malloc()
template<class T,class R=atomic_count,class U=alloc_new_delete>
struct intrusive_refcount //: boost::noncopyable
{
  typedef void is_refcounted_enable; // for is_refcounted
  typedef T intrusive_type;
  typedef U user_allocator;
  typedef T pointed_type;

  //for copy-on-write - may not give the thread-safety you expect, though (but if it's update+copy at same time, there's a race no matter what)
  bool unique() const
  {
    return refcount<=1;
  }

  // please don't call these directly - only let boost::intrusive_ptr<T> do it:
  inline void add_ref() const
  {
    ++refcount;
  }
  inline void release(T const* tc) const
  {
    T * t=const_cast<T*>(tc);
    if (!--refcount) {
      t->~T();
      U::free((char *)t);
    }
  }
  inline void release() const
  {
    release(derivedPtr());
  }

  typedef T self_type;


  template <class A0>
  static T *construct(A0 const& a0)
  {
    T *r=(T *)U::malloc(sizeof(T));
    return new(&r) T(a0);
  }

  inline T const* derivedPtr() const
  {
    return static_cast<T const*>(this);
  }

  // formerly had a trick to put boost::intrusive_ptr_release etc in the namespace of T by friend, but this proved non-portable to different compilers. had to add to namespace boost.

  intrusive_refcount() : refcount(0) {}
  intrusive_refcount(intrusive_refcount const& o) : refcount(0) {} // intentionally weird semantics so default T copy ctor does the right thing
  ~intrusive_refcount() { assert(refcount==0); }
private:
  mutable R refcount;
};


template <typename UserAlloc,typename element_type>
element_type * construct()
{

  element_type * const ret = (element_type *)UserAlloc::malloc(sizeof(element_type));
  if (!ret)
    return ret;
  try { new (ret) element_type(); }
  catch (...) { UserAlloc::free((char *)ret); throw; }
  return ret;
}

template <typename UserAlloc,typename element_type>
element_type * construct_copy(element_type const& copy_me)
{
  element_type * const ret = (element_type *)UserAlloc::malloc(sizeof(element_type));
  if (!ret)
    return ret;
  try { new (ret) element_type(copy_me); }
  catch (...) { UserAlloc::free((char *)ret); throw; }
  return ret;
}

template <class T>
struct intrusive_traits;

template <class T>
struct intrusive_traits
{
  typedef typename T::user_allocator user_allocator;
  //TODO: thread_safe constant? only true for atomic_count?
};


/// this trait means you have add_ref() and release(p) members and is used to prevent making a refcount around a refcount
template <class T,class Enable=void>
struct is_refcounted
{
  enum {value=0};
};

template <class T>
struct is_refcounted<T,typename T::is_refcounted_enable>
{
  enum {value=1};
};

}//ns graehl

namespace boost {
template <class T>
inline typename boost::enable_if<graehl::is_refcounted<T> >::type intrusive_ptr_add_ref(T const*p)
{
  p->add_ref();
}

template <class T>
inline typename boost::enable_if<graehl::is_refcounted<T> >::type intrusive_ptr_release(T const*p)
{
  p->release(p);
}
}//ns boost

// clang requires we declare the above before using intrusive_ptr ...


#include <boost/intrusive_ptr.hpp>

namespace graehl {

template <class T,class Enable=void>
struct shared_ptr_maybe_intrusive
{
  typedef boost::shared_ptr<T> type;
};

template <class T>
struct shared_ptr_maybe_intrusive<T,typename boost::enable_if<is_refcounted<T> >::type>
{
  typedef boost::intrusive_ptr<T> type;
};


template <class T>
//typename boost::enable_if<is_refcountend<T>,T>::type
inline T *intrusive_clone(T const& x) // result has refcount of 0 - must delete yourself or use to build an intrusive_ptr
{
  return construct_copy<typename intrusive_traits<T>::user_allocator,T>(x);
}

template <class T>
// enable_if ...
typename boost::intrusive_ptr<T> intrusive_copy_on_write(boost::intrusive_ptr<T> const& p)
{
  assert(p);
  return p->unique() ? p : intrusive_clone(*p);
}

template <class T>
//enable_if ...
void intrusive_make_unique(boost::intrusive_ptr<T> & p)
{
  assert(p);
  if (!p->unique())
    p.reset(intrusive_clone(*p));
}

template <class T>
//typename boost::enable_if<typename intrusive_traits<T>::user_allocator>::type
void
intrusive_make_valid_unique(boost::intrusive_ptr<T> & p)
{
  if (!p)
    p.reset(construct<typename intrusive_traits<T>::user_allocator,T>());
  else if (!p->unique())
    p.reset(intrusive_clone(*p));
}


template<class T>
struct intrusive_deleter
{
  void operator()(T * p)
  {
    if(p) intrusive_ptr_release(p);
  }
};

template <class T>
boost::shared_ptr<T> shared_from_intrusive(T * p)
{
  if(p) intrusive_ptr_add_ref(p);
  return boost::shared_ptr<T>(p, intrusive_deleter<T>());
}

}

#endif
