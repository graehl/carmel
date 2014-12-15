/** \file

    shared_ptr helper for nondeleting references (where the refcount is
    meaningless, but you pay for it anyway to simplify - everything can be a
    shared_ptr)
*/

#ifndef GRAEHL__SHARED__NULL_DELETER_HPP
#define GRAEHL__SHARED__NULL_DELETER_HPP

#include <boost/shared_ptr.hpp>

namespace graehl {

struct null_deleter {
  void operator()(void const*) const {}
};

template <class V>
boost::shared_ptr<V> no_delete(V &v) {
  return boost::shared_ptr<V>(&v, null_deleter());
}

template <class V>
boost::shared_ptr<V> no_delete(V *v) {
  return boost::shared_ptr<V>(v, null_deleter());
}


}

#endif
