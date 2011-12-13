#ifndef GRAEHL__SHARED__INTRUSIVE_REFCOUNT_HPP
#define GRAEHL__SHARED__INTRUSIVE_REFCOUNT_HPP

#include <boost/intrusive_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/detail/atomic_count.hpp>
#include <boost/pool/pool.hpp>
#include <cassert>

/**

   usage:

   struct mine : private boost::instrusive_refcount<mine> {};
   boost::intrusive_ptr<mine> p(new mine());

   this is inserted into boost namespace so that it works with boost::intrusive_ptr


   (a non-virtual dtor requires the CRTP <mine> template)

*/

namespace graehl {
// note: the free functions need to be in boost namespace, OR namespace of involved type. it looks like the friend functions will go into T's namespace, so I put this in graehl instead of boost. for now using decls in boost should maintain backward compat (untested)

template<class T,class U=boost::default_user_allocator_new_delete,class R=unsigned>
struct intrusive_refcount //: boost::noncopyable
{
  typedef U user_allocator;
  typedef T pointed_type;
  friend void intrusive_ptr_add_ref(T const* ptr)
  {
    ++((const intrusive_refcount *)ptr)->refcount;
  }

  friend void intrusive_ptr_release(T const* ptr)
  {
    if (!--((const intrusive_refcount *)ptr)->refcount) {
      ptr->~T();
      U::free((char *)ptr);
    }
  }
protected:
  intrusive_refcount() : refcount(0) {}
  ~intrusive_refcount() { assert(refcount==0); }
private:
  mutable R refcount;
};

//template typedef
template <class T,class U=boost::default_user_allocator_new_delete>
struct atomic_refcount
{
  typedef intrusive_refcount<T,U,boost::detail::atomic_count> type;
};

// on the other hand, this is less typing to use than the equivalent above typedef
template<class T,class U=boost::default_user_allocator_new_delete>
struct atomic_intrusive_refcount //: boost::noncopyable
{
  typedef U user_allocator;
  typedef T pointed_type;
  friend void intrusive_ptr_add_ref(T const* ptr)
  {
    ++((const atomic_intrusive_refcount *)ptr)->refcount;
  }

  friend void intrusive_ptr_release(T const* ptr)
  {
    if (!--((const atomic_intrusive_refcount *)ptr)->refcount) {
      ptr->~T();
      U::free((void *)ptr);
    }
  }
protected:
  atomic_intrusive_refcount(): refcount(0) {}
  ~atomic_intrusive_refcount() { assert(refcount==0); }
private:
  typedef boost::detail::atomic_count R;
  mutable R refcount;
};

template <class T,class U>
void construct()
{
  T *p=(T*)U::malloc(sizeof(T));
  new(&p) T();
  return p;
}

template <class T,class U,class T1>
void construct(T1 const& t1)
{
  T *p=(T*)U::malloc(sizeof(T));
  new(&p) T(t1);
  return p;
}

template <class T,class U,class T1>
void construct(T1 & t1)
{
  T *p=(T*)U::malloc(sizeof(T));
  new(&p) T(t1);
  return p;
}

template <class T>
struct intrusive_traits;

template <class T>
struct intrusive_traits
{
  typedef typename T::user_allocator user_allocator;
  //TODO: thread_safe constant? only true for atomic_count?
};


//etc

}//ns graehl

namespace boost {

using graehl::intrusive_refcount;
using graehl::atomic_intrusive_refcount; // for backward compat - I used to think these had to be declared in boost ns

}//ns

#endif
