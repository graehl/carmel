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
#ifndef GRAEHL__SHARED__MAYBE_UPDATE_BOUND_HPP
#define GRAEHL__SHARED__MAYBE_UPDATE_BOUND_HPP

namespace graehl {

// see also associative container versions in assoc_container.hpp

template <class To, class From>
inline void maybe_increase_max(To &to, const From &from) {
  if (to<from)
    to = from;
}

template <class To, class From>
inline void maybe_decrease_min(To &to, const From &from) {
  if (from<to)
    to = from;
}

} //graehl


#endif
