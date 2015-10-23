/** \file

 remove soft rlimit(s).
*/

#ifndef UNLIMIT_MEMLOCK_GRAEHL_2015_10_23_HPP
#define UNLIMIT_MEMLOCK_GRAEHL_2015_10_23_HPP
#pragma once


#include <stdexcept>
#include <cstdint>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#if defined(_WIN32) || defined(_WIN64)
#else
#include <sys/resource.h>
#endif

namespace graehl {

inline void throw_errno_for_negative(int ret, std::string const& reason) {
  if (ret < 0)
    throw std::runtime_error((reason+": ")+strerror(errno));
}

/// \return max limit for rlim_type or 0 if no limit
inline std::size_t unlimit(int rlim_type, std::string const& rlim_name) {
#if defined(_WIN32) || defined(_WIN64)
  //TODO?
  return 0;
#else
  struct rlimit rlp;
  int ret;

  throw_errno_for_negative(getrlimit(rlim_type, &rlp), "getrlimit" + rlim_name);

  if (rlp.rlim_cur < rlp.rlim_max) {
    rlp.rlim_cur = rlp.rlim_max;
    throw_errno_for_negative(setrlimit(rlim_type, &rlp), "setrlimit" + rlim_name);
  }
  return rlp.rlim_max;
#endif
}

inline std::size_t unlimit_memory() {
#if defined(_WIN32) || defined(_WIN64)
  return 0;
#else
  return unlimit(RLIMIT_AS, "(RLIMIT_AS)");
#endif
}
inline std::size_t unlimit_memlock() {
#if defined(_WIN32) || defined(_WIN64)
  return 0;
#else
  return unlimit(RLIMIT_MEMLOCK, "(RLIMIT_MEMLOCK)");
#endif
}

}

#endif
