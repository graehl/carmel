/** \file

 .
*/

#ifndef CONFIGURE_TRAITS_GRAEHL_2016_09_12_HPP
#define CONFIGURE_TRAITS_GRAEHL_2016_09_12_HPP
#pragma once

#include <boost/optional.hpp>
#include <graehl/shared/leaf_configurable.hpp>
#include <graehl/shared/type_traits.hpp>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace configure {

using graehl::enable_if;
using graehl::false_type;
using graehl::integral_constant;
using graehl::is_enum;
using graehl::is_fundamental;
using graehl::is_integral;
using graehl::true_type;

template <class Val, class Enable = void>
struct has_leaf_configure : false_type {};

template <class Val>
struct has_leaf_configure<Val, typename Val::leaf_configure> : true_type {};

template <class T>
struct is_string : false_type {};

template <class charT, typename traits, typename Alloc>
struct is_string<std::basic_string<charT, traits, Alloc>> : true_type {};

// leaf_configurable means don't expect a .configure member.
template <class Val, class Enable>
struct leaf_configurable : integral_constant<bool, has_leaf_configure<Val>::value || is_fundamental<Val>::value
                                                     || is_enum<Val>::value || is_string<Val>::value> {};

//"leaf" - use this for all classes that should be configured as leaves, where you
// can't add a member "typedef void leaf_configurable;"

template <class T1>
struct leaf_configurable<boost::optional<T1>, void> : leaf_configurable<T1> {};
template <class T1>
struct leaf_configurable<std::shared_ptr<T1>, void> : leaf_configurable<T1> {};

template <class Val, class Enable = void>
struct scalar_leaf_configurable : leaf_configurable<Val> {};
template <class T1>
struct scalar_leaf_configurable<std::vector<T1>, void> : false_type {};
template <class T1, class T2>
struct scalar_leaf_configurable<std::map<T1, T2>, void> : false_type {};

template <class Val, class Enable = void>
struct map_leaf_configurable : false_type {};
template <class T1, class T2>
struct map_leaf_configurable<std::map<T1, T2>, void> : true_type {
  typedef T1 key_type;
  typedef T2 mapped_type;
  static inline void clear(std::map<T1, T2>* m) { m->clear(); }
  enum { is_pair = false };
};
template <class T1, class T2>
struct map_leaf_configurable<std::pair<T1, T2>, void> : true_type {
  typedef T1 key_type;
  typedef T2 mapped_type;
  static inline void clear(std::pair<T1, T2>*) {}
  enum { is_pair = true };
};
template <class T1>
struct map_leaf_configurable<std::shared_ptr<T1>, void> : map_leaf_configurable<T1> {
  static inline void clear(std::shared_ptr<T1>* m) { m->reset(std::make_shared<T1>()); }
};

template <class Val, class Enable = void>
struct set_leaf_configurable : false_type {};
template <class T1>
struct set_leaf_configurable<std::set<T1>, void> : true_type {};

template <class Val, class Enable = void>
struct sequence_leaf_configurable : false_type {};
template <class T1>
struct sequence_leaf_configurable<std::vector<T1>, void> : true_type {};


template <class X>
bool is_config_leaf(X const&) {
  return leaf_configurable<X>::value;
}

template <class X, class Enable = void>
struct preorder_recurse {
  template <class C>
  static void recurse(X* x, C const& c) {
    c.recurse_configurable(x);
  }
};
template <class X, class C>
void recurse_preorder(X* x, C const& c) {
  preorder_recurse<X>::recurse(x, c);
}
template <class X>
struct preorder_recurse<X, typename enable_if<leaf_configurable<X>::value>::type> {
  template <class C>
  static void recurse(X* x, C const& c) {
    c.recurse_scalar(x);
  }
};
template <class X>
struct preorder_recurse<X, typename enable_if<sequence_leaf_configurable<X>::value>::type> {
  template <class C>
  static void recurse(X* x, C const& c) {
    typedef typename X::value_type Y;
    for (Y& y : *x)
      preorder_recurse<Y>::recurse(&y, c);
  }
};
template <class X>
struct preorder_recurse<X, typename enable_if<map_leaf_configurable<X>::value>::type> {
  template <class C>
  static void recurse(X* x, C const& c) {
    recurse_preorder_map(*x, c);
    //   for (auto& y : *x) preorder_recurse(&y.second, c);
  }
  template <class C>
  static void recurse(std::shared_ptr<X>* x, C const& c) {
    auto& p = x;
    if (p)
      recurse_preorder_map(p.get(), c);
  }
};
template <class X>
struct preorder_recurse<X, typename enable_if<set_leaf_configurable<X>::value>::type> {
  template <class C>
  static void recurse(X* x, C const& c) {
    typedef typename X::value_type Y;
    for (auto const& y : *x)
      preorder_recurse<Y>::recurse(const_cast<Y*>(&y), c);
  }
};

template <class A, class B, class C>
inline void recurse_preorder_map(std::pair<A, B>& pair, C const& c) {
  recurse_preorder(&pair.second, c);
}
template <class M, class C>
inline void recurse_preorder_map(M& m, C const& c) {
  for (auto& y : m)
    recurse_preorder(&y.second, c);
}
template <class M, class C>
inline void recurse_preorder_map(std::shared_ptr<M>& m, C const& c) {
  if (m)
    for (auto& y : *m)
      recurse_preorder(&y.second, c);
}


} // namespace configure

#endif
