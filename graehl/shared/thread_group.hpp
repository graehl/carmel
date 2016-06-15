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
/** \file

  boost::thread_group else C++11 vector<thread> + add_thread + create_thread +
  join_all(). but no mutex protecting create_thread add_thread join_all -
  i.e. all of those should be called from a single master thread only.
*/

#ifndef THREAD_GROUP_GRAEHL_2015_10_28_HPP
#define THREAD_GROUP_GRAEHL_2015_10_28_HPP
#pragma once
#include <graehl/shared/cpp11.hpp>

#if !GRAEHL_CPP11
#include <boost/thread/thread_group.hpp>
namespace graehl {
using boost::thread_group;
typedef thread_group unsynchronized_thread_group;
}
#else
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace graehl {

struct unsynchronized_thread_group : std::vector<std::thread> {
  unsynchronized_thread_group() = default;
  unsynchronized_thread_group(unsynchronized_thread_group const& o) = delete;
  template <class... Args>
  void create_thread(Args&&... args) {
    this->emplace_back(std::forward<Args>(args)...);
  }
  void add_thread(std::thread&& p) { this->push_back(std::move(p)); }
  void add_thread(std::thread& p) { this->push_back(std::move(p)); }
  void join_all() {
    for (auto& thread : *this)
      if (thread.joinable()) thread.join();
    clear();
  }
};

typedef unsynchronized_thread_group thread_group;


}

#endif
#endif
