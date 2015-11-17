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
#ifndef GRAEHL__SHARED__EXACT_CAST_HPP
#define GRAEHL__SHARED__EXACT_CAST_HPP

#include <stdexcept>

namespace graehl {

struct inexact_cast : public std::runtime_error
{
  inexact_cast() : std::runtime_error("inexact_cast - casting to a different type lost information") {}
};

template <class To, class From>
To exact_static_assign(To &to, From const& from)
{
  to = static_cast<To>(from);
  if (static_cast<From>(to)!=from)
    throw inexact_cast();
  return to;
}

template <class To, class From>
To exact_static_cast(From const& from)
{
  To to;
  exact_static_assign(to, from);
  return to;
}


}//graehl

#endif
