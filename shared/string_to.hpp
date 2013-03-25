#ifndef GRAEHL__SHARED__STRING_TO_HPP
#define GRAEHL__SHARED__STRING_TO_HPP

/*
  string_to and to_string, which default to boost::lexical_cast, but, unlike that, may be overriden for user types other than supporting default/copy ctor and ostream <<, istream >>.

  note: optional<T> is "none" if absent, else regular string rep for T. this is different from the default optional streaming, which gives empty string for none, and " T" (space followed by T rep)

  USAGE:

  X string_to<X>(string);
  string to_string(X);
  X& string_to(string,X &); // note: returns the same ref you passed in, for convenience of use

  default implementation via boost lexical_cast if GRAEHL_USE_BOOST_LEXICAL_CAST, else stringstreams (quite slow, I'm sure)

  fast implementation for string, int, unsigned, float, double, and, if HAVE_LONGER_LONG=1, long

  also: to_string calls itos utos etc

  to override for your type without going through iostreams, define an ADL-locatable string_to_impl and/or to_string_impl

  ----

  may not be any faster than boost::lexical_cast in later incarnations (see http://accu.org/index.php/journals/1375)
  but is slightly simpler. no wide char or locale. if you use "C" locale, boost::lexical_cast takes advantage?

  see http://www.boost.org/doc/libs/1_47_0/libs/conversion/lexical_cast.htm#faq for benchmarks. so this should be faster still

  see also boost.spirit and qi for template-parser-generator stuff that should be about as fast!

  http://alexott.blogspot.com/2010/01/boostspirit2-vs-atoi.html
  and
  http://stackoverflow.com/questions/6404079/alternative-to-boostlexical-cast/6404331#6404331

*/

#include <iostream>
#include <algorithm>
#include <iterator>
#ifndef DEBUG_GRAEHL_STRING_TO
# define DEBUG_GRAEHL_STRING_TO 1
#endif
#include <utility>
#include <graehl/shared/ifdbg.hpp>

#if DEBUG_GRAEHL_STRING_TO
#include <graehl/shared/show.hpp>
DECLARE_DBG_LEVEL(GRSTRINGTO)
# define GRSTRINGTO(x) x
#else
# define GRSTRINGTO(x)
#endif

#ifndef GRAEHL_USE_FTOA
#define GRAEHL_USE_FTOA 0
#endif
#ifndef HAVE_STRTOUL
# define HAVE_STRTOUL 1
#endif

#ifndef GRAEHL_USE_BOOST_LEXICAL_CAST
# define GRAEHL_USE_BOOST_LEXICAL_CAST 1
#endif

#include <boost/optional.hpp>
#if GRAEHL_USE_BOOST_LEXICAL_CAST
# ifndef BOOST_LEXICAL_CAST_ASSUME_C_LOCALE
#  define BOOST_LEXICAL_CAST_ASSUME_C_LOCALE
# endif
# include <boost/lexical_cast.hpp>
#endif
#include <boost/type_traits/remove_reference.hpp>
#include <limits> //numeric_limits
#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <graehl/shared/nan.hpp>
#include <graehl/shared/have_64_bits.hpp>
#include <graehl/shared/atoi_fast.hpp>
#include <graehl/shared/itoa.hpp>
#include <graehl/shared/is_container.hpp>
#if GRAEHL_USE_FTOA
# include <graehl/shared/ftoa.hpp>
#endif


namespace graehl {

//documentation:

inline std::string const& to_string(std::string const& str) { return str; }

template <class Data>
std::string to_string(Data const& out_str_to_data);

template <class Data,class Str>
Data& string_to(const Str &str,Data &out_str_to_data);

template <class Data,class Str> inline
Data string_to(const Str &str);
}


