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
#ifndef GRAEHL__SHARED__RECONSTRUCT_HPP
#define GRAEHL__SHARED__RECONSTRUCT_HPP

#include <new>


namespace graehl
{

template <class pointed_to>
void delete_now(std::auto_ptr<pointed_to> &p) {
  //    std::auto_ptr<pointed_to> take_ownership_and_kill(p);
  delete p.release();
}

struct delete_anything
{
  template <class P>
  void operator()(P *p)
  {
    delete p;
  }
};

template <class V>
void reconstruct(V &v) {
  v.~V();
  new (&v)V();
}

template <class V, class A1>
void reconstruct(V &v, A1 const& a1) {
  v.~V();
  new (&v)V(a1);
}

template <class V, class A1>
void reconstruct(V &v, A1 & a1) {
  v.~V();
  new (&v)V(a1);
}

template <class V, class A1, class A2>
void reconstruct(V &v, A1 const& a1, A2 const& a2) {
  v.~V();
  new (&v)V(a1, a2);
}

template <class V, class A1, class A2>
void reconstruct(V &v, A1 & a1, A2 const& a2) {
  v.~V();
  new (&v)V(a1, a2);
}

template <class V, class A1, class A2>
void reconstruct(V &v, A1 const& a1, A2 & a2) {
  v.~V();
  new (&v)V(a1, a2);
}

template <class V, class A1, class A2>
void reconstruct(V &v, A1 & a1, A2 & a2) {
  v.~V();
  new (&v)V(a1, a2);
}

}


#endif
