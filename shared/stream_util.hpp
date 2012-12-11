#ifndef GRAEHL__SHARED__STREAM_UTIL_HPP
#define GRAEHL__SHARED__STREAM_UTIL_HPP

#include <iomanip>
#include <iostream>
#include <cmath>

namespace graehl {

// faster c++ stream -> OS read/write
inline void unsync_stdio()
{
  std::ios_base::sync_with_stdio(false);
}

// also don't coordinate input/output (for stream processing vs. interactive prompt/response)
inline void unsync_cout()
{
  unsync_stdio();
  std::cin.tie(0);
}

template <class I,class O>
void copy_stream_to(O &o,I&i)
{
  o << i.rdbuf();
}

template <class I>
void rewind_get(I &i)
{
  i.seekg(0,std::ios::beg);
}


}//graehl

#endif
