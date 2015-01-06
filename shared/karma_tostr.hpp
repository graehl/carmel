// Copyright 2014 Jonathan Graehl - http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
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
