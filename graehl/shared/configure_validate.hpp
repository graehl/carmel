/** \file

    (doesn't support user specified validate fns)

    call .validate() on everything in the tree
*/

#ifndef CONFIGURE_VALIDATE_GRAEHL_2016_09_12_HPP
#define CONFIGURE_VALIDATE_GRAEHL_2016_09_12_HPP
#pragma once


#include <graehl/shared/configure_is.hpp>
#include <graehl/shared/configure_noop.hpp>
#include <graehl/shared/configure_traits.hpp>
#include <graehl/shared/shared_ptr.hpp>
#include <graehl/shared/validate.hpp>

namespace configure {

struct validate_tree : noop {
  validate_tree const& operator()(std::string const& usage) const { return *this; }
  validate_tree const& operator()(std::vector<char> const& usage) const { return *this; }
  validate_tree const& operator()(char charname) const { return *this; }
  template <class X>
  validate_tree const& operator()(std::string const& name, X* x) const {
    validate(x);
    return *this;
  }
  template <class X>
  void sibling(X* x) const {
    validate(x);
  }
  template <class X, class A1>
  void sibling(X* x, A1 const& a1) const {
    validate(x);
  }
  template <class X, class A1, class A2>
  void sibling(X* x, A1 const& a1, A2 const& a2) const {
    validate(x);
  }
  template <class X, class A1, class A2, class A3>
  void sibling(X* x, A1 const& a1, A2 const& a2, A3 const& a3) const {
    validate(x);
  }

  template <class X>
  void recurse_scalar(X* x) const {
    //::adl::adl_validate(*x);
  }
  template <class X>
  void recurse_configurable(X* x) const {
    GRAEHL_VALIDATE_LOG(x, "+validate_tree configurable");
    x->configure(*this);
    GRAEHL_VALIDATE_LOG(x, "-validate_tree - calling adl_validate");
    ::adl::adl_validate(*x);
  }
  template <class X>
  void recurse_configurable(shared_ptr<X>* x) const {
    shared_ptr<X>& p = *x;
    if (p) recurse_configurable(p.get());
  }
  template <class X>
  void validate(X* x) const {
    recurse_preorder(x, *this);
  }
};


}

#endif
