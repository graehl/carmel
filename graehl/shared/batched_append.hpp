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
#ifndef GRAEHL__SHARED__batched_append_hpp
#define GRAEHL__SHARED__batched_append_hpp

#include <algorithm> //swap
#include <cstddef>

template <class SRange, class Vector>
void batched_append(Vector &v, SRange const& s) {
  std::size_t news = v.size()+s.size();
  v.reserve(news);
  v.insert(v.end(), s.begin(), s.end());
}

template <class SRange, class Vector>
void batched_append_swap(Vector &v, SRange & s) {
  using namespace std; // to find the right swap
  size_t i = v.size();
  size_t news = i+s.size();
  v.resize(news);
  typename SRange::iterator si = s.begin();
  for (; i<news; ++i, ++si)
    swap(v[i], *si);
}

#endif
