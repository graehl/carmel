/**
   print \0a instead of newline and other nonprintable ascii chars and \.
*/

#ifndef GRAEHL_SHARED__ESCAPE3
#define GRAEHL_SHARED__ESCAPE3

#include <cassert>
#include <string>

namespace graehl {

static char const* const k_hex = "0123456789ABCDEF";

inline bool plaintext_escape3(char c) {
  return c >= 32 && c < 127 && c != '\\';
}
inline void append_escape3(char *&out, unsigned char c) {
  if (plaintext_escape3(c))
    *out++ = c;
  else {
    *out++ = '\\';
    *out++ = k_hex[c >> 4];
    *out++ = k_hex[c & 0xf];
  }
}
inline std::size_t reserve_escape3(std::size_t len) {
  return len * 3;
}

/// return true if truncated (can print ... or something). append to empty s.
template <class String>
inline bool escape3(char const* i, std::size_t n, String &s, std::size_t maxlen = 0) {
  assert(s.empty());
  if (!maxlen || maxlen > n)
    maxlen = n;
  if (!maxlen)
    return false;
  s.resize(reserve_escape3(maxlen));
  char *out = &s[0];
  char *obegin = out;
  char const* end = i + maxlen;
  for (; i < end; ++i)
    append_escape3(out, *i);
  s.resize(out - obegin);
  return maxlen < n;
}

template <class String>
inline bool escape3(void const* i, std::size_t n, String &s, std::size_t maxlen = 0) {
  return escape3((char const*)i, n, s, maxlen);
}

struct Escape3 : std::string {
  Escape3(void const* i, std::size_t n, std::size_t maxlen = 0) {
    if (escape3(i, n, *this, maxlen)) {
      push_back('.');
      push_back('.');
      push_back('.');
    }
  }
};

}

#endif
