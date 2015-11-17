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
#ifndef GRAEHL_SHARED__MUST_EOF_HPP
#define GRAEHL_SHARED__MUST_EOF_HPP

#include <stdexcept>
#include <string>

namespace graehl {

template <class I>
inline void must_eof(I &in, char const* msg="Expected end of input, but got: ")
{
  char c;
  if (in >> c)
    throw std::runtime_error(msg+std::string(1, c));
}

}


#endif
