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
#ifndef STRING_TR_HPP
#define STRING_TR_HPP


namespace graehl {

// [] to {}

template <class O,class S,class F>
void write_tr(O &o,S const& s,F map) {
  for (typename S::const_iterator i=s.begin(),e=s.end();i!=e;++i)
    o<<map(*i);
}

template <class S,class F>
S tr(S const& s,F map) {
  S r(s);
  for (typename S::iterator i=s.begin(),e=s.end();i!=e;++i)
    *i=map(*i);
  return r;
}

}



#endif
