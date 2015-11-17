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
#ifndef GRAEHL_SHARED__PODCPY_HPP
#define GRAEHL_SHARED__PODCPY_HPP

#include <cstring>

namespace graehl {

template <class P> inline
void podset(P& dst, unsigned char c = 0)
{
  std::memset((void*)&dst, c, sizeof(dst));
}

template <class P> inline
void podzero(P& dst)
{
  std::memset((void*)&dst, 0, sizeof(dst));
}

template <class P> inline
P &podcpy(P& dst, P const& src)
{
  std::memcpy((void*)&dst, (void*)&src, sizeof(dst));
}

}


#endif
