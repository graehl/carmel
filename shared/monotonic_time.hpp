/** \file

 .
*/

#ifndef MONOTONIC_CLOCK_JG_2015_04_27_HPP
#define MONOTONIC_CLOCK_JG_2015_04_27_HPP
#pragma once

#include <time.h>
#include <sys/time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

namespace graehl {

/// \return .tv_sec .tv_nsec
inline struct timespec monotonic_timespec() {
  struct timespec ts;
#ifdef __MACH__  // OS X does not have clock_gettime, use clock_get_time
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts.tv_sec = mts.tv_sec;
  ts.tv_nsec = mts.tv_nsec;
#else
  clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
  return ts;
}

inline double seconds_with_ns(double sec, double nsec) {
  return sec + nsec * 1e-9;
}

inline double seconds_with_ns(struct timespec ts) {
  return seconds_with_ns(ts.tv_sec, ts.tv_nsec);
}

inline double monotonic_time() {
  return(seconds_with_ns(monotonic_timespec()));
}

}

#endif
