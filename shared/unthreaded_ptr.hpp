/** \file

    An external (non-intrusive) shared_ptr without a custom disposer, that's
    thread-unsafe (don't bother with an atomic_count Ref type since an optimization
    involving a null count object is employed). Optimization for common case that
    there's just 1 owner active (especially notable in C++11 w/ move ctor)

    A similar threaded_ptr could be made - just always create the others_ pointer,
    but a better choice then is shared_ptr (if perf. doesn't matter) or to use
    graehl::intrusive_refcount<T, atomic_count> as a base class for your object and
    boost::intrusive_ptr to that.

    This should be as efficient as boost::scoped_ptr or std::auto_ptr in practice
    (but riskier unless you use safely 'unthreaded': copy/reset are safe only with
    external sync. or in a single thread)

    In C++11 and with #owners never >1, use std::unique_ptr instead and you'll
    gain speed and correctness guarantees.
*/

#ifndef GRAEHL__UNTHREADED_PTR
#define GRAEHL__UNTHREADED_PTR

#include <xmt/graehl/shared/alloc_new_delete.hpp>
#include <cassert>
#include <cstring>

namespace graehl {

struct alloc_new_delete;

/// RefCount must not need its destructor called (try atomic_count or unsigned)
template <class T, class RefCount=unsigned, class AllocT=alloc_new_delete, class AllocRefCount=alloc_new_delete>
struct unthreaded_ptr {
  typedef T value_type;
  typedef T pointed_type;

  unthreaded_ptr()
      : p_()
  {}

#if __cplusplus >= 201103L
  unthreaded_ptr(unthreaded_ptr &&o)
      : p_(o.val)
      , others_(o.others_)
  {
    o.p_ = NULL; // so o.others_ value won't matter
  }
#endif

  explicit unthreaded_ptr(T *val)
      : p_(val)
      , others_()
  {}

  unthreaded_ptr(unthreaded_ptr const& o)
      : p_(o.p_)
      , others_(o.others())
  {
    ++*others_;
  }

  operator T*() const {
    return p_;
  }

  T *get() const { return p_; }

  bool unique() const {
    assert(p_);
    return !others_ || !*others_;
  }

  void reset() {
    if (p_) {
      destroy();
      p_ = NULL;
      others_ = NULL;
    }
  }

  void reset(T *p) {
    if (p_) {
      destroy();
      p_ = p;
      others_ = NULL;
    }
  }

  friend inline void swap(unthreaded_ptr &a, unthreaded_ptr &b) {
    char temp[sizeof(a)];
    std::memcpy(temp, &a, sizeof(a));
    std::memcpy(&a, &b, sizeof(a));
    std::memcpy(&b, temp, sizeof(a));
  }

  void destroy() {
    assert(p_);
    if (!others_) {
   deletep: // for smaller compiled code size and thus faster execution
      p_->~T();
      AllocT::free((char*)p_);
    } else if (!*others_) {
      /// no destructor for RefCount
      AllocRefCount::free(others_);
      goto deletep;
    } else {
      --*others_;
    }
  }

  void destroy_preserve_count() {
    assert(p_);
  }

  ~unthreaded_ptr() {
    if (p_)
      destroy();
  }
 private:
  RefCount *others() const {
    if (!others_)
      others_ = (RefCount*)AllocRefCount::malloc(sizeof(RefCount));
  }

  /// null p_ means others_ may not be read or written (as if it were null but you
  /// MUST NOT read it or valgrind might complain)
  T *p_;
  /// if p_, then null or 0 value means unique
  mutable RefCount *others_;
};

}

#endif
