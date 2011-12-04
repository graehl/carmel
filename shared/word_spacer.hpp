#ifndef GRAEHL__SHARED__WORD_SPACER_HPP
#define GRAEHL__SHARED__WORD_SPACER_HPP

#include <sstream>
#include <string>
#include <graehl/shared/print_read.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

namespace graehl {
/// print spaces between each word (none before or after)
///
///usage: word_spacer sp; while(...) o << sp << ...;
///
/// alternative: word_spacer_sp; while(...) o << get_string() << ...;
struct word_spacer {
  bool first;
  char space_string[2];
  word_spacer(char space=' ') : first(true) { space_string[0]=space;space_string[1]=0;}
  const char *empty() const
  {
    return space_string+1;
  }
  const char *space() const
  {
    return space_string;
  }
  const char *get_string()
  {
    if (first) {
      first=false;
      return empty();
    } else {
      return space();
    }
  }
  void reset()
  {
    first=true;
  }
  template <class C, class T>
  void print(std::basic_ostream<C,T>& o)
  {
    if (first)
      first=false;
    else
      o << space_string[0];
  }

  template <class C,class T>
  friend inline std::basic_ostream<C,T>& operator<<(std::basic_ostream<C,T>& o,word_spacer &me)
  {
    me.print(o);
    return o;
  }
};

inline std::string space_sep_words(const std::string &sentence,char space=' ')
{
  std::stringstream o;
  std::string word;
  std::istringstream i(sentence);
  word_spacer sep(space);
  while (i >> word) {
    o << sep;
    o << word;
  }
  return o.str();
}

inline int compare_space_normalized(const std::string &a, const std::string &b)
{
  return space_sep_words(a).compare(space_sep_words(b));
}


//!< print before word.
template <char sep=' '>
struct word_spacer_c {
  bool first;
  word_spacer_c() : first(true) {}
  void reset()
  {
    first=true;
  }
  template <class C, class T>
  void print(std::basic_ostream<C,T>& o)
  {
    if (first)
      first=false;
    else
      o << sep;
  }
  typedef word_spacer_c<sep> Self;
  static const char seperator=sep;

  template <class C, class T>
  friend inline std::basic_ostream<C,T>&
  operator<<(std::basic_ostream<C,T>& o,Self &me)
  {
    me.print(o);
    return o;
  }
};

template <char sep=' '>
struct spacesep {
  bool squelch;
  spacesep() : squelch(true) {}
  template <class O>
  void print(O& o)
  {
    if (squelch)
      squelch=false;
    else
      o << sep;
  }
  template <class C, class T>
  friend inline std::basic_ostream<C,T>&
  operator<<(std::basic_ostream<C,T>& o,spacesep<sep> &me)
  {
    me.print(o);
    return o;
  }
};

struct sep {
  char const* s;
  bool squelch;
  sep(char const* s=" ") : s(s),squelch(true) {  }
  operator char const* ()  {
    if (squelch) {
      squelch=false;
      return "";
    } else
      return s;
  }
};

struct singlelineT {};
struct multilineT {};
static singlelineT singleline;
static multilineT multiline;

struct range_sep {
  typedef char const* S;
  S space,pre,post;
  bool always_pre_space;
  range_sep(S space=" ",S pre="[",S post="]",bool always_pre_space=false) : space(space),pre(pre),post(post),always_pre_space(always_pre_space) {}
  range_sep(multilineT) : space("\n "),pre("{"),post("\n}"),always_pre_space(true) {}
  range_sep(singlelineT) : space(" "),pre("["),post("]"),always_pre_space() {}
  template <class O,class I>
  void print(O &o,I i,I const& end) const {
    o<<pre;
    if (always_pre_space)
      for (;i!=end;++i)
        o<<space<<*i;
    else {
      sep s(space);
      for (;i!=end;++i)
        o<<s<<*i;
    }
    o<<post;
  }
  template <class O,class I>
  void print(O &o,I const& i) const {
    print(o,boost::begin(i),boost::end(i));
  }
};

// see print_read.hpp: o << print(v,r) calls this
template <class O,class V>
inline void print(O &o,V const& v,range_sep const& r) {
  r.print(o,v);
}

}//ns

#endif
