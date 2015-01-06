// Copyright 2014 Jonathan Graehl - http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef GRAEHL_SHARED__LOCK_POLICY_HPP
#define GRAEHL_SHARED__LOCK_POLICY_HPP
#pragma once

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

// typedef boost::mutex locking;
// typedef boost::detail::lightweight_mutex spin_locking; ///WARNING: does not support
// scoped_lock(spin_locking&,bool)

struct locking {
  typedef boost::mutex mutex_type;
  typedef boost::lock_guard<mutex_type> guard_type;
};

struct spin_locking {
  typedef boost::detail::lightweight_mutex mutex_type;
  typedef mutex_type::scoped_lock guard_type;
};


}

#endif