namespace graehl {


template <class I,class To>
bool try_stream_into(I & i,To &to,bool complete=true)
{
  i >> to;
  if (i.fail()) return false;
  if (complete) {
    char c;
    return !(i >> c);
  }
  return true;
}

template <class Str,class To>
bool try_string_to(Str const& str,To &to,bool complete=true)
{
  std::istringstream i(str);
  return try_stream_into(i,to,complete);
}

inline std::string to_string_impl(unsigned x) {
  return utos(x);
}

inline std::string to_string_impl(int x) {
  return itos(x);
}

#if HAVE_LONGER_LONG
inline void string_to_impl(std::string const& s,int &x) {
  x=strtoi_complete_exact(s.c_str());
}
inline void string_to_impl(char const* s,int &x) {
  x=strtoi_complete_exact(s);
}
#endif

inline void string_to_impl(std::string const& s,long &x) {
  x=strtol_complete(s.c_str());
}
inline void string_to_impl(char const* s,long &x) {
  x=strtol_complete(s);
}

#ifndef XMT_32 // size_t == unsigned int, avoid signature collision
inline void string_to_impl(std::string const& s,unsigned &x) {
  x=atou_fast_complete<unsigned>(s.c_str());
}

inline void string_to_impl(char const* s,unsigned &x) {
  x=atou_fast_complete<unsigned>(s);
}
#endif

inline void string_to_impl(std::string const& s,std::size_t &x) {
  x=atou_fast_complete<std::size_t>(s.c_str());
}
inline void string_to_impl(char const* s,std::size_t &x) {
  x=atou_fast_complete<std::size_t>(s);
}

namespace {
const std::string str_1="1",str_true="true",str_yes="yes",str_on="on";
const std::string str_0="0",str_false="false",str_no="no",str_off="off";
}

// because the int version is favored over the template/stream one:
inline void string_to_impl(std::string const& s,bool &x) {
  if (s==str_1||s==str_true||s==str_yes||s==str_on)
    x=true;
  else if (s==str_0||s==str_false||s==str_no||s==str_off)
    x=false;
  else
    VTHROW_A_MSG(string_to_exception,"'"<<s<<"': not boolean ({true,1,yes,on} or {false,0,no,off}.");
}

inline void string_to_impl(std::string const& s,char &x) {
  if (s.size()!=1)
    VTHROW_A_MSG(string_to_exception,"'"<<s<<"': converting string to character.");
  x=s[0];
}

inline std::string to_string_impl(bool x)
{
  return x ? str_true : str_false;
}

inline std::string to_string_impl(char x)
{
  return std::string(1,x);
}

inline std::string to_string_impl(char const* s)
{
  return std::string(s);
}

#if HAVE_LONGER_LONG
inline void string_to_impl(std::string const& s,unsigned long &x) {
  x=strtoul_complete(s.c_str());
}
inline void string_to_impl(char const* s,unsigned long &x) {
  x=strtoul_complete(s);
}
#endif
//FIXME: end code duplication


/* 9 decimal places needed to avoid rounding error in float->string->float. 17 for double->string->double
   in terms of usable decimal places, there are 6 for float and 15 for double
*/
inline std::string to_string_roundtrip(float x) {
  char buf[17];
  return std::string(buf,buf+sprintf(buf,"%.9g",x));

}
inline std::string to_string_impl(float x) {
#if GRAEHL_USE_FTOA
  return ftos(x);
#else
  char buf[15];
  return std::string(buf,buf+sprintf(buf,"%.7g",x));
#endif
}
inline std::string to_string_roundtrip(double x) {
  char buf[32];
  return std::string(buf,buf+sprintf(buf,"%.17g",x));
}
inline std::string to_string_impl(double x) {
#if GRAEHL_USE_FTOA
  return ftos(x);
#else
  char buf[30];
  return std::string(buf,buf+sprintf(buf,"%.15g",x));
#endif
}

inline void string_to_impl(char const* s,double &x) {
  if (s[0]=='n'&&s[1]=='o'&&s[2]=='n'&&s[3]=='e'&&s[4]==0)
    x=(double)NAN;
  else
    x=std::atof(s);
}
inline void string_to_impl(char const* s,float &x) {
  if (s[0]=='n'&&s[1]=='o'&&s[2]=='n'&&s[3]=='e'&&s[4]==0)
    x=(float)NAN;
  else
    x=(float)std::atof(s);
}
inline void string_to_impl(std::string const& s,double &x) {
  string_to_impl(s.c_str(),x);
}
inline void string_to_impl(std::string const& s,float &x) {
  string_to_impl(s.c_str(),x);
}


template <class Str>
bool try_string_to(Str const& str,Str &to,bool complete=true)
{
  str=to;
  return true;
}

inline std::string const& to_string_impl(std::string const& d)
{
  return d;
}

// ADL participates in closest-match

template <class From,class To>
void string_to_impl(From const& from,To &to)
{
#if GRAEHL_USE_BOOST_LEXICAL_CAST
  to=boost::lexical_cast<To>(from);
#else
  if (!try_string_to(from,to))
    throw std::runtime_error(std::string("Couldn't convert (string_to): ")+from);
#endif
}

template <class To,class From>
To string_to_impl(From const& from)
{
  To r;
  string_to_impl(from,r);
  return r;
}

template <class Val>
std::string optional_to_string(boost::optional<Val> const& opt)
{
  return opt ?  to_string(*opt) : "none";
}

template <class Val>
boost::optional<Val>& string_to_optional(std::string const& str,boost::optional<Val>& opt)
{
  if (str=="none" || str.empty())
    opt.reset();
  else
    opt=string_to<Val>(str);
  //  SHOWIF2(GRSTRINGTO,0,"(free fn) STRING to optional",str,opt);
  return opt;
}

#if 0
template <class Val>
std::string to_string(boost::optional<Val> const& opt)
{
  SHOWIF1(GRSTRINGTO,0,"(free fn) optional to STRING CALLED",opt);
  return opt ?  to_string_impl(*opt) : "none";
}

template <class Val>
boost::optional<Val>& string_to(std::string const& str,boost::optional<Val>& opt)
{
  string_to_optional(str,opt);
  return opt;
}
#endif

#if 0

namespace detail {

template <class To,class From>
struct self_string_to_opt {
  typedef To result_type;
  static inline To string_to(From const& str)
  {
    return string_to_impl<To>(str);
  }
  static inline void string_to(From const& from,To &to)
  {
    string_to_impl(from,to);
  }
};

template <class Self>
struct self_string_to_opt<Self,Self>
{
  typedef Self const& result_type;
  static inline Self const& string_to(Self const& x) { return x; }
  static inline Self & string_to(Self const& x,Self &to) { return to=x; }
};

}
#endif

template <class Str>
void string_to_impl(Str const &s,Str &d)
{
  d=s;
}

template <class D,class Enable=void>
struct to_string_select
{
  static inline std::string to_string(D const &d)
  {
#if GRAEHL_USE_BOOST_LEXICAL_CAST
    return boost::lexical_cast<std::string>(d);
#else
    std::ostringstream o;
    o << d;
    return o.str();
#endif
  }
  template <class Str>
  static inline D& string_to(Str const &s,D &v)
  {
    string_to_impl(s,v);
    return v;
  }
};


template <class From>
std::string to_string_impl(From const &d)
{
  return to_string_select<From>::to_string(d);
}

template <class From>
std::string
to_string(From const &d)
{
  return to_string_impl(d);
}

#if 0
template <class To,class From>
typename detail::self_string_to_opt<
  typename boost::remove_reference<To>::type
 ,typename boost::remove_reference<From>::type
>::result_type
string_to(From const& from)
{
  return
    detail::self_string_to_opt<typename boost::remove_reference<To>::type
                              ,typename boost::remove_reference<From> >
    ::string_to(from);
}
#endif


template <class To,class From>
To & string_to(From const& from,To &to)
{
  to_string_select<To>::string_to(from,to);
  return to;
}

template <class To,class From>
To string_to(From const& from)
{
  To to;
  to_string_select<To>::string_to(from,to);
  return to;
}

template <class V>
struct is_pair
{
  enum { value=0 };
};

template <class A,class B>
struct is_pair<std::pair<A,B> >
{
  enum { value=1 };
};

namespace {
std::string string_to_sep_pair="->";
}

template <class V>
struct to_string_select<V,typename boost::enable_if<is_pair<V> >::type> {
  static inline std::string to_string(V const &p)
  {
    return graehl::to_string(p.first)+string_to_sep_pair+graehl::to_string(p.second);
  }
  template <class Str>
  static inline void string_to(Str const &s,V &v)
  {
    using namespace std;
    string::size_type p=s.find(string_to_sep_pair);
    if (p==string::npos)
      VTHROW_A_MSG(string_to_exception,"'"<<s<<"': pair missing "<<string_to_sep_pair<<" separator");
    string::const_iterator b=s.begin();
    graehl::string_to(std::string(b,b+p),v.first);
    graehl::string_to(std::string(b+p+string_to_sep_pair.size(),s.end()),v.second);
  }
};

template <class V>
struct is_optional
{
  enum { value=0 };
};

template <class V>
struct is_optional<boost::optional<V> >
{
  enum { value=1 };
  //typedef V optional_value_type;
};


template <class V>
struct to_string_select<V,typename boost::enable_if<is_optional<V> >::type> {
  static inline std::string to_string(V const &opt)
  {
    return opt ? graehl::to_string(*opt) : "none";
  }
  template <class Str>
  static inline void string_to(Str const &s,V &v)
  {
    string_to_optional(s,v);
  }
};


typedef std::vector<char> string_buffer;

struct string_builder : string_buffer
{
  string_builder() { this->reserve(80); }
  explicit string_builder(std::string const& str)
      : string_buffer(str.begin(),str.end()) {}
  template <class S>
  string_builder & append(S const& s)
  {
    return (*this)(s);
  }
  template <class S>
  string_builder & append(S const& s1,S const& s2)
  {
    return (*this)(s1,s2);
  }
  string_builder & operator()(char c)
  {
    this->push_back(c);
    return *this; }
  template <class CharIter>
  string_builder & operator()(CharIter i,CharIter end)
  {
    this->insert(this->end(),i,end);
    return *this; }
  string_builder & operator()(std::string const& s)
  {
    (*this)(s.begin(),s.end());
    return *this; }
  string_builder & operator()(char const* s)
  {
    for (;*s;++s)
      this->push_back(*s);
    return *this; }
  template <class T>
  string_builder & operator()(T const& t)
  {
    (*this)(to_string(t));
    return *this;
  }
  string_builder & operator()(std::streambuf &ibuf)
  {
    typedef std::istreambuf_iterator<char> I;
    std::copy(I(&ibuf),I(),std::back_inserter(*this));
    return *this;
  }
  string_builder & operator()(std::istream &i)
  {
    return (*this)(*i.rdbuf());
  }
  std::string str() const
  {
    return std::string(this->begin(),this->end());
  }
  std::string str(std::size_t len) const
  {
    len=std::min(len,this->size());
    return std::string(this->begin(),this->begin()+len);
  }
  std::string shorten(std::size_t drop_suffix_chars)
  {
    std::size_t n=this->size();
    if (drop_suffix_chars>n)
      return std::string();
    return std::string(this->begin(),this->begin()+(n-drop_suffix_chars));
  }

