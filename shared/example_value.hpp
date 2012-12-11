#ifndef EXAMPLE_VALUE_2012531_HPP
#define EXAMPLE_VALUE_2012531_HPP

/* default printable placeholder value of a type for documentation purposes, so
   defaults to empty string rather than to_string(default constructed value). */

//TODO: ADL

#include <graehl/shared/int_types.hpp>
#include <graehl/shared/string_to.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

namespace graehl {

template <class T>
std::string example_value(T const&)
{
  return ""; //to_string(T());
}

inline std::string example_value(std::string const&)
{
  return "foo";
}

#define GRAEHL_EXAMPLE_VALUE_FLOAT(Type) inline std::string example_value(Type const&) { return to_string(Type(0.5)); }
#define GRAEHL_EXAMPLE_VALUE_INT(Type) inline std::string example_value(Type const&) { return to_string(Type(65)); }

GRAEHL_FOR_DISTINCT_INT_TYPES(GRAEHL_EXAMPLE_VALUE_INT)
GRAEHL_FOR_DISTINCT_FLOAT_TYPES(GRAEHL_EXAMPLE_VALUE_FLOAT)

template<class T>
std::string example_value()
{
  return example_value(*(T const*)0);
}

template <class T>
std::string example_value(boost::shared_ptr<T> const&)
{
  return example_value(*(T const*)0);
}

template <class T>
std::string example_value(boost::optional<T> const&)
{
  return example_value(*(T const*)0);
}

}

#endif
