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
//    typename Locking::lock(*this,do_lock);
    // (locks if do_lock)
}
};
*/

#include <graehl/shared/no_locking.hpp>

namespace graehl {
typedef boost::mutex locking;    
}


#endif
