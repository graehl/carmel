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
/** \file

    forall and exists ('all') and ('any').
*/


#ifndef GRAEHL__ANY_ALL___HPP
#define GRAEHL__ANY_ALL___HPP

namespace graehl {

/// exists some x in s
template <class Seq, class Pred>
bool any(Seq const& s, Pred const& p) {
  for (typename Seq::const_iterator i = s.begin(), e = s.end(); i != e; ++i)
    if (p(*i)) return true;
  return false;
}

/// for every x in s
template <class Seq, class Pred>
bool all(Seq const& s, Pred const& p) {
  for (typename Seq::const_iterator i = s.begin(), e = s.end(); i != e; ++i)
    if (!p(*i)) return false;
  return true;
}

/// does not exist some x in s
template <class Seq, class Pred>
bool none(Seq const& s, Pred const& p) {
  for (typename Seq::const_iterator i = s.begin(), e = s.end(); i!=e; ++i)
    if (p(*i)) return false;
  return true;
}

}

#endif
