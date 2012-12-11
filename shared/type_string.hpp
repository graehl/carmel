#ifndef TYPE_STRING_2012531_HPP
#define TYPE_STRING_2012531_HPP

/* type names for end users. override free function type_string(T) in T's namespace to define T's name

   note: type_string returns empty string if nobody specifies a nice name. type_name will return "unnamed type" instead.

 */

//TODO: ADL

#include <vector>
#include <map>
#include <string>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <graehl/shared/int_types.hpp>
#include <boost/type_traits/is_integral.hpp>

namespace graehl {

namespace {
  std::string unnamed_type="unnamed type";
}

template <class T>
struct type_string_traits
{
  static std::string type_string() { return std::string(); }
};

template <class Int,bool=boost::is_integral<Int>::value>
struct default_type_string
{
  // is_integral=false. you can override the type_string free function, or, in case you want to use some template specialization, override type_string_traits
  static std::string type_string() { return type_string_traits<Int>::type_string(); }
};

template <class Int>
struct default_type_string<Int,true> // note: this is the is_integral=true specialization
{
  static std::string type_string() { return "integer"; }
};

template <class T>
std::string type_string(T const&)
{
  return default_type_string<T>::type_string();
}

template <class T>
bool no_type_string(T const& t)
{
  return type_string(t).empty();
}

template <class T>
std::string type_name(T const& t)
{
  std::string n=type_string(t);
  return n.empty() ? unnamed_type : n;
}


inline std::string type_string(std::string const&) { return "string"; }


#define GRAEHL_PRIMITIVE_TYPE_STRING(T,name) inline std::string type_string(T const&) { return #name; }

GRAEHL_PRIMITIVE_TYPE_STRING(bool,boolean);
GRAEHL_PRIMITIVE_TYPE_STRING(char,character);
GRAEHL_PRIMITIVE_TYPE_STRING(int8_t,signed byte);
GRAEHL_PRIMITIVE_TYPE_STRING(uint8_t,non-negative byte);
GRAEHL_PRIMITIVE_TYPE_STRING(int16_t,16-bit integer);
GRAEHL_PRIMITIVE_TYPE_STRING(uint16_t,non-negative 16-bit integer);
GRAEHL_PRIMITIVE_TYPE_STRING(int32_t,32-bit integer);
GRAEHL_PRIMITIVE_TYPE_STRING(uint32_t,non-negative 32-bit integer);
#if HAVE_64BIT_INT64_T
GRAEHL_PRIMITIVE_TYPE_STRING(int64_t,64-bit integer);
GRAEHL_PRIMITIVE_TYPE_STRING(uint64_t,non-negative 64-bit integer);
#endif
#if PTRDIFF_DIFFERENT_FROM_INTN
GRAEHL_PRIMITIVE_TYPE_STRING(std::size_t,64-bit size);
GRAEHL_PRIMITIVE_TYPE_STRING(std::ptrdiff_t,64-bit offset);
#endif
#if INT_DIFFERENT_FROM_INTN
GRAEHL_PRIMITIVE_TYPE_STRING(int,integer)
GRAEHL_PRIMITIVE_TYPE_STRING(unsigned,non-negative integer)
#endif
#if HAVE_LONGER_LONG
GRAEHL_PRIMITIVE_TYPE_STRING(long,large integer)
GRAEHL_PRIMITIVE_TYPE_STRING(unsigned long,large non-negative integer)
#endif
GRAEHL_PRIMITIVE_TYPE_STRING(float,real number)
GRAEHL_PRIMITIVE_TYPE_STRING(double,double-precision real number)
GRAEHL_PRIMITIVE_TYPE_STRING(long double,long-double-precision real number)

//TODO: this override doesn't get found? use struct instead?
#define GRAEHL_TYPE_STRING_TEMPLATE_1(T,prefix) template <class T1> \
  inline std::string type_string(T<T1> const&) { return std::string(prefix) + type_string(*(T1 const*)0); }

GRAEHL_TYPE_STRING_TEMPLATE_1(std::vector,"sequence of ")
GRAEHL_TYPE_STRING_TEMPLATE_1(boost::optional,"optional ")
GRAEHL_TYPE_STRING_TEMPLATE_1(boost::shared_ptr,"")


/*
template <class T>
std::string type_string(std::vector<T> const&)
{
  return "sequence of "+type_string<T>();
}
*/

template<class T>
std::string type_string()
{
  return type_string(*(T const*)0);
}

template <class T>
std::string type_string(std::map<std::string,T> const&)
{
  return "map to "+type_string<T>();
}

template <class K,class T>
std::string type_string(std::map<K,T> const&)
{
  return "map from "+type_string<K>()+" to "+type_string<T>();
}


}

#endif
