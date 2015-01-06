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
#ifndef GRAEHL__SHARED__PREDICATE_COMPOSE_HPP
#define GRAEHL__SHARED__PREDICATE_COMPOSE_HPP


namespace graehl {

/// heavyweight predicate impl. - most STL style containers/fns take functor by value so you can provide a default?  not sure why they don't take const ref instead.
template <class P>
class predicate_ref
{
  P const& ref;
 public:
  typedef bool result_type;
  predicate_ref(P const &cp) : ref(cp) {}
  operator P const&()
  { return ref; }
  template <class Arg>
  bool operator()(Arg const& c) const
  {
    return ref(c);
  }
  template <class Arg>
  bool operator()(Arg & c) const
  {
    return ref(c);
  }
};

/// like using std::binary_compose<std::logical_or,P1,P2>(p1,p2) but works for templated arg types

/// NOTE: public inheritance means you could be stupid and accidentally use this
/// as just a P1 ... BUT advantage: if P1 is adaptable, then so are we.
template <class P1, class P2>
struct or_predicates : public P1
{
  typedef bool result_type;
  P2 pred2;
  or_predicates(P1 const& p1 = P1(), P2 const& p2 = P2()) : P1(p1), pred2(p2) {}
  template <class A>
  bool operator()(A const& a)
  {
    return ((P1&)*this)(a) || pred2(a);
  }
};

template <class P1, class P2>
struct and_predicates : public P1
{
  typedef bool result_type;
  P2 pred2;
  and_predicates(P1 const& p1 = P1(), P2 const& p2 = P2()) : P1(p1), pred2(p2) {}
  template <class A>
  bool operator()(A const& a)
  {
    return ((P1&)*this)(a) && pred2(a);
  }
};

template <class P1>
struct not_predicate : public P1
{
  typedef bool result_type;
  not_predicate(P1 const& p1 = P1()) : P1(p1) {}
  template <class A>
  bool operator()(A const& a)
  {
    return !((P1&)*this)(a);
  }
};


}


#endif
