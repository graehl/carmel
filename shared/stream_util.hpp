/** \file

    some high performance quirky tricks for C++ iostreams.
*/

#ifndef GRAEHL__SHARED__STREAM_UTIL_HPP
#define GRAEHL__SHARED__STREAM_UTIL_HPP
#pragma once

#ifndef GRAEHL_BUFFER_STDOUT_NON_TTY
#define GRAEHL_BUFFER_STDOUT_NON_TTY 1
#endif

#if __linux__ && GRAEHL_BUFFER_STDOUT_NON_TTY
#define GRAEHL_DETECT_LINUX_STDOUT_TTY 0
#else
#define GRAEHL_DETECT_LINUX_STDOUT_TTY 0
#undef GRAEHL_BUFFER_STDOUT_NON_TTY
#endif

#if GRAEHL_DETECT_LINUX_STDOUT_TTY
#include <unistd.h>
#endif

#if GRAEHL_BUFFER_STDOUT_NON_TTY
#include <graehl/shared/large_streambuf.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include <iomanip>
#include <iostream>
#include <cmath>

namespace graehl {

inline bool stdout_is_tty() {
#if GRAEHL_DETECT_LINUX_STDOUT_TTY
  return true;
#else
  return true;  // conservative: let users see output to STDOUT (-) relatively quickly
#endif
}

/// faster c++ stream -> OS read/write
inline void unsync_stdio() {
  std::ios_base::sync_with_stdio(false);
}

/// don't coordinate input/output (for stream processing vs. interactive prompt/response)
inline void unsync_cout() {
  unsync_stdio();
  std::cin.tie(0);
}

/**
   call before anyone starts using cout, because this increases the buffer size.

   unfortunately, on windows, cin and cout will still be in text mode, which has
   a performance cost. therefore, prefer filename args over redirection on
   windows for large input/output

   for unix, there's no overhead as binary/text are the same ('\n' is one byte,
   untranslated)

   this object should last as long as uses of cout
*/
struct use_fast_cout {
#if GRAEHL_BUFFER_STDOUT_NON_TTY
  boost::scoped_ptr<bigger_streambuf> pbuf;
#endif
  use_fast_cout(std::size_t bufsize = 256 * 1024, bool no_buffer_for_terminal = true) {
    unsync_stdio();
    unsync_cout();
#if GRAEHL_BUFFER_STDOUT_NON_TTY
    if (no_buffer_for_terminal && stdout_is_tty()) return;
    pbuf.reset(new bigger_streambuf(bufsize, std::cout, true));
#endif
  }
};

template <class I, class O>
void copy_stream_to(O& o, I& i) {
  o << i.rdbuf();
}

template <class I>
void rewind_get(I& i) {
  i.seekg(0, std::ios::beg);
}


}  // graehl

#endif
