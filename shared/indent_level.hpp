/** \file

    nesting hanging indent (outline style) for logging.
*/

#ifndef GRAEHL__SHARED__INDENT_LEVEL_HPP
#define GRAEHL__SHARED__INDENT_LEVEL_HPP

#include <iostream>

#ifndef DEBUG_GRAEHL_INDENT
#define DEBUG_GRAEHL_INDENT 1
#endif

#if DEBUG_GRAEHL_INDENT
#define GRAEHL_INDENT_DBG_MSG(x) std::cerr << x << '\n'
#else
#define GRAEHL_INDENT_DBG_MSG
#endif

namespace graehl {


struct indent_level {
  char tab;
  char const* bullet;
  int lvl;
  bool operator<=(int maxlvl) const { return lvl <= maxlvl; }
  indent_level(char tab_ = ' ', char const* bullet_ = "") { reset(tab_, bullet_); }
  void reset(char tab_ = ' ', char const* bullet_ = "") {
    lvl = 0;
    tab = tab_;
    bullet = bullet_;
  }
  void in() { ++lvl; }
  void out() { --lvl; }
  void operator++() { in(); }
  void operator--() { out(); }
  // use this newline+indent only if the only way you print is with initial newline (closing document means an
  // explicit nl must follow)
  template <class O>
  O& newline(O& o) {
    o << '\n';
    print(o);
  }
  template <class O>
  void print(O& o) {
    for (int i = 0; i < lvl; ++i) o << tab;
    if (bullet)  // in case of using this in static inits, lack of init => bullet is NULL
      o << bullet;
  }
  template <class C, class T>
  friend std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& o, indent_level& self) {
    self.print(o);
    return o;
  }
};

struct indent {
  int& lvl;
  indent(int& lvlref) : lvl(lvlref) { ++lvl; }
  indent(indent_level& ind) : lvl(ind.lvl) { ++lvl; }
  ~indent() {
    assert(lvl > 0);
    --lvl;
  }
};

// usage e.g.:
/*
  namespace my_ns {
  struct my_indent_class {};
  }
  DEFINE_INDENT(my_ns::my_indent_class);
  void recurse(int i) {
    if (i==0) return;
    typedef my_ns::my_indent_class Indent;
    SCOPED_INDENT_NEST(Indent);
    cout << SCOPED_INDENT(Indent) << i<<endl;
    recurse(i-1);
  }
*/
template <class Tag>
struct static_indent {
  static indent_level indentlvl;
  struct scoped_indent : indent {
    scoped_indent() : indent(indentlvl) { GRAEHL_INDENT_DBG_MSG("nest-indenting -> " << indentlvl.lvl); }
  };
  template <class O>
  void print(O& o) const {
    GRAEHL_INDENT_DBG_MSG("print-indenting -> " << indentlvl.lvl);
    indentlvl.print(o);
  }
  template <class C, class Tr>
  friend std::basic_ostream<C, Tr>& operator<<(std::basic_ostream<C, Tr>& o, scoped_indent const& self) {
    self.print(o);
    return o;
  }
};

template <class Tag>
graehl::indent_level graehl::static_indent<Tag>::indentlvl;

#define SCOPED_INDENT_NEST(T) graehl::static_indent<T> static_indent_line_##__LINE__
#define SCOPED_INDENT(T) graehl::static_indent<T>::indentlvl
#define IF_LIMIT_INDENT(T, maxdepth) if (SCOPED_INDENT(T) <= maxdepth)
#define PRINT_LIMIT_INDENT(T, maxdepth, printerfn, msg) \
  do {                                                  \
    IF_LIMIT_INDENT(T, maxdepth) { printerfn(msg); }    \
  } while (0)

}  // graehl

#endif
