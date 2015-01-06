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
#ifndef GRAEHL__SHARED__ABS_INT_HPP
#define GRAEHL__SHARED__ABS_INT_HPP

#include <boost/cstdint.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/remove_cv.hpp>

namespace graehl {

template <class I>
inline typename boost::enable_if< typename boost::is_integral<I>
                                  , typename boost::remove_cv<I>::type
                                  >::type
bit_rotate_right(I x)
{
  typedef typename boost::remove_cv<I>::type IT;
  return x<0?-x:x;
}

}


#endif
