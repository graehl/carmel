// Copyright 2014 Jonathan Graehl-http://graehl.org/
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
#ifndef GRAEHL_SHARED__NO_LOCKING_HPP
#define GRAEHL_SHARED__NO_LOCKING_HPP
#pragma once

#include <boost/thread/mutex.hpp>

/*

  intent:

  template <class Locking=graehl::no_locking> // or graehl::locking
  struct collection : private Locking
  {
  void some_operation()
  {
  typename Locking::lock(*this);
// or bool do_lock=...;
//    typename Locking::scoped_lock(*this, do_lock);
// (locks if do_lock)
}
};
*/

namespace graehl {

struct no_locking {
  typedef no_locking self_type;
  typedef no_locking mutex_type;
  struct guard_type {
    guard_type(self_type const& l) {}
    guard_type(self_type const& l, bool b) {}
  };
};


}

#endif
