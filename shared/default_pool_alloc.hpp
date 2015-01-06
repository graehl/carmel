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
#ifndef DEFAULT_POOL_ALLOC_HPP
#define DEFAULT_POOL_ALLOC_HPP

namespace graehl {

template <class T, bool destroy = true>
struct default_pool_alloc {
  std::vector <T *> allocated;
  typedef T allocated_type;
  default_pool_alloc() {}
  // move semantics:
  default_pool_alloc(default_pool_alloc<T> &other) : allocated(other.allocated) {
    other.allocated.clear();
  }
  default_pool_alloc(const default_pool_alloc<T> &other) : allocated() {
    assert(other.allocated.empty());
  }
  ~default_pool_alloc() {
    deallocate_all();
  }
  void deallocate_all() {
    for (typename std::vector<T*>::const_iterator i = allocated.begin(), e = allocated.end(); i!=e; ++i) {
      T *p=*i;
      if (destroy)
        p->~T();
      ::operator delete((void *)p);
    }
  }
  void deallocate(T *p) const {
    // can only deallocate_all()
  }
  T *allocate() {
    allocated.push_back((T *)::operator new(sizeof(T)));
    return allocated.back();
  }
};

}


#endif
