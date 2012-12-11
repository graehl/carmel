#ifndef GRAEHL_SHARED__POOL_TRAITS
#define GRAEHL_SHARED__POOL_TRAITS

#include <boost/pool/object_pool.hpp>
#include <boost/shared_ptr.hpp>
#include <graehl/shared/intrusive_refcount.hpp>

namespace graehl {

// e.g. T=boost::object_pool<O,A> >

template <class T>
struct pool_traits_default
{
  typedef T impl; // only while this lives (noncopyable), its pointers are valid
  typedef typename T::element_type element_type;
  typedef typename T::user_allocator user_allocator;
  typedef typename user_allocator::size_type size_type;
  typedef typename user_allocator::difference_type difference_type;
  enum { can_leak=false }; // if false, when impl is destroyed, everything gets destroyed (true for new_delete_pool)
  enum { has_is_from=false }; // is_from(p)
  typedef element_type *pointer_type; // this may be overriden e.g. refcounted ptr
};

template <class T>
struct pool_traits : public pool_traits_default<T>
{
  enum { has_is_from=true }; // is_from(p)
};

template <class T>
struct pool_destroyer;

template <class T>
struct pool_destroyer
{
  typedef T pool_type;
  T &t;
  explicit pool_destroyer(T &t) : t(t) {}
  typedef typename pool_traits<T>::pointer_type pointer_type;
  void operator()(typename T::pointer_type p) const
  {
    t.destroy(p);
  }
};


// so long as you use pool_traits::pointer_type, this is interface compatible w/ boost object pool except there's no way to force early deallocation or allow cycles - reset those pointers!
// U has only static malloc, free, size_type
template <class T,class U=alloc_new_delete >
struct untracked_pool
{
  typedef T element_type;
  typedef U user_allocator;
  typedef element_type *pointer_type;
  static inline T *malloc() { return (T*)U::malloc(sizeof(T)); }
  static inline void free(T const* p) { U::free((char *)p); }
  static inline bool is_from(T const* p) { return true; } // warning: this is not like a normal pool
  enum { can_leak=true };
  enum { has_is_from=false };
  static inline void destroy(T const *p) {
    p->~T();
    free(p);
  }
#include <graehl/shared/pool_construct.ipp>
};

template <class T,class U>
struct pool_destroyer<untracked_pool<T,U> >
{
  typedef untracked_pool<T,U> pool_type;
  typedef typename T::pointer_type pointer_type;
  void operator()(typename T::pointer_type p) const
  {
    pool_type::destroy(p);
  }
};

template <class T,class U>
struct pool_traits<untracked_pool<T,U> > : public pool_traits_default<T>
{
  enum { has_is_from = false };
};


// E must have graehl::intrusive_traits<E>::user_allocator, intrusive_ptr_add_ref(p) and intrusive_ptr_release(p) - the latter actually freeing using U::free
// e.g. struct my : graehl::intrusive_refcount
template <class T>
// R=unsigned if single-threaded!
struct intrusive_pool
{
  typedef typename intrusive_traits<T>::user_allocator user_allocator;
  typedef T element_type;
  typedef boost::intrusive_ptr<T> pointer_type;
  static inline pointer_type malloc() { return (T*)user_allocator::malloc(sizeof(T)); }
  static inline void free(pointer_type const& p) { } // noop!
  static inline bool is_from(pointer_type const& p) { return true; } // warning: this is not like a normal pool
  enum { can_leak=true };
  enum { has_is_from=false };
  static inline void destroy(pointer_type p) {} //noop!
  //FIXME: can intrusive_ptrs safely destroy early? i.e. do we need to check if count was 0 first and not decrease? if so, implement free/destroy
#include <graehl/shared/pool_construct.ipp>
};

//default traits are good

template <class T>
struct pool_destroyer<untracked_pool<T> >
{
  typedef untracked_pool<T> pool_type;
  typedef typename T::pointer_type pointer_type;
  void operator()(typename T::pointer_type p) const
  {
  }
};


}//ns

#endif