  string_builder & append_space(std::string const& space)
  {
    if (!this->empty())
      operator()(space);
    return *this; }
  string_builder & append_space(char space=' ')
  {
    if (!this->empty())
      operator()(space);
    return *this; }

  string_builder & word(std::string const& t,std::string const& space)
  {
    if (t.empty()) return *this;
    return append_space(space)(t);
  }
  string_builder & word(std::string const& t,char space=' ')
  {
    if (t.empty()) return *this;
    return append_space(space)(t);
  }

};

// function object pointing to string_builder or buffer. cheap copy
struct append_string_builder
{
  string_builder &b;
  append_string_builder(string_builder &b) : b(b) { b.reserve(100); }
  append_string_builder(append_string_builder const& b) : b(b.b) {}
  append_string_builder & operator()(char c)
  {
    b(c); return *this;
  }
  template <class CharIter>
  append_string_builder const& operator()(CharIter const& i,CharIter const& end) const
  {
    b(i,end); return *this;
  }
  template <class T>
  append_string_builder const& operator()(T const& t) const
  {
    b(t); return *this;
  }
  template <class S>
  append_string_builder const& append(S const& s) const
  {
    return (*this)(s);
  }
  template <class S>
  append_string_builder const& append(S const& s1,S const& s2) const
  {
    return (*this)(s1,s2);
  }
  std::string str() const
  {
    return std::string(b.begin(),b.end());
  }
  std::string str(std::size_t len) const
  {
    return b.str(len);
  }
  std::string shorten(std::size_t drop_suffix_chars)
  {
    return b.shorten(drop_suffix_chars);
  }
};

struct append_string_builder_newline : append_string_builder
{
  std::string newline;
  append_string_builder_newline(string_builder &b,std::string const& newline="\n")
    : append_string_builder(b),newline(newline) {}
  append_string_builder_newline(append_string_builder_newline const& o)
    : append_string_builder(o),newline(o.newline) {}
  template <class S>
  append_string_builder_newline const& operator()(S const& s) const
  {
    append_string_builder::operator()(s);
    append_string_builder::operator()(newline);
    return *this;
  }
  template <class S>
  append_string_builder_newline const& operator()(S const& s1,S const& s2) const
  {
    return (*this)(s1,s2);
    append_string_builder::operator()(s1,s2);
    append_string_builder::operator()(newline);
    return *this;
  }
  template <class S>
  append_string_builder_newline const& append(S const& s) const
  {
    return (*this)(s);
  }
  template <class S>
  append_string_builder_newline const& append(S const& s1,S const& s2) const
  {
    return (*this)(s1,s2);
  }
};

template <class V>
struct to_string_select<V,typename boost::enable_if<is_nonstring_container<V> >::type> {
  static inline std::string to_string(V const &val)
  {
    string_builder b;
    b('[');
    bool first=true;
    for (typename V::const_iterator i=val.begin(),e=val.end();i!=e;++i) {
      if (first)
        first=false;
      else
        b(' ');
      b(*i);
    }
    b(']');
    return b.str();
  }
  template <class Str>
  static inline void string_to(Str const &s,V &v)
  {
    throw "string_to for sequences not yet supported";
  }
};


}


#endif
