#ifndef GRAEHL__SHARED__HEX_INT_HPP
#define GRAEHL__SHARED__HEX_INT_HPP
/*
  wrapped integral types that do iostream with hex format (both in and out) - 0x prefix is mandatory, else read as regular int

  ALSO default init to 0 :)

  (actual octal handling for 0123 - who uses that anyway? maybe a feature not to have it. but consistency with string_into which uses strtoul etc.)
 */

#include <boost/lexical_cast.hpp>
#include <graehl/shared/print_read.hpp>
#include <graehl/shared/string_to.hpp>
#include <graehl/shared/have_64_bits.hpp>
#include <iomanip>

namespace graehl {

template <class I>
struct hex_int {
  typedef typename signed_for_int<I>::unsigned_t U;
  I i;
  hex_int() : i() {}
  explicit hex_int(I i) : i(i) {}
  explicit hex_int(std::string const& s) {string_into(s,i);}
  operator I() const { return i; }
  operator I&() { return i; }

  typedef hex_int<I> self_type;
  TO_OSTREAM_PRINT
  FROM_ISTREAM_READ
  template <class S>
  void print(S &s) const {
    s<<'0'<<'x'<<std::hex<<U(i)<<std::dec;
  }
  template <class S>
  void read(S &s) {
/*    std::string r; //TODO: peek into stream 1 char at a time to detect base using normal istream ops?
    s>>r;
    string_into(r,i.i); //TODO: set istream bad instead of exception, if wrong format? also don't require whitespace at end of number?
*/
    char c;
    if (!s.get(c)) return;
    if (c=='0') {
      i=0;
      if (!s.get(c)) return;
      if (c=='x') {
        U u;
        if (!(s>>std::hex>>u>>std::dec)) return;
        i=u;
      } else {
        s.unget(); // actual number starting with 0. //octal for consistency with string_into.
        if (isdigit(c))
          s>>std::oct>>i;
      }
    } else {
      //regular int, maybe signed
      s.unget();
      s>>i;
    }
  }
};

template <class I>
hex_int<I> hex(I i) {
  return hex_int<I>(i);
}

#define DEFINE_HEX_INT(i) typedef hex_int<i> hex_##i;
#define DEFINE_HEX_INTS(i) DEFINE_HEX_INTS(i) DEFINE_HEX_INTS(u##i)
// these are brought into global namespace by string_into - deal, or use boost::intN_t instead
DEFINE_HEX_INT(int8_t);
DEFINE_HEX_INT(int16_t);
DEFINE_HEX_INT(int32_t);
#if HAVE_64_BITS
DEFINE_HEX_INT(int64_t);
#endif

#if 0 // redundant with default string_into
template <class S,class I>
void string_into(S const& s,hex_int<I> &i) {
  std::istringstream in(s);
  in>>i;
  return i;
}
#endif

//only have to int and long with hex using string_into - e.g. int64_t - check strtoull existing?
#define HEX_USE_STRING_TO(unsigned) \
template <class S> \
void string_into(S const& s,hex_int<unsigned> &i) { \
  string_into(s,i.i); \
}
#if 0
HEX_USE_STRING_TO(unsigned)
HEX_USE_STRING_TO(int)
#if HAVE_LONGER_LONG
HEX_USE_STRING_TO(long unsigned)
HEX_USE_STRING_TO(long int)
#endif
#endif

}//ns

namespace boost {
// for program_options - not sure if ADL would have found since it's on return type not args
template <class I>
graehl::hex_int<I> lexical_cast(std::string const &s) {
  graehl::hex_int<I> ret;
  string_into(s,ret);
  return ret;
}
}

#endif
