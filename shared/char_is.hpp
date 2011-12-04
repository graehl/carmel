#ifndef GRAEHL__CHAR_IS_HPP
#define GRAEHL__CHAR_IS_HPP

namespace graehl {

#define MAKE_P(f,arg,ret) struct p ## f  { typedef arg argument_type; typedef ret result_type; inline result_type operator()(argument_type const& a) const { return f(a); } };
#define MAKE_CHARP(f) MAKE_P(f,char,bool)

inline bool isdigit(char c) {
  return c>='0' && c<='9';
}
inline bool isalpha(char c) {
  return c>='A' && c<='Z' || c>='a'&& c<='z';
}
inline bool isblank(char c) {
  return c=='\t' || c==' ';
}
inline bool isspace(char c) {
  return c=='\n' || isblank(c);  // intentionally neglecting \r \v \f
}
MAKE_CHARP(isdigit)
MAKE_CHARP(isalpha)
MAKE_CHARP(isblank)
MAKE_CHARP(isspace)

}//ns


#endif
