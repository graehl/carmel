/** \file

 you'll need to override:

  template <class Child>
  is_expr<Child> operator()(std::string const& name, Child* child) const {
    return is_expr<Child>(child, 0);
  }

*/

#ifndef CONFIGURE_NOOP_GRAEHL_2016_09_12_HPP
#define CONFIGURE_NOOP_GRAEHL_2016_09_12_HPP
#pragma once

#include <string>
#include <vector>

namespace configure {

struct noop {
  template <class X>
  void sibling(X* x) const {
    x->configure(*this);
  }
  template <class X, class A1>
  void sibling(X* x, A1 const& a1) const {
    x->configure(*this, a1);
  }
  template <class X, class A1, class A2>
  void sibling(X* x, A1 const& a1, A2 const& a2) const {
    x->configure(*this, a1, a2);
  }
  template <class X, class A1, class A2, class A3>
  void sibling(X* x, A1 const& a1, A2 const& a2, A3 const& a3) const {
    x->configure(*this, a1, a2, a3);
  }
  template <class X>
  noop const& operator()(std::string const&, X*) const {
    return *this;
  }

  noop const& is(std::string const& is) const { return *this; }
  noop const& is_also(std::string const& is) const { return *this; }
  noop const& operator()(std::string const& usage) const { return *this; }
  noop const& operator()(std::vector<char> const& usage) const { return *this; }

  /** disabled to help you not forget to override:
  template <class Child>
  noop<Child> operator()(std::string const& name, Child* child) const {
    return noop<Child>(child);
  }
  */
  template <class V2>
  noop const& init(V2 const& v2) const {
    return *this;
  }
  template <class V2>
  noop const& init(bool enable, V2 const& v2) const {
    return init(v2);
  }
  // similar concept to implicit except that you have the --key implicit true, and --no-key implicit false
  noop const& flag(bool is_to = false) const { return init(is_to); }
  template <class V2>
  noop const& inits(V2 const& v2) {
    return *this;
  }

  /// the rest of the protocol are all no-ops

  template <class V2>
  noop const& null_ok(V2 const& val) const {
    return *this;
  }
  noop const& null_ok() const { return *this; }
  noop const& alias(bool = true) const { return *this; }
  template <class V2>
  noop const& eg(V2 const& eg) const {
    return *this;
  }
  noop const& operator()(char charname) const { return *this; }
  noop const& deprecate(std::string const& info = "", bool deprecated = true) const { return *this; }
  noop const& is_default(bool enable = true) const { return *this; }
  noop const& todo(bool enable = true) const { return *this; }
  noop const& verbose(int verbosity = 1) const { return *this; }
  noop const& positional(bool enable = true, int max = 1) const { return *this; }
  template <class unrecognized_opts>
  noop const& allow_unrecognized(bool enable, bool warn, unrecognized_opts* unrecognized_storage) const {
    return *this;
  }
  noop const& allow_unrecognized(bool enable = true, bool warn = false) const {
    return *this;
  }
  noop const& require(bool enable = true, bool just_warn = false) const { return *this; }
  noop const& desire(bool enable = true) const { return *this; }
  template <class V2>
  noop const& implicit(bool enable, V2 const& v2) const {
    return *this;
  }
  template <class V2>
  noop const& implicit(V2 const& v2) const {
    return *this;
  }
  noop const& defaulted(bool enable = true) const { return *this; }
  template <class V2>
  noop const& validate(V2 const& validator) const {
    return *this;
  }
};


}

#endif
