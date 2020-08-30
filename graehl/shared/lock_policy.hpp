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
#ifndef GRAEHL_SHARED__LOCK_POLICY_HPP
#define GRAEHL_SHARED__LOCK_POLICY_HPP
#pragma once
#include <graehl/shared/cpp11.hpp>

#if GRAEHL_CPP11
#include <mutex>
#endif

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

#define GRAEHL_SPIN_LOCKING_ATOMIC 1
#if !GRAEHL_CPP11
#undef GRAEHL_SPIN_LOCKING_ATOMIC
#endif

#if defined(_MSC_VER)
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN 1
#endif
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#include <windows.h>
#include <emmintrin.h> // _mm_pause
#include <synchapi.h> // Sleep
#include <timeapi.h> // timeBeginPeriod
#undef min
#undef max
#else
#include <sched.h> // ssched_yield
#include <time.h> // nanosleep
#endif

#if GRAEHL_SPIN_LOCKING_ATOMIC
#include <atomic>
#else
#include <boost/detail/lightweight_mutex.hpp>
#include <boost/thread/locks.hpp>
#endif
#include <graehl/shared/no_locking.hpp>
#include <cassert>
#include <limits>
#if GRAEHL_CPP11
#include <thread>
#endif

namespace graehl {

#if defined(_MSC_VER)
inline void sleepms(long milliseconds) {
  typedef std::numeric_limits<DWORD> Limit;
  while (milliseconds > Limit::max()) {
    Sleep(Limit::max());
    milliseconds -= Limit::max();
  }
  Sleep((DWORD)milliseconds);
}
inline void sleepns(long nanoseconds) {
  sleepms(nanoseconds / 1000);
}
#else
inline void sleepns(long nanoseconds) {
  timespec r = {0, nanoseconds};
  assert(r.tv_nsec == nanoseconds);
  assert(r.tv_sec == 0);
  nanosleep(&r, nullptr);
}
inline void sleepms(long milliseconds) {
  sleepns(milliseconds * 1000);
}
#endif

inline void sleep_escalating_3() {
  sleepms(1);
}

inline void sleep_escalating_2() {
#if defined(_MSC_VER)
#if GRAEHL_CPP11
  std::this_thread::yield();
#else
  sleepms(0);
#endif
#else
  sched_yield(); // on linux, like sleep(0) - go to end of line
#endif
}

inline void sleep_escalating_1() {
#if defined(_MSC_VER)
  _mm_pause();
#else
  __builtin_ia32_pause(); // __asm__ __volatile__( "rep; nop" :: : "memory" );
#endif
}

struct init_sleep_escalating {
#if defined(_MSC_VER)
  init_sleep_escalating() { timeBeginPeriod(1u); }
#endif
};

struct locking {
#if GRAEHL_CPP11
  typedef std::mutex mutex_type;
  typedef std::lock_guard<mutex_type> guard_type;
  typedef std::unique_lock<mutex_type> unique_lock_type;
#else
  typedef boost::mutex mutex_type;
  typedef boost::lock_guard<mutex_type> guard_type;
  typedef guard_type unique_lock_type;
#endif
};

struct spin_locking {
#if GRAEHL_SPIN_LOCKING_ATOMIC
  class mutex_type {
    std::atomic<bool> lock_ = {0};

   public:
    /// \post you hold lock. may sleep forever unless previous holder calls unlock()
    void lock() noexcept {
   maybe_unlocked:
      if (!lock_.exchange(true, std::memory_order_acquire)) return;
      if (!lock_.load(std::memory_order_relaxed)) goto maybe_unlocked;
      if (!lock_.load(std::memory_order_relaxed)) goto maybe_unlocked;
      if (!lock_.load(std::memory_order_relaxed)) goto maybe_unlocked;
      unsigned k = 0;
      for (; k < 16; ++k)
        if (try_lock())
          return;
        else
          sleep_escalating_1();
      for (; k < 64; ++k)
        if (try_lock())
          return;
        else
          sleep_escalating_2();
      for (;;)
        if (try_lock())
          return;
        else
          sleep_escalating_3();
    }

    /// \return true iff you hold lock. does not wait.
    bool try_lock() noexcept {
      // load before exchange optimize while(!try_lock()) against held lock
      return !lock_.load(std::memory_order_relaxed) && !lock_.exchange(true, std::memory_order_acquire);
    }

    /// \pre you hold lock
    /// \post you don't hold lock
    void unlock() noexcept { lock_.store(false, std::memory_order_release); }
    typedef std::lock_guard<mutex_type> scoped_lock;
  };
#else
  ///
  typedef boost::detail::lightweight_mutex mutex_type;
#endif
  typedef mutex_type::scoped_lock guard_type;
  typedef mutex_type::scoped_lock unique_lock_type;
};


} // namespace graehl

#endif
