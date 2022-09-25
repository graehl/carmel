/** \file

 .
*/

#ifndef CONFIGURE_POLICY_GRAEHL_2016_09_12_HPP
#define CONFIGURE_POLICY_GRAEHL_2016_09_12_HPP
#pragma once

#include <graehl/shared/assign_traits.hpp>
#include <graehl/shared/configure_traits.hpp>
#include <graehl/shared/value_str.hpp>

namespace configure {


template <class T>
void init(T& t) {
} // default is to do nothing. ADL overridable. used to 'default construct' vectors of configurable objects

template <class T>
void init_default(T& t) // used if .init_default() conf_opt
{
  graehl::assign_traits<T>::init(t);
  init(t);
}

template <class T>
void call_init_default(T& t) {
  init_default(t);
}

using graehl::value_str;
struct conf_expr_base;

struct configure_policy_base {
  template <class Val, class InitVal>
  static void apply_init(Val* val, InitVal const& initVal, bool isInit, bool isInitDefault) {}
};

struct tree_configure_policy : configure_policy_base {
  template <class Val, class Expr>
  static void configure(Val* pval, Expr& expr) {
    pval->configure(expr);
  }
  template <class Backend, class Action, class Val>
  static bool init_tree(Backend const& backend, Action const& action, Val* pval, conf_expr_base const& conf) {
    return backend.do_init_tree(action, pval, conf);
  }
  template <class Backend, class Action, class Val>
  static void action(Backend const& backend, Action const& action, Val* pval, conf_expr_base const& conf) {
    backend.do_tree_action(action, pval, conf);
  }
  template <class Val, class Expr>
  static void configure(std::shared_ptr<Val>* pval, Expr& expr) {
    auto& p = *pval;
    p->configure(expr);
  }
  template <class Backend, class Action, class Val>
  static bool init_tree(Backend const& backend, Action const& action, std::shared_ptr<Val>* pval,
                        conf_expr_base const& conf) {
    auto& p = *pval;
    return backend.do_init_tree(action, p.get(), conf);
  }
  template <class Backend, class Action, class Val>
  static void action(Backend const& backend, Action const& action, std::shared_ptr<Val>* pval,
                     conf_expr_base const& conf) {
    auto& p = *pval;
    backend.do_tree_action(action, p.get(), conf);
  }
};

struct leaf_configure_policy : configure_policy_base {
  template <class Val>
  static void apply_init(Val* val, value_str const& initVal, bool isInit, bool isInitDefault) {
    if (isInit)
      initVal.assign_to(*val);
    else if (isInitDefault)
      call_init_default(*val);
  }
  template <class Backend, class Action, class Val>
  static bool init_tree(Backend const& backend, Action const& action, Val* pval, conf_expr_base const& conf) {
    return false;
  }
  template <class Val, class Expr>
  static void configure(Val* pval, Expr& expr) {}
  template <class Backend, class Action, class Val>
  static void action(Backend const& backend, Action const& action, Val* pval, conf_expr_base const& conf) {
    backend.do_leaf_action(action, pval, conf);
  }
};

struct map_configure_policy : configure_policy_base {
  template <class Backend, class Action, class Val>
  static bool init_tree(Backend const& backend, Action const& action, Val* pval, conf_expr_base const& conf) {
    backend.do_print_action_open(action, pval, conf);
    return true;
  }
  template <class Val, class Expr>
  static void configure(Val* pval, Expr& expr) {}
  template <class Backend, class Action, class Val>
  static void action(Backend const& backend, Action const& action, Val* pval, conf_expr_base const& conf) {
    backend.do_map_action(action, pval, conf);
    backend.do_print_map_sequence_action_close(action, pval, conf);
  }

  template <class Backend, class Action, class Val>
  static bool init_tree(Backend const& backend, Action const& action, std::shared_ptr<Val>* pval,
                        conf_expr_base const& conf) {
    auto& p = *pval;
    if (!p)
      p = std::make_shared<Val>();
    backend.do_print_action_open(action, p.get(), conf);
    return true;
  }
  template <class Val, class Expr>
  static void configure(std::shared_ptr<Val>* pval, Expr& expr) {}
  template <class Backend, class Action, class Val>
  static void action(Backend const& backend, Action const& action, std::shared_ptr<Val>* pval,
                     conf_expr_base const& conf) {
    auto& p = *pval;
    if (!p)
      p = std::make_shared<Val>();
    backend.do_map_action(action, p.get(), conf);
    backend.do_print_map_sequence_action_close(action, p.get(), conf);
  }
};

struct set_configure_policy : configure_policy_base {
  template <class Backend, class Action, class Val>
  static bool init_tree(Backend const& backend, Action const& action, Val* pval, conf_expr_base const& conf) {
    backend.do_print_action_open(action, pval, conf);
    return true;
  }
  template <class Val, class Expr>
  static void configure(Val* pval, Expr& expr) {}
  template <class Backend, class Action, class Val>
  static void action(Backend const& backend, Action const& action, Val* pval, conf_expr_base const& conf) {
    backend.do_set_action(action, pval, conf);
    backend.do_print_map_sequence_action_close(action, pval, conf);
  }
};

struct sequence_configure_policy : configure_policy_base {
  template <class Backend, class Action, class Val>
  static bool init_tree(Backend const& backend, Action const& action, Val* pval, conf_expr_base const& conf) {
    backend.do_print_action_open(action, pval, conf);
    return true;
  }
  template <class Val, class Expr>
  static void configure(Val* pval, Expr& expr) {}
  template <class Backend, class Action, class Val>
  static void action(Backend const& backend, Action const& action, Val* pval, conf_expr_base const& conf) {
    backend.do_sequence_action(action, pval, conf);
    backend.do_print_map_sequence_action_close(action, pval, conf);
  }
};

// struct rather than member fn because partial member fn template spec is tricky
template <class Val2, class Enable = void>
struct select_configure_policy {
  typedef tree_configure_policy type;
};
template <class Val2>
struct select_configure_policy<Val2, typename enable_if<scalar_leaf_configurable<Val2>::value>::type> {
  typedef leaf_configure_policy type;
};
template <class Val2>
struct select_configure_policy<Val2, typename enable_if<sequence_leaf_configurable<Val2>::value>::type> {
  typedef sequence_configure_policy type;
};
template <class Val2>
struct select_configure_policy<Val2, typename enable_if<map_leaf_configurable<Val2>::value>::type> {
  typedef map_configure_policy type;
};
template <class Val2>
struct select_configure_policy<Val2, typename enable_if<set_leaf_configurable<Val2>::value>::type> {
  typedef set_configure_policy type;
};


} // namespace configure

#endif
