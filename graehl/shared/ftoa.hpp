#include <graehl/shared/ftos.hpp>

namespace graehl {

template <class F>
inline char* append_pos_frac(char* p, F f, char decimal = '.') {
  if (GRAEHL_DECIMAL_FOR_WHOLE > 0) *p++ = '0';
  *p++ = decimal;
  return append_pos_frac_digits(p, f);
}

/// '0' right-padded, nul terminated, return position of nul.  [p,ret) are the digits
template <class F>
char* append_pos_frac_digits(char* p, F f) {
  if (f == 0) {
    *p++ = '0';
    return p;
  }
  typedef ftos_traits<F> FT;
  typename FT::uint_t i = FT::rounded_fracblock(f);
  if (i > 0) {
    char* e = p + FT::chars_block;
    utoa_left_pad(p, e, i, '0');
    *e = 0;
    return e;
  } else {
    *p = 0;
    return p;
  }
}


template <class F>
inline char* append_sign(char* p, F f, bool positive_sign = false) {
  if (f < 0) {
    *p++ = '-';
  } else if (positive_sign)
    *p++ = '+';
  return p;
}

template <class F>
inline char* append_frac(char* p, F f, bool positive_sign = false, char decimal = '.') {
  if (f == 0) {
    *p++ = '0';
    return p;
  } else if (f < 0) {
    *p++ = '-';
    return append_pos_frac(p, -f, decimal);
  }
  if (positive_sign) {
    *p++ = '+';
    return append_pos_frac(p, f, decimal);
  }
}

template <class F>
inline char* append_nonsci(char* p, F f, bool positive_sign = false) {
  if (positive_sign && f >= 0) *p++ = '+';
  return p + ftoa_traits<F>::sprintf_nonsci(p, f);
}

template <class F>
inline char* append_sci(char* p, F f, bool positive_sign = false) {
  if (positive_sign && f >= 0) *p++ = '+';
  return p + ftoa_traits<F>::sprintf_sci(p, f);
}

template <class F>
inline char* append_ftoa(char* p, F f, bool positive_sign = false) {
  if (positive_sign && f >= 0) *p++ = '+';
  return p + ftoa_traits<F>::sprintf(p, f);
}

template <class F>
inline std::string ftos_append(F f) {
  typedef ftoa_traits<F> FT;
  char buf[FT::bufsize];
  return std::string(buf, append_ftoa(buf, f));
}

}  // namespace graehl
