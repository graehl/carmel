#ifndef KARMA_GENERATE_HPP
#define KARMA_GENERATE_HPP

#include <boost/spirit/karma.hpp>

namespace karma = boost::spirit::karma;

template <class T>
bool tostr(std::string& str, T const& value)
{
  std::back_insert_iterator<std::string> sink(str);
  return karma::generate(sink, value);
}

template <class T>
std::string tostr(T const& value)
{
  string str;
  std::back_insert_iterator<std::string> sink(str);
  karma::generate(sink, value);
  return str;
}


#endif
