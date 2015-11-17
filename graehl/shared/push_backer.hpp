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

    output iterator that appends to a collection with push_back.
*/

#ifndef GRAEHL__SHARED__PUSH_BACKER_HPP
#define GRAEHL__SHARED__PUSH_BACKER_HPP
#pragma once

namespace graehl {

template <class Cont>
struct push_backer {
  Cont* cont;
  typedef void result_type;
  typedef typename Cont::value_type argument_type;
  typedef push_backer<Cont> self_type;
  push_backer(self_type const& o) : cont(o.cont) {}
  push_backer(Cont& container) : cont(&container) {}
  template <class V>
  void operator()(V const& v) const {
    cont->push_back(v);
  }
  void operator()() const { cont->push_back(argument_type()); }
};

template <class Cont>
inline push_backer<Cont> make_push_backer(Cont& container) {
  return push_backer<Cont>(container);
}

template <class Output_It>
struct outputter {
  Output_It o;
  typedef void result_type;
  //    typedef typename std::iterator_traits<Output_It>::value_type  argument_type;
  typedef outputter<Output_It> self_type;
  outputter(self_type const& o) : o(o.o) {}
  outputter(Output_It const& o) : o(o) {}
  template <class V>
  void operator()(V const& v) const {
    *o++ = v;
  }
};

template <class Cont>
inline outputter<typename Cont::iterator> make_outputter_cont(Cont& container) {
  return outputter<typename Cont::iterator>(container.begin());
}


}

#endif
