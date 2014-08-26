#ifndef GRAEHL_SHARED__LOCK_POLICY_HPP
#define GRAEHL_SHARED__LOCK_POLICY_HPP

#include <boost/thread/mutex.hpp>

/*

  intent:

  template <class Locking=graehl::no_locking> // or graehl::locking
  struct collection : private Locking::mutex_type
  {
  void some_operation()
  {
  typename Locking::guard_type lock(*this);
// or bool do_lock=...;
//    typename Locking::guard_type lock(*this, do_lock);
// (locks if do_lock)
}
};
*/

#include <graehl/shared/no_locking.hpp>
#include <boost/detail/lightweight_mutex.hpp>
#include <boost/thread/locks.hpp>

namespace graehl {

//typedef boost::mutex locking;
//typedef boost::detail::lightweight_mutex spin_locking; ///WARNING: does not support scoped_lock(spin_locking&,bool)

struct locking
{
  typedef boost::mutex mutex_type;
  typedef boost::lock_guard<mutex_type> guard_type;
};

struct spin_locking
{
  typedef boost::detail::lightweight_mutex mutex_type;
//    typedef boost::lock_guard<mutex_type> guard_type;
  typedef mutex_type::scoped_lock guard_type;
};

}


#endif
