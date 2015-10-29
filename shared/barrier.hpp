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
/** \file

 boost::barrier else C++11.

 slightly simplified from boost barrier by David Moore, William E. Kempf,
 Anthony Williams
*/

#ifndef BARRIER_GRAEHL_2015_10_28_HPP
#define BARRIER_GRAEHL_2015_10_28_HPP
#pragma once

#if __cplusplus < 201103L
#include <boost/thread/barrier.hpp>
namespace graehl {
using boost::barrier;
}
#else
#include <cassert>
#include <mutex>
#include <condition_variable>

namespace graehl {

struct barrier {
  barrier(barrier const& o) = delete;

  /// require = # of calls to wait() that must occur before any of those calls return
  barrier(unsigned require) : require_(require), next_gen_require_(require), generation_(0) {
    assert(require);
  }

  /// wait until 'require' calls to wait occur and return 'true' for the last of
  /// them (this is helpful if you want only one thread to execute something
  /// when all reach, and you don't care which thread does it). the barrier
  /// 'require' value is reset to its initial so the barrier can be reused.
  bool wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    unsigned const g = generation_;
    if (--require_ == 0) {
      ++generation_;
      require_ = next_gen_require_;
      condition_.notify_all();
      return true;
    }
    while (g == generation_) condition_.wait(lock);
    return false;
  }

  /// \return 0 until the first wait() barrier is breached, then 1, ...
  unsigned generation() const { return generation_; }

 private:
  std::mutex mutex_;
  std::condition_variable condition_;
  unsigned require_;
  unsigned next_gen_require_;
  unsigned generation_;
};


}

#endif

#endif
