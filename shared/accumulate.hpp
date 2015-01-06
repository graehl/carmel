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
#ifndef GRAEHL__SHARED__ACCUMULATE_HPP
#define GRAEHL__SHARED__ACCUMULATE_HPP

#include <boost/integer_traits.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <limits>

namespace graehl {

template <class A> inline
void set_multiply_identity(A &i)
{
  i = 1;
}

template <class A> inline
void set_add_identity(A &i)
{
  i = 0;
}

//FIXME: use boost::integer_traits, std::numeric_limits, boost::is_integral, is_floating_point
template <class A> inline
void set_min_identity(A &i, boost::enable_if<boost::is_integral<A> >* d = 0)
{
  i = boost::integer_traits<A>::max;
}

template <class A> inline
void set_max_identity(A &i, boost::enable_if<boost::is_integral<A> >* d = 0)
{
  i = boost::integer_traits<A>::min;
}

template <class A> inline
void set_min_identity(A &i, boost::enable_if<boost::is_float<A> >* d = 0)
{
  i = std::numeric_limits<A>::max();
}

template <class A> inline
void set_max_identity(A &i, boost::enable_if<boost::is_float<A> >* d = 0)
{
  i=-std::numeric_limits<A>::max();
}

struct accumulate_multiply
{
  template <class A>
  void operator()(A &sum) const
  {
    set_multiply_identity(sum);
  }

  template <class A, class X>
  void operator()(A &sum, X const& x) const
  {
    sum *= x;
  }
};

struct accumulate_sum
{
  template <class A>
  void operator()(A &sum) const
  {
    set_add_identity(sum);
  }

  template <class A, class X>
  void operator()(A &sum, X const& x) const
  {
    sum += x;
  }
};

struct accumulate_max
{
  template <class A>
  void operator()(A &sum) const
  {
    set_max_identity(sum);
  }
  template <class A, class X>
  void operator()(A &sum, X const& x) const
  {
    if (sum<x)
      sum = x;
  }
};

struct accumulate_min
{
  template <class A>
  void operator()(A &sum) const
  {
    set_min_identity(sum);
  }
  template <class A, class X>
  void operator()(A &sum, X const& x) const
  {
    if (x < sum)
      sum = x;
  }
};

}//graehl

#endif
