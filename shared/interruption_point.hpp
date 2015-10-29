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

 cooperatively interruptible (but not on thread.sleep join or I/O)
 stoppable_thread. TODO: is there a wait for condition w/ timeout avail to give
 interruptible sleep?
*/

#ifndef INTERRUPTION_POINT_GRAEHL_2015_10_28_HPP
#define INTERRUPTION_POINT_GRAEHL_2015_10_28_HPP
#pragma once

#if __cplusplus < 201103L
#include <boost/thread.hpp>
namespace graehl {
inline void interruption_point() {
  boost::this_thread::interruption_point();
}
}
#else

#include <thread>
#include <stdexcept>
#include <automic>
#include <utility>
namespace graehl {

struct interrupted_exception : std::exception {
  virtual char const* what() const override { return "interrupted"; }
};

struct stoppable_thread {
  friend void check_for_interrupt();

  /// call f(args...) such that any time fun calls interruption_point it will
  /// get an interrupted_exception. recall that std::thread stores args by
  /// value, so if you want otherwise, pass pointers or std::ref, else use
  /// std::bind explicitly
  template <class Function, class... Args>
  stoppable_thread(Function&& f, Args&&... args)
      : thread_([](std::atomic_bool& flag, Function && f, Args && ... args) {
        flag_this_thread_ = &flag;
        f(std::forward<Args>(args)...);
        flag_this_thread = 0;
      }, flag_, std::forward<Function>(f), std::forward<Args>(args)...) {}

  bool stopping() const { return flag_.load(); }

  void stop() { flag_.store(true); }

 private:
  std::thread thread_;
  static thread_local std::atomic_bool* flag_this_thread_ = 0;
  std::atomic_bool flag_ = false;
};

inline void interruption_point() noexcept(false) {
  if (stoppable_thread::flag_this_thread && stoppable_thread::flag_this_thread->load())
    throw interrupted_exception();
}


}

#endif
