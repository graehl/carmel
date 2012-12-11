#ifndef IS_CONTAINER_JG2012614_HPP
#define IS_CONTAINER_JG2012614_HPP

#include <boost/utility/enable_if.hpp>
#include <boost/icl/type_traits/is_container.hpp>

namespace graehl {

using boost::icl::is_container;

template <class T> struct is_nonstring_container : boost::icl::is_container<T> {};

template <class charT,class Traits>
struct is_nonstring_container<std::basic_string<charT,Traits> > { enum {value=0}; };


template <class Val,class Enable=void>
struct print_maybe_container
{
  template <class O>
  void print(O &o,Val const& val,bool bracket=false)
  {
    o<<val;
  }
};

template <class Val>
struct print_maybe_container<Val,typename boost::enable_if<is_nonstring_container<Val> >::type>
{
  template <class O>
  void print(O &o,Val const& val,bool bracket=false)
  {
    bool first=true;
    if (bracket)
      o<<'[';
    for (typename Val::const_iterator i=val.begin(),e=val.end();i!=e;++i) {
      if (!first)
        o<<' ';
      o<<*i;
      first=false;
    }
    if (bracket)
      o<<']';
  }
};

}

#endif // IS_CONTAINER_JG2012614_HPP
