#ifndef GRAEHL__SHARED__INTRUSIVE_REFCOUNT_HPP
#define GRAEHL__SHARED__INTRUSIVE_REFCOUNT_HPP

#include <boost/intrusive_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/detail/atomic_count.hpp>
#include <cassert>
#include <memory>

/**

   usage:

   struct mine : private boost::instrusive_refcount<mine> {};
   boost::intrusive_ptr<mine> p(new mine());

   this is inserted into boost namespace so that it works with boost::intrusive_ptr


   (a non-virtual dtor requires the CRTP <mine> template)

*/

namespace boost {
// note: the free functions need to be in boost namespace, OR namespace of involved type. this is the only way to do it.

template<class T,class R=unsigned>
struct intrusive_refcount //: boost::noncopyable
{
  typedef T pointed_type;
  friend void intrusive_ptr_add_ref(T const* ptr)
  {
    ++((const intrusive_refcount *)ptr)->refcount;
  }

  friend void intrusive_ptr_release(T const* ptr)
  {
    if (--((const intrusive_refcount *)ptr)->refcount)
      delete ptr;
  }
protected:
  intrusive_refcount(): refcount(0) {}
  ~intrusive_refcount() { assert(refcount==0); }
private:
  mutable R refcount;
};

//template typedef
template <class T>
struct atomic_refcount
{
  typedef intrusive_refcount<T,boost::detail::atomic_count> type;
};

// on the other hand, this is more usable than a template typedef.
template<class T>
struct atomic_intrusive_refcount //: boost::noncopyable
{
  typedef T pointed_type;
  friend void atomic_intrusive_ptr_add_ref(T const* ptr)
  {
    ++((const atomic_intrusive_refcount *)ptr)->refcount;
  }

  friend void atomic_intrusive_ptr_release(T const* ptr)
  {
    if (--((const atomic_intrusive_refcount *)ptr)->refcount)
      delete ptr;
  }
protected:
  atomic_intrusive_refcount(): refcount(0) {}
  ~atomic_intrusive_refcount() { assert(refcount==0); }
private:
  typedef boost::detail::atomic_count R;
  mutable R refcount;
};


struct delete_any
{
  template <class V>
  void operator()(V const* v) const
  {
    delete v;
  }
};

template<class T,class D=delete_any,class R=unsigned>
struct intrusive_refcount_destroy : protected D //: boost::noncopyable
{
  typedef T pointed_type;
  D & destroyer() const { return *((D *)this); }
  intrusive_refcount_destroy(D const& d) : D(d) {}
  intrusive_refcount_destroy() {}
  void set_destroyer(D const& d) { destroyer() = d; }
// typedef intrusive_refcount_destroy<T> pointed_type;
  friend void intrusive_ptr_add_ref(T const* ptr)
  {
    ++((const intrusive_refcount_destroy *)ptr)->refcount;
  }

  friend void intrusive_ptr_release(T const* ptr)
  {
    if (--((const intrusive_refcount_destroy *)ptr)->refcount)
      destroyer()(ptr);
  }
protected:
  intrusive_refcount_destroy(): refcount(0) {}
  ~intrusive_refcount_destroy() { assert(refcount==0); }
private:
  mutable R refcount;
};


}//ns


#endif
