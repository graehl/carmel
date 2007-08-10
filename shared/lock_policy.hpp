#ifndef GRAEHL_SHARED__LOCK_POLICY_HPP
#define GRAEHL_SHARED__LOCK_POLICY_HPP

#include <boost/thread/mutex.hpp>

/*

intent:

template <class Locking=graehl::no_locking> // or graehl::locking
struct collection : private Locking
{
void some_operation() 
{
    typename Locking::lock(*this);
// or    bool do_lock=...;
//    typename Locking::scoped_lock(*this,do_lock);
    // (locks if do_lock)
}
};
*/

#include <graehl/shared/no_locking.hpp>
#include <boost/detail/lightweight_mutex.hpp>

namespace graehl {

typedef boost::mutex locking;
typedef boost::detail::lightweight_mutex spin_locking; ///WARNING: does not support scoped_lock(spin_locking&,bool)

}


#endif
