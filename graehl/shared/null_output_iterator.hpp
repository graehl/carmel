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
#ifndef GRAEHL_SHARED__NULL_OUTPUT_ITERATOR_HPP
#define GRAEHL_SHARED__NULL_OUTPUT_ITERATOR_HPP

#include <iterator>

namespace graehl {

struct null_output_iterator {
  typedef std::output_iterator_tag iteratory_category;
  typedef void value_type;
  typedef void difference_type;
  typedef void pointer;
  typedef void reference;
  template <class V>
  void operator = (V const& v) const {}
  null_output_iterator const& operator*() const { return *this; }
  void operator++() const {}
  void operator++(int) const {}
};

}


#endif
