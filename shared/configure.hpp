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

    a template library for specifying config structs (see
    configure_program_options.hpp for an example)
*/

#ifndef GRAEHL_SHARED__CONFIGURE_HPP
#define GRAEHL_SHARED__CONFIGURE_HPP
#pragma once

// TODO: allow boost::optional<non-leaf> such that backend option presence auto-constructs the optional value,
// which is otherwise left at none (or previous value)

// TODO: automatically unwrap optional<X> to X for validate()

// TODO: detect this error: same address Val configured w/ diff named options, given conflicting init
// defaults. provide explicit .alias(str) for multiple names?

// TODO: pretty word wrap + indent

#include <graehl/shared/ifdbg.hpp>
#ifndef GRAEHL_DEBUG_CONFIGURE_EXPR
#define GRAEHL_DEBUG_CONFIGURE_EXPR 0
#endif

#if GRAEHL_DEBUG_CONFIGURE_EXPR
#include <graehl/shared/show.hpp>
#define CONFEXPR(x) x
DECLARE_DBG_LEVEL(CONFEXPR)
#else
#define CONFEXPR(x)
#endif

#include <boost/type_traits/is_fundamental.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/logical.hpp>
#include <boost/any.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/pointee.hpp>
#include <boost/noncopyable.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/detail/atomic_count.hpp>
#include <boost/static_assert.hpp>

#include <graehl/shared/assign_traits.hpp>
#include <graehl/shared/shell_escape.hpp>
#include <graehl/shared/word_spacer.hpp>
#include <graehl/shared/warn.hpp>
#include <graehl/shared/string_to.hpp>
#include <graehl/shared/type_string.hpp>
#include <graehl/shared/example_value.hpp>
#include <graehl/shared/value_str.hpp>
#include <graehl/shared/validate.hpp>
#include <graehl/shared/leaf_configurable.hpp>
#include <graehl/shared/string_match.hpp>
#include <graehl/shared/configure_init.hpp>

#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <boost/optional.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <exception>

// TODO: overridable free fn for default usage string, e.g. longer explanation of enum type meaning

namespace configure {

typedef std::map<std::string, std::string> unrecognized_opts;

// validate_config(warn) is an action that follows store because we may want repeated stores from multiple
// sources. this means you need to explicitly call it after store (with validate_stored)

namespace {
const char path_sep = '.';
}

using graehl::string_consumer;
using graehl::warn_consumer;
using graehl::shell_quote;
using graehl::value_str;
using graehl::to_string;
using graehl::string_to;


namespace detail {
template <class T>
inline std::string config_is(T const& t) {
  using graehl::type_string;
  return type_string(t);  // unqualified for ADL
}
}

template <class T>
inline std::string call_config_is(T const& t) {
  using namespace detail;
  return config_is(t);  // for ADL
}

template <class O, class T, class Sep>
inline void print_opt(O& o, std::string const& name, boost::optional<T> const& opt, Sep const& sep) {
  if (opt) o << sep << name << '=' << *opt;
}

template <class O, class T>
inline void print_opt_else(O& o, std::string const& name, boost::optional<T> const& opt,
                           std::string const& none = "NONE") {
  o << name << '=';
  if (opt)
    o << *opt;
  else
    o << none;
}

inline std::string concat_optional(std::string const& a, std::string const& pre_if_b_nonempty,
                                   std::string const& b, std::string const& post_if_b_nonempty = "") {
  if (b.empty()) return a;
  if (a.empty()) return b;
  return a + pre_if_b_nonempty + b + post_if_b_nonempty;
}

inline std::string concat_optional(std::string const& a, std::string const& pre_if_b,
                                   boost::optional<std::string> const& maybe_b,
                                   std::string const& post_if_b = "") {
  if (a.empty()) return maybe_b.get_value_or("");
  return maybe_b ? a + pre_if_b + *maybe_b + post_if_b : a;
}

inline std::string concat_optional_skip_empty(std::string const& a, std::string const& pre_if_b,
                                              boost::optional<std::string> const& maybe_b,
                                              std::string const& post_if_b = "") {
  if (a.empty()) return maybe_b.get_value_or("");
  return maybe_b ? concat_optional(a, pre_if_b, *maybe_b, post_if_b) : a;
}

inline std::string prefix_optional(std::string const& pre_if_b, boost::optional<std::string> const& maybe_b,
                                   std::string const& post_if_b = "") {
  return maybe_b ? pre_if_b + *maybe_b + post_if_b : "";
}

template <class T>
void init(T& t) {
}  // default is to do nothing. ADL overridable. used to 'default construct' vectors of configurable objects

template <class T>
void init_default(T& t)  // used if .init_default() conf_opt
{
  graehl::assign_traits<T>::init(t);
  init(t);
}

template <class T>
void call_init_default(T& t) {
  init_default(t);
}

template <class Vec>
typename Vec::value_type& append_default(Vec& v) {
  typedef typename Vec::value_type T;
  v.push_back(T());
  T& r = v.back();
  call_init_default(r);
  return r;
}

/**
   this should never be called but is needed for compile. the paradigm of *first* adding a default element
   then modifying its value with configure() cannot work for std::set since the values are const.
*/
template <class Val>
Val& append_default(std::set<Val>& v) {
  assert(0);
  return *(Val*)0;
}

template <class Val>
void add_back(std::set<Val>& set, Val const& val) {
  set.insert(val);
}

template <class Val>
void add_back(std::vector<Val>& vec, Val const& val) {
  vec.push_back(val);
}

template <class Map>
typename Map::mapped_type& append_default(Map& map, typename Map::key_type const& key) {
  typedef typename Map::mapped_type T;
  T& r = map[key];
  call_init_default(r);
  return r;
}

template <class Val>
struct defaulted {
  Val val;
  Val* operator&() { return &val; }
  operator Val&() { return val; }
  operator Val const&() const { return val; }
  defaulted() { call_init_default(val); }
};

template <class Val>
struct nonconst_pointer {
  typedef Val* type;
};

template <class Val>
struct nonconst_pointer<Val const> {
  typedef Val* type;
};

using boost::mpl::false_;
using boost::mpl::true_;

template <class Val, class Enable = void>
struct has_leaf_configure : false_ {};

template <class Val>
struct has_leaf_configure<Val, typename Val::leaf_configure> : true_ {};

template <typename T>
struct is_string : false_ {};

template <typename charT, typename traits, typename Alloc>
struct is_string<std::basic_string<charT, traits, Alloc> > : true_ {};

// leaf_configurable means don't expect a .configure member.
template <class Val, class Enable>
struct leaf_configurable
    : boost::mpl::or_<has_leaf_configure<Val>, boost::is_fundamental<Val>, boost::is_enum<Val>, is_string<Val> > {
};

//"leaf" - use this for all classes that should be configured as leaves, where you
// can't add a member "typedef void leaf_configurable;"

template <class T1>
struct leaf_configurable<boost::optional<T1>, void> : leaf_configurable<T1> {};
template <class T1>
struct leaf_configurable<boost::shared_ptr<T1>, void> : leaf_configurable<T1> {};

template <class Val, class Enable = void>
struct scalar_leaf_configurable : leaf_configurable<Val> {};
template <class T1>
struct scalar_leaf_configurable<std::vector<T1>, void> : false_ {};
template <class T1, class T2>
struct scalar_leaf_configurable<std::map<T1, T2>, void> : false_ {};

template <class Val, class Enable = void>
struct map_leaf_configurable : false_ {};
template <class T1, class T2>
struct map_leaf_configurable<std::map<T1, T2>, void> : true_ {};

template <class Val, class Enable = void>
struct set_leaf_configurable : false_ {};
template <class T1>
struct set_leaf_configurable<std::set<T1>, void> : true_ {};

template <class Val, class Enable = void>
struct sequence_leaf_configurable : false_ {};
template <class T1>
struct sequence_leaf_configurable<std::vector<T1>, void> : true_ {};


template <class Val>
bool is_config_leaf(Val const&) {
  return leaf_configurable<Val>::value;
}


struct conf_expr_base;

template <class Val, class Enable = void>
struct has_assign_bool : false_ {};

template <class Val>
struct has_assign_bool<Val, typename Val::assign_bool> : true_ {};

template <class Val, class Enable = void>
struct can_assign_bool : boost::mpl::or_<has_assign_bool<Val>, boost::is_integral<Val> > {};

template <class Val>
struct can_assign_bool<boost::optional<Val>, void> : can_assign_bool<Val> {};

typedef boost::optional<std::string> maybe_string;  // syntax like string*

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

template <class Val2, class Enable = void>
struct select_configure_policy  // struct rather than member fn because partial member fn template spec is
    // iffy
    {
  typedef tree_configure_policy type;
};
template <class Val2>
struct select_configure_policy<Val2, typename boost::enable_if<scalar_leaf_configurable<Val2> >::type> {
  typedef leaf_configure_policy type;
};
template <class Val2>
struct select_configure_policy<Val2, typename boost::enable_if<sequence_leaf_configurable<Val2> >::type> {
  typedef sequence_configure_policy type;
};
template <class Val2>
struct select_configure_policy<Val2, typename boost::enable_if<map_leaf_configurable<Val2> >::type> {
  typedef map_configure_policy type;
};
template <class Val2>
struct select_configure_policy<Val2, typename boost::enable_if<set_leaf_configurable<Val2> >::type> {
  typedef set_configure_policy type;
};


struct conf_opt {
  typedef std::string string;

  typedef boost::optional<char> maybe_char;
  typedef boost::optional<int> maybe_int;
  typedef boost::optional<bool> maybe_bool;


  template <class Val>
  std::string get_is(Val const& val) const {
    return is ? *is : call_config_is(val);
  }

  bool is_todo() const { return todo.get_value_or(false); }
  bool is_required() const { return require && require->enable; }
  bool is_required_warn() const { return is_required() && require->just_warn; }
  bool is_required_err() const { return is_required() && !require->just_warn; }

  bool is_over_limit(int num_defined) const { return limit && num_defined > *limit; }

  bool is_too_verbose(int max_verbose = 1) const { return verbose && *verbose > max_verbose; }

  bool is_init_default() const { return init_default && *init_default; }

  std::string get_eg() const { return eg.get_value_or(init_str()); }

  template <class Val>
  std::string get_eg(Val const& val) const {
    using graehl::example_value;  // ADL
    return eg.get_value_or(is_init() ? init_str() : example_value(val));
  }

  std::string get_init_or_eg() const { return is_init() ? init_str() : eg.get_value_or(""); }

  template <class Val>
  std::string get_init_or_eg(Val const& val) const {
    using graehl::example_value;  // ADL
    return is_init() ? init_str() : eg.get_value_or(example_value(val));
  }

  template <class Val>
  std::string get_eg_suffix(Val const& val) const {
    return " (e.g. " + get_eg(val) + ")";
  }

  template <class HelpQuote>
  std::string get_eg_suffix_quote(HelpQuote const& help_quoter) const {
    std::string egs = get_eg();
    return egs.empty() ? egs : " (e.g. " + help_quoter.help_quote(egs) + ")";
  }

  template <class Val>
  std::string get_init_or_eg_suffix(Val const& val) const {
    return is_init() ? (" (default: " + init_str() + ")") : get_eg_suffix(val);
  }


  std::string required_suffix() const { return is_required() ? " [REQUIRED]" : ""; }

  template <class HelpQuote>
  std::string get_init_or_eg_suffix_quote(HelpQuote const& help_quoter) const {
    return is_init() ? (" (default: " + help_quoter.quote(init_str()) + ")") : get_eg_suffix_quote(help_quoter);
  }

  template <class HelpQuote>
  std::string get_help_suffix_quote(HelpQuote const& help_quoter) const {
    return get_init_or_eg_suffix_quote(help_quoter) + required_suffix();
  }

  template <class Val>
  std::string get_usage(Val const& val) const {
    std::string pre = get_is(val);
    return concat_optional(pre, usage&& pre.size() ? " - " : "", usage);
  }

  std::string get_usage() const { return get_usage_optional().get_value_or(""); }

  maybe_string get_usage_optional() const {
    return usage ? concat_optional_skip_empty(*usage, " (", is, ")") : is;
  }

  std::string get_is_suffix() const {
    if (!is || is->empty()) return "";
    return " (" + *is + ")";
  }

  template <class Val>
  std::string get_usage_eg(Val const& val) const {
    return get_usage(val) + get_eg_suffix(val);
  }

  template <class Val>
  std::string get_usage_init_or_eg(Val const& val) const {
    return get_usage(val) + get_init_or_eg_suffix(val);
  }

  std::string init_str() const { return is_init() ? init->value.str : ""; }

  template <class Val>
  std::string get_eg_suffix(Val* val) const {
    return get_eg_suffix(*val);
  }

  template <class Val>
  std::string get_leaf_value(Val const& val) const {
    typedef leaf_configurable<Val> leaf_val;
    if (!leaf_val::value) return "";
    SHOWIF1(CONFEXPR, 1, "leaf_value", graehl::to_string(val));
    return graehl::to_string(val);
  }

  struct positional_args {
    bool enable;
    int max;  //>0 = limited. 0=unlimited
    positional_args(bool enable, int max) : enable(enable), max(max) {}
    template <class O>
    void print(O& o) const {
      o << (enable ? "(on)" : "(off)");
      if (max > 0)
        o << max;
      else
        o << "unlimited";
    }
    template <class C, class T>
    friend std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& o, positional_args const& self) {
      self.print(o);
      return o;
    }
  };
  bool is_positional() const { return positional && positional->enable; }

  struct allow_unrecognized_args {
    bool enable;
    unrecognized_opts* unrecognized_storage;  // 0 means not enabled
    bool warn;
    std::string help;

    allow_unrecognized_args(bool enable = true, bool warn = false, unrecognized_opts* unrecognized_storage = 0,
                            std::string const& help = "allows unrecognized string key:val options")
        : enable(enable), unrecognized_storage(unrecognized_storage), warn(warn), help(help) {}
    template <class O>
    void print(O& o) const {
      if (enable) o << " allow unrecognized (";
      o << (unrecognized_storage ? "save, " : "");
      o << (warn ? "quiet" : "warn");
      o << ")";
    }
    template <class C, class T>
    friend std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& o,
                                                allow_unrecognized_args const& self) {
      self.print(o);
      return o;
    }
  };
  struct require_args {
    bool enable;
    bool just_warn;
    require_args(bool enable, bool just_warn) : enable(enable), just_warn(just_warn) {}
    template <class O>
    void print(O& o) const {
      o << (enable ? "(on)" : "(off)");
      o << (just_warn ? "fatal" : "warn");
    }
    template <class C, class T>
    friend std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& o, require_args const& self) {
      self.print(o);
      return o;
    }
  };
  struct deprecate_args {
    bool enable;
    std::string info;
    deprecate_args(bool enable, std::string const& info) : enable(enable), info(info) {}
    template <class O>
    void print(O& o) const {
      if (enable) o << "DEPRECATED: " << info;
    }
    struct deprecate_callback {
      deprecate_callback(std::string const& msg, string_consumer const& warn) : msg(msg), warn(warn) {}
      std::string msg;
      string_consumer warn;
      void operator()() { warn(msg); }
    };
    deprecate_callback get_notify0(string_consumer const& warn, std::string const& pathname) const {
      std::string suffix = info.empty() ? "." : "; " + info + ".";
      return deprecate_callback(pathname + " is deprecated" + suffix, warn);
    }
    template <class C, class T>
    friend std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& o, deprecate_args const& self) {
      self.print(o);
      return o;
    }
  };
  struct implicit_args {
    bool enable;
    value_str value;
    template <class T>
    implicit_args(bool enable, T const& implicit)
        : enable(enable), value(implicit) {}
    template <class O>
    void print(O& o) const {
      if (enable) o << " implicit value of no-argument option=" << value;
    }
    template <class C, class T>
    friend std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& o, implicit_args const& self) {
      self.print(o);
      return o;
    }
  };
  struct init_args {
    bool enable;
    value_str value;
    template <class T>
    init_args(bool enable, T const& init)
        : enable(enable), value(init) {}
    template <class O>
    void print(O& o) const {
      if (enable) o << " default value=" << value;
    }
    template <class C, class T>
    friend std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& o, init_args const& self) {
      self.print(o);
      return o;
    }
  };
  template <class Arg>
  static inline bool enabled(boost::optional<Arg> const& x) {
    return x && x->enable;
  }
  bool is_init() const { return enabled(init); }
  bool is_implicit() const { return enabled(implicit); }
  bool is_deprecated() const { return enabled(deprecate); }
  bool allows_unrecognized() const { return enabled(allow_unrecognized); }

  boost::optional<init_args> init;
  boost::optional<implicit_args> implicit;
  boost::optional<allow_unrecognized_args> allow_unrecognized;
  boost::optional<positional_args> positional;
  boost::optional<require_args> require;
  boost::optional<deprecate_args> deprecate;
  maybe_char charname;  // single char option
  maybe_string eg, is, usage;
  maybe_bool init_default;
  bool alias;  // TODO: allow ignoring part of the (back end managed, so tricky) pathname for backward compat
  maybe_int verbose;
  maybe_int limit;  // max amount of children allowed - e.g. max=1 means it's a variant
  maybe_bool todo;  // don't actually allow parsing the option, or add it to usage
// warning: keep this list current - look at above boost::optional and maybe_ typedefs
#define CONFIGURE_OPTIONAL_MEMBERS(x)                                                                     \
  x(charname) x(eg) x(is) x(usage) x(deprecate) x(verbose) x(allow_unrecognized) x(positional) x(require) \
      x(init_default) x(implicit) x(init) x(todo)

  allow_unrecognized_args get_unrecognized() const {
    return allow_unrecognized.get_value_or(allow_unrecognized_args(false));
  }

  void clear() {
#define CONFIGURE_INIT_OPTIONAL_MEMBERS(m) m = (boost::none);
    CONFIGURE_OPTIONAL_MEMBERS(CONFIGURE_INIT_OPTIONAL_MEMBERS);
    alias = false;
  }

  conf_opt() { clear(); }

  enum Inherit { inherit = 1 };

  /// if parent, inherit certain options (for now, none)
  conf_opt(conf_opt* parent, Inherit inh) {
    assert(inh == inherit);
    clear();
    assert(!require);
  }

  template <class O>
  void print(O& o) const {
    graehl::word_spacer_f sp;
#define CONFIGURE_OPTIONAL_PRINT(m) print_opt(o, #m, m, sp);
    CONFIGURE_OPTIONAL_MEMBERS(CONFIGURE_OPTIONAL_PRINT);
  }
  template <class C, class T>
  friend std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& o, conf_opt const& self) {
    self.print(o);
    return o;
  }

  template <class Val>
  void apply_init(Val* pval) const {
    bool const do_init = is_init();
    bool const init_default = is_init_default();
    if (do_init || init_default)
      select_configure_policy<Val>::type::apply_init(pval, init ? init->value : value_str(), do_init,
                                                     init_default);
  }

  void warn_deprecated(std::string const& pathname, string_consumer const& warn) const {
    if (is_deprecated()) deprecate->get_notify0(warn, pathname)();
  }

  template <class Val>
  void apply_string_value(std::string str, Val* pval, std::string const& pathname,
                          string_consumer const& warn) const {
    warn_deprecated(pathname, warn);
    string_to(str, *pval);
  }

  template <class Val>
  void apply_implicit_value(Val* pval, std::string const& pathname, string_consumer const& warn) const {
    warn_deprecated(pathname, warn);
    if (is_implicit()) implicit->value.assign_to(*pval);
  }
};

typedef boost::shared_ptr<conf_opt> p_conf_opt;

struct conf_expr_base;

typedef boost::shared_ptr<conf_expr_base> p_conf_expr_base;


enum config_action_type {
  kInitConfig,
  kStoreConfig,
  kValidateConfig,
  kHelpConfig,
  kShowExampleConfig,
  kShowEffectiveConfig,
  knConfigActionType
};

struct config_action {
  /// return whether this is a store action. necessary for self-pointer configure
  static bool store() { return false; }
  enum { action = knConfigActionType };
};

inline char const* name(config_action_type type) {
  switch (type) {
    case kInitConfig:
      return "init";
    case kStoreConfig:
      return "store";
    case kValidateConfig:
      return "validate";
    case kHelpConfig:
      return "help";
    case kShowExampleConfig:
      return "show-example";
    case kShowEffectiveConfig:
      return "show-effective";
    default:
      return "invalid action type";
  };
}

inline std::ostream& operator<<(std::ostream& out, config_action_type type) {
  return out << name(type);
}

// standard actions:
struct init_config : config_action {
  enum { action = kInitConfig };
};  // required to call this before store etc. - actually initializes objects w/ defaults as specified

struct store_config : config_action {
  enum { action = kStoreConfig };
  static bool store() { return true; }
};  // backend implementers: remember to check required() in store_config

struct validate_config : config_action {
  enum { action = kValidateConfig };
};

struct help_config : config_action {
  enum { action = kHelpConfig };
  std::ostream* o;
  help_config(std::ostream& o) : o(&o) {}
  help_config(help_config const& o) : o(o.o) {}
};

struct show_example_config : config_action {
  enum { action = kShowExampleConfig };
  std::ostream* o;
  show_example_config(std::ostream& o) : o(&o) {}
  show_example_config(show_example_config const& o) : o(o.o) {}
};

struct show_effective_config : config_action {
  enum { action = kShowEffectiveConfig };
  std::ostream* o;
  show_effective_config(std::ostream& o) : o(&o) {}
  show_effective_config(show_effective_config const& o) : o(o.o) {}
};

struct default_conf_expr_base : config_action {
  p_conf_expr_base pconf;
};

// optional:
struct check_config {};  // checks protocol validity without really doing anything. e.g. would detect multiple
// options with same name

template <class Action>
config_action_type action_type() {
  return (config_action_type)Action::action;
}

template <class Action>
char const* action_name(Action const& action) {
  return name(action_type<Action>());
}

template <class Action>
bool is_init(Action const&) {
  return action_type<Action>() == kInitConfig;
}

template <class Action>
bool is_help(Action const&) {
  return action_type<Action>() == kHelpConfig;
}

template <class Action>
bool is_store(Action const&) {
  return action_type<Action>() == kStoreConfig;
}

inline bool is_store(config_action_type action) {
  return action == kStoreConfig;
}

namespace {
const int default_verbose_max = 100;
}

using boost::detail::atomic_count;

template <class FinishableCRTP  // has a .finish() member to be called whenever release() -> count 0. note:
          // count starts at 1.
          ,
          class R = atomic_count>
class finish_refcount {
  finish_refcount() : refcount(new R(1)) {}
  finish_refcount(finish_refcount const& o) : refcount(o.refcount) { add_ref(); }
  ~finish_refcount() { release(); }
  inline void add_ref() const { ++*refcount; }
  inline void release(FinishableCRTP const* tc) const {
    FinishableCRTP* t = const_cast<FinishableCRTP*>(tc);
    if (!--*refcount) {
      delete refcount;
      t->finish();
    }
  }
  inline void release() const { release(derivedPtr()); }
  inline FinishableCRTP const* derivedPtr() const { return static_cast<FinishableCRTP const*>(this); }

 private:
  void operator=(finish_refcount const&) {}
  R* refcount;
};


template <class MapOrSet, class Val>
bool contains(MapOrSet const& map, Val const& val) {
  return map.find(val) != map.end();
}

typedef std::vector<std::string> opt_path;

inline std::string join_opt_path(opt_path const& p, char sep = path_sep) {
  std::ostringstream o;
  graehl::word_spacer sp(sep);
  for (opt_path::const_iterator i = p.begin(), e = p.end(); i != e; ++i) o << sp << *i;
  return o.str();
}

inline std::string join_opt_path(opt_path const& p, std::string last, char sep = path_sep) {
  std::ostringstream o;
  for (opt_path::const_iterator i = p.begin(), e = p.end(); i != e; ++i) o << *i << sep;
  o << last;
  return o.str();
}

struct conf_expr_base {
  void warn_if_deprecated() const { opt->warn_deprecated(path_name(), warn_to); }

  opt_path path;  // may not be needed, but helps inform exceptions
  unsigned depth;
  string_consumer warn_to;
  p_conf_opt opt;

  typedef boost::optional<boost::any> validate_callback;
  mutable validate_callback validator;

  // TODO: not sure if this can be correct when using non-compositional
  // configure() - might need dynamic_cast from Val to whatever validate takes as
  // its argument? confusing.
  template <class Val, class Validate>
  void set_validate(Validate const& validate) const {
    throw config_exception("TODO: custom validate implementation is questionable");
    validator = boost::function<void(Val&)>(validate);
  }
  template <class Val>
  bool custom_validate(Val* pval) const {
    if (!validator) return false;
    boost::any_cast<boost::function<void(Val&)> >(*validator)(*pval);
    return true;
  }
  template <class Val>
  void call_validate(Val* pval) const {
    try {
      if (!custom_validate(pval)) ::adl::adl_validate(*pval);
    } catch (std::exception& e) {
      graehl::string_builder msg;
      if (!path.empty()) msg("key '")(path_name())("' ");
      msg("failed validation: ")(e.what());
      throw config_exception(msg.str());
    }
  }
  explicit conf_expr_base(string_consumer const& warn_to = warn_consumer(),
                          opt_path const& root_path = opt_path())
      : path(root_path), depth(), warn_to(warn_to), opt(new conf_opt()) {}

  /// parent is cloned with inheritance of (used to be require, now nothing)
  conf_expr_base(conf_expr_base const& parent, std::string const& name)
      : path(parent.path)
      , depth(parent.depth + 1)
      , warn_to(parent.warn_to)
      , opt(new conf_opt(parent.opt.get(), conf_opt::inherit)) {
    path.push_back(name);
  }

  std::string path_name() const { return join_opt_path(path, path_sep); }
  std::string name() const { return path.empty() ? "" : path.back(); }
  template <class Val>
  std::string description(Val const& val) const {
    return concat_optional(name(), " - ", opt->get_usage(val));
  }
  std::string description() const { return concat_optional(name(), " - ", opt->get_usage()); }

  template <class O>
  void print(O& o) const {
    o << path_name() << ": " << *opt;
  }
  template <class Ch, class Tr>
  friend std::basic_ostream<Ch, Tr>& operator<<(std::basic_ostream<Ch, Tr>& o, conf_expr_base const& self) {
    self.print(o);
    return o;
  }

 protected:
  typedef conf_opt::require_args require_args;  // TODO

  mutable std::set<std::string> subnames;  // duplicate checking
  void check_name(std::string const& name) const {
    if (!subnames.insert(name).second)
      throw config_exception("duplicate configuration option: " + join_opt_path(path, name, path_sep));
  }
  void check_name(char c) const { check_name(std::string(1, c)); }
};

template <class Backend, class Action, class RootVal>
void configure_action(Backend const& c, Action const& action, RootVal* pval,
                      string_consumer const& warn_to = warn_consumer(), opt_path const& root_path = opt_path());

// Temporary objects are destroyed as the last step in evaluating the full-expression (1.9) that (lexically)
// contains the point where they were created. - std 12.2/3 (can live longer if named temp const ref stores
// result)

struct conf_expr_destroy {
  virtual std::string what() const = 0;
  virtual ~conf_expr_destroy() { SHOWIF1(CONFEXPR, 3, "processing sub-config ", what()); }
};

// template <typename> class Backends
// typedef Backends<Val> Backend;
template <class Backend, class Action, class Val>
struct conf_expr : Backend, conf_expr_base, boost::noncopyable, conf_expr_destroy {
 private:
  mutable boost::shared_ptr<conf_expr_destroy> subconfig_deleter;

 public:
  bool store() const { return is_store(action); }
  std::string what() const {
    std::string r(conf_expr_base::path_name());
    r.push_back(' ');
    r += action_name(action);
    return r;
  }
  enum { kMaxExpandDepth = 50 };
  // for configure_lazy
  bool store_expand() const {
    bool const infiniteRecursion = depth > kMaxExpandDepth;
    if (infiniteRecursion)
      throw config_exception(
          "configuration reached depth more than maximum - infinite recursion in configure() methods?");
    return !infiniteRecursion && init_tree_return && Action::store();
  }

  conf_expr_base const& base() const { return (conf_expr_base const&)*this; }
  Backend const& backend() const { return (Backend const&)*this; }

  Action action;
  Val* pval;

  typedef typename select_configure_policy<Val>::type configure_policy;

  bool init_tree_return;

  explicit conf_expr(Backend const& o, Action const& action, Val* pval, conf_expr_base const& conf_base)
      : Backend(o)
      , conf_expr_base(conf_base)
      , action(action)
      , pval(pval)
  // requirement: Backend(Backend const&)
  {
    if (!opt->is) opt->is = call_config_is(*pval);
    Backend::do_init(action, pval, base());
    init_tree_return = configure_policy::init_tree(backend(), action, pval, base());
    configure_policy::configure(pval, *this);  // unless leaf, call pval->configure(*this);
    // requirement: template<class action> Backend.init(action, Val *pval, conf_opt_base &conf)
    SHOWIF1(CONFEXPR, 1, "<conf_expr>", base());
  }

  bool skip_help() const { return is_help(action) && Backend::too_verbose(*this); }

  ~conf_expr()  // destructor makes things happen; however, we want to take final action only in context of
  // caller and all of caller's overrides (when name and superceding parent values are known)
  {
    drop();
    if (skip_help()) return;
    if (is_init(action)) opt->apply_init(pval);  // only works on leaves for now
    if (opt->is_todo()) return;
    configure_policy::action(backend(), action, pval, base());
    SHOWIF1(CONFEXPR, 1, "</conf_expr>", base());
  }

 public:
  bool root() const { return depth == 0; }

  void hold(conf_expr_destroy* destroyOnDrop) const { subconfig_deleter.reset(destroyOnDrop); }

  void drop() const {
    subconfig_deleter.reset();  // trigger previous action
  }

  // requirement: Backend(Backend const&, string subname)
  template <class Vchild>
  conf_expr<Backend, Action, Vchild> const& operator()(std::string const& name, Vchild* pchild) const {
    drop();
    check_name(name);
    typedef conf_expr<Backend, Action, Vchild> SubConfExpr;
    SubConfExpr* ret = new SubConfExpr(*this, action, pchild, conf_expr_base(*this, name));
    hold(ret);
    return *ret;
  }

  conf_expr const& eg(std::string const& eg_str) const {
    opt->eg = eg_str;
    return *this;
  }
  conf_expr const& eg(char const* eg_str) const {
    opt->eg = eg_str;
    return *this;
  }
  conf_expr const& is_also(std::string const& is) {
    if (opt->is)
      opt->is = *opt->is + ", " + is;
    else
      opt->is = is;
    return *this;
  }
  conf_expr const& is(std::string const& is) const {
    opt->is = is;
    return *this;
  }
  // TODO: concatenate the different is() for non-hierarchical siblings?
  template <class Val2>
  conf_expr const& eg(Val2 const& eg_val) const {
    opt->eg = to_string(eg_val);
    return *this;
  }
  conf_expr const& operator()(char charname) const {
    check_name(charname);
    opt->charname = charname;
    return *this;
  }
  conf_expr const& operator()(std::string const& usage) const {
    opt->usage = usage;
    return *this;
  }
  conf_expr const& deprecate(std::string const& info = "", bool deprecated = true) const {
    opt->deprecate = conf_opt::deprecate_args(deprecated, info);
    return *this;
  }
  conf_expr const& init_default(bool enable = true) const {
    opt->init_default = enable;
    return *this;
  }
  conf_expr const& todo(bool enable = true) const {
    opt->todo = enable;
    return *this;
  }
  conf_expr const& verbose(int verbosity = 1) const {
    opt->verbose = verbosity;
    return *this;
  }
  conf_expr const& alias() const {
    opt->alias = true;
    return *this;
  }
  /// -1 means unlimited
  conf_expr const& limit(int limitTo = 1) const {
    if (limitTo >= 0)
      opt->limit = limitTo;
    else
      opt->limit = boost::none;
    return *this;
  }
  conf_expr const& positional(bool enable = true, int max = 1) const {
    opt->positional = conf_opt::positional_args(enable, max);
    return *this;
  }
  conf_expr const& allow_unrecognized(bool enable = true, bool warn = false,
                                      unrecognized_opts* unrecognized_storage = 0) const {
    opt->allow_unrecognized = conf_opt::allow_unrecognized_args(enable, warn, unrecognized_storage);
    return *this;
  }
  conf_expr const& require(bool enable = true, bool just_warn = false) const {
    opt->require = conf_opt::require_args(enable, just_warn);
    return *this;
  }
  conf_expr const& desire(bool enable = true) const { return require(enable, true); }

  /* TODO:
     #. `.flag(bool enable=true, boost::optional<bool> init_to=false)` - hint to
     command line parser to use `--key` and `--no-key` zero-argument flags. `Val`
     should be `bool` or `optional<bool>`, or integer (gets 0 or 1 per normal
     C++). For all backends, if `init_to` is not `boost::none`, then the value is
     initialized to `*init_to`. The accompanying `init_to` is ignored if `enable=false`.
  */

  // similar concept to implicit except that you have the --key implicit true, and --no-key implicit false
  conf_expr const& flag(bool init_to = false) const {
    if (!can_assign_bool<Val>::value) throw config_exception("flag specified for non-boolean option");
    Val v = init_to;  // so we store the right type of boost::any
    init(true, v);
    Val notv = !init_to;
    implicit(notv);
    return *this;
  }

  template <class V2>
  conf_expr const& implicit(bool enable, V2 const& v2) const {
    Val val((v2));  // so we store the right type of boost::any
    opt->implicit = conf_opt::implicit_args(enable, val);
    return *this;
  }
  template <class V2>
  conf_expr const& implicit(V2 const& v2) const {
    return implicit(true, v2);
  }
  template <class V2>
  conf_expr const& init(bool enable, V2 const& v2) const {
    Val val((v2));  // so we store the right type of boost::any in init_args
    opt->init = conf_opt::init_args(enable, val);
    return *this;
  }
  template <class V2>
  conf_expr const& init(V2 const& v2) const {
    return init(true, v2);
  }
  conf_expr const& self_init(bool enable = true) const {
    if (enable)
      init(enable, *pval);
    else
      opt->init = boost::none;
    return *this;
  }
  /**
     like self_init, but configure_init.hpp treats this differently (actually
     assigns v2, for default constructors)
  */
  template <class V2>
  conf_expr const& inits(V2 const& v2) {
    return self_init(true);
  }

  template <class V2>
  conf_expr const& validate(V2 const& validator) const {
    this->template set_validate<Val, V2>(validator);
    return *this;
  }
};


template <class Backend, class Action, class RootVal>
void configure_action(Backend const& backend, Action const& action, RootVal* pval,
                      string_consumer const& warn_to, opt_path const& root_path) {
  if (backend.do_init_action(action))
    conf_expr<Backend, Action, RootVal> rc(backend, action, pval, conf_expr_base(warn_to, root_path));
  // destructor makes it happen. pval is irrelevant unless leaf.
}

template <class Backend, class Action, class RootVal>
void configure_action_from_base(Backend const& backend, Action const& action, RootVal* pval,
                                conf_expr_base const& base) {
  if (backend.do_init_action(action)) conf_expr<Backend, Action, RootVal> rc(backend, action, pval, base);
  // destructor makes it happen. pval is irrelevant unless leaf.
}

/**
   useful in recursing on backend sequence / map / set default-initialization.
*/
template <class Backend, class RootVal>
void configure_store_init_from_base(Backend const& backend, RootVal* pval, conf_expr_base const& base) {
  conf_expr<Backend, init_config, RootVal> b1(backend, init_config(), pval, base);
  conf_expr<Backend, store_config, RootVal> b2(backend, store_config(), pval, base);
  // destructor makes it happen. pval is irrelevant unless leaf.
}

namespace {
std::string tab = "  ";

/** return number of characters indented. */
template <class O>
unsigned indent_line(O& o, unsigned depth) {
  unsigned indent = depth * (unsigned)tab.size();
  while (depth) {
    --depth;
    o << tab;
  }
  return indent;
}
}

template <class ConfigureBackend>
inline unsigned max_column(ConfigureBackend const& conf) {
  return 80;
}  // override this if you like
template <class ConfigureBackend>
inline unsigned hanging_column_offset(ConfigureBackend const& conf) {
  return 4;
}  // override this if you like

inline unsigned indent_line_len(unsigned ntabs) {
  return (unsigned)tab.size() * ntabs;
}
inline unsigned n_tabs(conf_expr_base const& conf) {
  return conf.depth > 0 ? conf.depth - 1 : 0;
}
inline unsigned indent_line_len(conf_expr_base const& conf) {
  return indent_line_len(n_tabs(conf));
}
inline unsigned indent_conf_line(std::ostream& o, conf_expr_base const& conf) {
  return indent_line(o, n_tabs(conf));
}

inline std::string parent_option_name(std::string const& s) {
  std::string::size_type pos = s.rfind(path_sep);
  return pos == std::string::npos ? "" : std::string(s.begin(), s.begin() + pos);
}

struct configure_backend {
  virtual ~configure_backend() {}
  typedef boost::shared_ptr<configure_backend const> ptr;
};


/** minimum set of required Backend methods*/
struct null_backend : configure_backend {
  template <class Action, class Val>
  bool do_init_tree(Action const&, Val*, conf_expr_base const&) const {
    return true;
  }

  template <class Action>
  bool do_init_action(Action const&) const {
    return true;
  }

  template <class Action, class Val>
  void do_init(Action const&, Val*, conf_expr_base const&) const {}

  template <class Action, class Val>
  void do_tree_action(Action const&, Val*, conf_expr_base const&) const {}
  template <class Action, class Val>
  void do_leaf_action(Action const&, Val*, conf_expr_base const&) const {}
  template <class Action, class Val>
  void do_map_action(Action const&, Val*, conf_expr_base const&) const {}
  template <class Action, class Val>
  void do_sequence_action(Action const&, Val*, conf_expr_base const&) const {}
  template <class Action, class Val>
  void do_set_action(Action const&, Val*, conf_expr_base const&) const {}

  bool too_verbose(conf_expr_base const&) const { return false; }

  template <class Action, class Val>
  void do_print_action_open(Action const&, Val*, conf_expr_base const&) const {}
  template <class Action, class Val>
  void do_print_action_close(Action const&, Val*, conf_expr_base const&) const {}
  template <class Action, class Val>
  void do_print_map_sequence_action_close(Action const&, Val*, conf_expr_base const&) const {}
};

/** Doesn't recurse; serves only to store the final object default values with default_conf_expr_base action.
 * note that external overrides aren't available. */
struct get_default_conf_backend : null_backend {
 private:
  unsigned start_depth;
  bool done(conf_expr_base const& conf) const { return conf.depth == start_depth; }

  void finish(default_conf_expr_base const& a, conf_expr_base const& conf) const {
    if (done(conf)) *a.pconf = conf;
  }

 public:
  get_default_conf_backend(default_conf_expr_base& a, conf_expr_base const& conf) : start_depth(conf.depth) {
    a.pconf.reset(new conf_expr_base());
  }

  template <class Action, class Val>
  void do_init(Action const& a, Val* pval, conf_expr_base const& conf) const {}

  template <class Action>
  bool do_init_action(Action const& a) const {
    return false;
  }

  bool do_init_action(default_conf_expr_base const&) const { return true; }

  template <class Action, class Val>
  void do_tree_action(Action const& a, Val* pval, conf_expr_base const& conf) const {}
  template <class Action, class Val>
  void do_leaf_action(Action const& a, Val* pval, conf_expr_base const& conf) const {}
  template <class Action, class Val>
  void do_map_action(Action const& a, Val* pval, conf_expr_base const& conf) const {}
  template <class Action, class Val>
  void do_sequence_action(Action const& a, Val* pval, conf_expr_base const& conf) const {}
  template <class Action, class Val>
  void do_set_action(Action const& a, Val* pval, conf_expr_base const& conf) const {}

  template <class Val>
  void do_tree_action(default_conf_expr_base const& a, Val* pval, conf_expr_base const& conf) const {
    finish(a, conf);
  }
  template <class Val>
  void do_leaf_action(default_conf_expr_base const& a, Val* pval, conf_expr_base const& conf) const {
    finish(a, conf);
  }
  template <class Val>
  void do_map_action(default_conf_expr_base const& a, Val* pval, conf_expr_base const& conf) const {
    finish(a, conf);
  }
  template <class Val>
  void do_sequence_action(default_conf_expr_base const& a, Val* pval, conf_expr_base const& conf) const {
    finish(a, conf);
  }
  template <class Val>
  void do_set_action(default_conf_expr_base const& a, Val* pval, conf_expr_base const& conf) const {
    finish(a, conf);
  }
};

template <class Val>
conf_expr_base get_default_conf(Val* pval, conf_expr_base const& conf = conf_expr_base()) {
  default_conf_expr_base action;
  get_default_conf_backend backend(action, conf);
  configure_action_from_base(backend, action, pval, conf);
  return *action.pconf;
}


/** This is what's expected of you when you define a configure Backend.

    To provide defaults (without using virtual methods), inherit using the CRTP pattern:

    struct YOUR_BACKEND : configure_backend_base<YOUR_BACKEND> {
    FORWARD_BASE_CONFIGURE_ACTIONS(configure_backend_base<YOUR_BACKEND>)
    template <class Val>
    void leaf_action(configure::store_action, Val *val, configure::conf_expr_base const& conf) const {
    }
    };

    YOUR_BACKEND should be copy_constructible (use shared_ptr for mutable state) and the action methods should
   be const. Note that it's the do_ methods that are actually called (the rest are just to make it easier to
   provide default implementations).

    The macro is needed because leaf_action(store_action,...) would otherwise hide
   leaf_action(init_action,...) etc. You can forward those without the macro, of course.
*/
template <class CRTP>
struct configure_backend_base : configure_backend {
  // note: the design of printing actions would likely be simplified by returning an expression tree filled
  // with the eg, usage, init, etc. values. formatting of that would be handled by the backend. even a "build
  // list of lines" rather than "print to ostream" would add flexibility: you'd be easily able to show the
  // *final* values (not just the default ones) for .is(), usage, etc. with the header, not only postorder
 private:
  CRTP const& sub() const { return *static_cast<CRTP const*>(this); }

 public:
  string_consumer warn;
  int verbose_max;

  // TODO: refactor to put more common code here for YAML + prog opts, e.g. closest ancestor pathname for
  // allow unknown

  explicit configure_backend_base(string_consumer const& warn, int verbose_max = default_verbose_max)
      : warn(warn), verbose_max(verbose_max) {}
  bool too_verbose(conf_expr_base const& conf) const { return conf.opt->is_too_verbose(verbose_max); }

  configure_backend_base(configure_backend_base const& o) : warn(o.warn), verbose_max(o.verbose_max) {}

  // return false iff you handled the action completely, without needing the below 4 tree walking callbacks:
  template <class Action>
  bool do_init_action(Action const& a) const {
    return sub().init_action(a);
  }

  // conf_expr_base has e.g. conf.name(), conf.path_name(), conf.warn_to
  // for Action = each of the action types above - not necessarily a template on Action
  template <class Action, class Val>
  void do_init(Action const& a, Val* pval, conf_expr_base const& conf) const {
    sub().init(a, pval, conf);
  }
  template <class Action, class Val>
  void do_leaf_action(Action const& a, Val* pval, conf_expr_base const& conf) const {
    sub().leaf_action(a, pval, conf);
  }
  template <class Val>
  void do_leaf_action(help_config const& a, Val* pval, conf_expr_base const& conf) const {
    sub().leaf_action(a, pval, conf);
  }

  template <class Action, class Val>  // called after leaves and sub-configs
  void do_tree_action(Action const& a, Val* pval, conf_expr_base const& conf) const {
    sub().tree_action(a, pval, conf);
  }
  template <class Action, class Val>  // called before configure. init() is still called
  bool do_init_tree(Action const& a, Val* pval, conf_expr_base const& conf) const {
    sub().print_action_open(a, pval, conf);
    // we don't have usage, etc set at this point - see tree_action which happens after. if you wanted to
    // pretty-format with the info BEFORE the children, you'd have to store objects THEN print
    return sub().init_tree(a, pval, conf);
  }

  template <class Action, class Val>
  void print_action_open(Action const& a, Val* pval, conf_expr_base const& conf) const {}
  template <class Action, class Val>  // called after leaves and sub-configs
  void print_action_close(Action, Val* pval, conf_expr_base const& conf) const {}
  template <class Action, class Val>
  void print_map_sequence_action_close(Action, Val* pval, conf_expr_base const& conf) const {}
  template <class Action, class Val>
  void do_print_action_open(Action const& a, Val* pval, conf_expr_base const& conf) const {}
  template <class Action, class Val>  // called after leaves and sub-configs
  void do_print_action_close(Action, Val* pval, conf_expr_base const& conf) const {}
  template <class Action, class Val>
  void do_print_map_sequence_action_close(Action, Val* pval, conf_expr_base const& conf) const {}

  template <class Val>
  void print_action_open(help_config const& a, Val* pval, conf_expr_base const& conf) const {
    sub().default_print_action_open(a, pval, conf);
  }
  template <class Val>
  void print_action_open(show_example_config const& a, Val* pval, conf_expr_base const& conf) const {
    sub().default_print_action_open(a, pval, conf);
  }
  template <class Val>
  void print_action_open(show_effective_config const& a, Val* pval, conf_expr_base const& conf) const {
    sub().default_print_action_open(a, pval, conf);
  }
  template <class Val>
  void do_print_action_open(help_config const& a, Val* pval, conf_expr_base const& conf) const {
    sub().print_action_open(a, pval, conf);
  }
  template <class Val>
  void do_print_action_open(show_example_config const& a, Val* pval, conf_expr_base const& conf) const {
    sub().print_action_open(a, pval, conf);
  }
  template <class Val>
  void do_print_action_open(show_effective_config const& a, Val* pval, conf_expr_base const& conf) const {
    sub().print_action_open(a, pval, conf);
  }

  template <class Val>
  void do_print_action_close(help_config const& a, Val* pval, conf_expr_base const& conf) const {
    sub().print_action_close(a, pval, conf);
  }
  template <class Val>
  void do_print_action_close(show_example_config const& a, Val* pval, conf_expr_base const& conf) const {
    sub().print_action_close(a, pval, conf);
  }
  template <class Val>
  void do_print_action_close(show_effective_config const& a, Val* pval, conf_expr_base const& conf) const {
    sub().print_action_close(a, pval, conf);
  }

  template <class Val>
  void do_print_map_sequence_action_close(help_config const& a, Val* pval, conf_expr_base const& conf) const {
    sub().print_map_sequence_action_close(a, pval, conf);
  }
  template <class Val>
  void do_print_map_sequence_action_close(show_example_config const& a, Val* pval,
                                          conf_expr_base const& conf) const {
    sub().print_map_sequence_action_close(a, pval, conf);
  }
  template <class Val>
  void do_print_map_sequence_action_close(show_effective_config const& a, Val* pval,
                                          conf_expr_base const& conf) const {
    sub().print_map_sequence_action_close(a, pval, conf);
  }

  template <class Val>
  void print_map_sequence_action_close(help_config const& a, Val* pval, conf_expr_base const& conf) const {
    maybe_string const& usage = conf.opt->usage;
    if (usage) {
      sub().print_usage(*a.o, *usage, 0, indent_line_len(conf));
      *a.o << '\n';
    }
  }

  /** sequence_action, map_action, leaf_action, and tree_action (has a
      .configure()) are mutually exclusive.  your map_action and sequence_action
      should likely pval->clear() on store_config (unless you want configuration
      to be merged with existing contents), and should ultimately recurse with
      configure::configure_action_from_base(*this, configure::store_config(), &subval, subconf)
      after adding subval to the map or sequence. you can call
      defer_ {map, sequence}_action to do the recursion.
  */
  template <class Action, class Val>
  void do_sequence_action(Action const& a, Val* pval, conf_expr_base const& conf) const {
    sub().sequence_action(a, pval, conf);
  }
  template <class Action, class Val>
  void do_set_action(Action const& a, Val* pval, conf_expr_base const& conf) const {
    sub().set_action(a, pval, conf);
  }
  template <class Action, class Val>
  void do_map_action(Action const& a, Val* pval, conf_expr_base const& conf) const {
    sub().map_action(a, pval, conf);
  }

  /// you must override this if you used configure::append_default in your sequence backend.
  template <class Action, class Val>
  void set_action(Action const& a, Val* pval, conf_expr_base const& conf) const {
    sub().sequence_action(a, pval, conf);
  }

  template <class Action>
  bool init_action(Action) const {

    return true;  // return false iff you handled the action completely, without needing the below 4 tree
    // walking callbacks:
  }

  // for Action = each of the action types above - not necessarily a template on Action
  /// init happens before anything:
  template <class Action, class Val>
  void init(Action, Val* pval, conf_expr_base const& conf) const {
  }  // e.g. conf.name(), conf.path_name(), conf.warn_to
  template <class Action, class Val>  // called after init() (still before pval->configure())
  bool init_tree(Action, Val* pval, conf_expr_base const& conf) const {
    return true;
  }
  template <class Action, class Val>
  void leaf_action(Action, Val* pval, conf_expr_base const& conf) const {}
  template <class Action, class Val>  // called after leaves and sub-configs
  void tree_action(Action, Val* pval, conf_expr_base const& conf) const {}

  bool show_tree_header(conf_expr_base const& conf) const { return conf.depth > 0; }


  template <class Val>
  void help_action_open(std::ostream& out, Val* pval, conf_expr_base const& conf) const {
    sub().default_help_action_open(out, pval, conf);
  }


  template <class Action, class Val>  // called after leaves and sub-configs
  void default_print_action_open(Action const& action, Val* pval, conf_expr_base const& conf) const {

    if (is_help(action)) {
      sub().help_action_open(*action.o, pval, get_default_conf(pval, conf));
      return;
    } else if (sub().show_tree_header(conf)) {
      std::ostream& o = *action.o;
      sub().indent(action, conf);
      sub().header_conf(o, conf);
      sub().header_line_end(o, pval, conf);
      o << '\n';
    }
  }

  template <class Val>  // called after leaves and sub-configs
  void help_header_conf(std::ostream& o, Val const& val, conf_expr_base const& conf) const {
    sub().header_conf(o, conf);
  }

  void warning(std::string const& s) const {
    if (warn) warn(s);
  }

  template <class Val>
  void check_leaf_impl(Val* pval, conf_expr_base const& conf)
      const {  // this might have to be called explicitly by child due to name hiding. you can call it inside
    // other actions, too (but check_config is the only place you must)
    conf_opt const& opt = *conf.opt;
    if (opt.is_required_err() && opt.is_init())
      warning("Option is both required and init-value - so init is effectively ignored: --"
              + sub().option_name(conf) + "=" + opt.init->value.str);
  }
  template <class Val>
  void leaf_action(check_config, Val* pval, conf_expr_base const& conf) const {
    check_leaf_impl(pval, conf);
  }

#define FORWARD_BASE_CONFIGURE_ACTION_RET(ret, base, memberfn)                            \
  template <class Action, class Val>                                                      \
  ret memberfn(Action const& a, Val* pval, configure::conf_expr_base const& conf) const { \
    return base::memberfn(a, pval, conf);                                                 \
  }

/// to avoid hiding the default actions defined here when overriding.
#define FORWARD_BASE_CONFIGURE_ACTION(base, memberfn) FORWARD_BASE_CONFIGURE_ACTION_RET(void, base, memberfn)

#define FORWARD_BASE_CONFIGURE_NONCOLLECTION_ACTIONS(base) \
  FORWARD_BASE_CONFIGURE_ACTION_RET(bool, base, init_tree) \
  FORWARD_BASE_CONFIGURE_ACTION(base, tree_action)         \
  FORWARD_BASE_CONFIGURE_ACTION(base, leaf_action)         \
  template <class Action>                                  \
  bool init_action(Action const& a) const {                \
    return base::init_action(a);                           \
  }

#define FORWARD_BASE_CONFIGURE_COLLECTION_ACTIONS(base) \
  FORWARD_BASE_CONFIGURE_ACTION(base, sequence_action)  \
  FORWARD_BASE_CONFIGURE_ACTION(base, map_action)       \
  FORWARD_BASE_CONFIGURE_ACTION(base, set_action)

#define FORWARD_BASE_CONFIGURE_ACTIONS(base)      \
  FORWARD_BASE_CONFIGURE_COLLECTION_ACTIONS(base) \
  FORWARD_BASE_CONFIGURE_NONCOLLECTION_ACTIONS(base)

  template <class Action, class Val>
  void header_line_end(Action const&, Val* pval, conf_expr_base const& conf) const {}
  template <class Val>
  void header_line_end(help_config const& help, Val* pval, conf_expr_base const& conf) const {
    *help.o << prefix_optional(" - ", conf.opt->get_usage_optional());
  }
  std::string unrecognized() const { return "[*]"; }
  enum { SHOW_ROOT_USAGE_CODA = 0 };  // if you want usage to show also at end of root help
  template <class Val>
  void tree_action(help_config const& help, Val* pval, conf_expr_base const& conf) const {
    if (too_verbose(conf)) return;
    conf_opt const& opt = *conf.opt;
    if (opt.allows_unrecognized()) {
      conf_expr_base unrecognized_conf(conf, sub().unrecognized());
      sub().print_conf_val_line(help, opt.allow_unrecognized->help, unrecognized_conf);
    }
    if (SHOW_ROOT_USAGE_CODA && conf.depth == 0) {
      if (opt.usage) *help.o << "\n " << *opt.usage << "\n\n";
    }
  }

  template <class Val>
  void validate(Val* pval, conf_expr_base const& conf) const {
    conf.call_validate(pval);
  }

  // validate doesn't defer to sub() - no need. map/sequence validate is
  // forwarded through defer_*_action
  template <class Val>
  void tree_action(validate_config, Val* pval, conf_expr_base const& conf) const {
    validate(pval, conf);
  }

  template <class Val>
  void leaf_action(validate_config, Val* pval, conf_expr_base const& conf) const {
    validate(pval, conf);
  }

  template <class Action, class Val>
  void map_action(Action const& action, Val* pval, conf_expr_base const& conf) const {
    defer_map_action(action, pval, conf);
  }
  template <class Action, class Val>
  void sequence_action(Action const& action, Val* pval, conf_expr_base const& conf) const {
    defer_sequence_action(action, pval, conf);
  }

  template <class Action, class Val>
  void defer_map_action(Action const& action, Val* pval, conf_expr_base const& conf) const {
    for (typename Val::iterator i = pval->begin(), e = pval->end(); i != e; ++i) {
      configure::conf_expr_base subconf(conf, to_string(i->first));
      configure::configure_action_from_base(sub(), action, &i->second, subconf);
    }
  }

  template <class Action, class Val>
  void defer_sequence_action(Action const& action, Val* pval, conf_expr_base const& conf) const {
    std::size_t key = 0;
    for (typename Val::iterator i = pval->begin(), e = pval->end(); i != e; ++i) {
      configure::conf_expr_base subconf(conf, to_string(key++));
      typedef typename Val::value_type Value;
      typedef typename nonconst_pointer<Value>::type Pointer;
      // cast away const pointer to std::set members
      configure::configure_action_from_base(sub(), action, (Pointer) & *i, subconf);
    }
  }

  void help_action(std::ostream& out, conf_expr_base const& conf, std::string const& suffix,
                   maybe_string const& usage) const {
    if (too_verbose(conf)) return;
    unsigned indent_column = indent_conf_line(out, conf);
    std::string header = sub().option_name(conf);
    if (!header.empty()) header += ":";
    out << header;
    unsigned column = indent_column + (unsigned)header.size();
    out << suffix;
    column += (unsigned)suffix.size();
    if (usage) column = sub().print_usage(out, *usage, column, indent_column);
    out << '\n';
  }

  void help_leaf_action(std::ostream& out, conf_expr_base const& conf) const {
    help_action(out, conf, conf.opt->get_help_suffix_quote(sub()), conf.opt->get_usage());
  }

  template <class Val>
  void default_help_action_open(std::ostream& out, Val* pval, conf_expr_base const& conf) const {
    sub().help_action(out, conf, conf.opt->get_is_suffix(), conf.opt->usage);
  }

  template <class Val>
  void leaf_action(help_config const& help, Val* pval, conf_expr_base const& conf) const {
    help_leaf_action(*help.o, conf);
  }

 protected:
  // if you don't like these defaults, override in your CRTP subclass

  typedef std::string::size_type strpos;


  unsigned print_usage(std::ostream& out, std::string const& usage, unsigned start_column,
                       unsigned indent_column) const {
    if (start_column) {
      out << ' ';
      ++start_column;
    }
    SHOWIF3(CONFEXPR, 10, "print_usage (default)", usage, start_column, indent_column);
    std::string nl_indent(indent_column, ' ');
    std::string hanging(hanging_column_offset(sub()), ' ');
    return graehl::print_indent(out, usage, start_column, max_column(sub()), nl_indent + hanging,
                                nl_indent + "... ");
  }

  void print_name_val(std::ostream& o, std::string const& name, std::string const& val) const {
    o << "--" << name << "=" << val;
  }

  template <class Action>
  unsigned indent(Action const& action, conf_expr_base const& conf) const {
    return indent_conf_line(*action.o, conf);
  }

  void header(std::ostream& o, std::string const& name) const { o << name << ":"; }

  void header_conf(std::ostream& o, conf_expr_base const& conf) const { sub().header(o, header_name(conf)); }

  template <class Action>
  void print_name_val_line(Action const& action, std::string const& name, std::string const& val,
                           conf_expr_base const& conf) const {
    std::ostream& o = *action.o;
    sub().indent(action, conf);
    sub().print_name_val(o, name, val);
    o << '\n';
  }
  std::string option_name(conf_expr_base const& conf) const { return conf.path_name(); }
  std::string header_name(conf_expr_base const& conf) const { return conf.name(); }
  friend struct conf_opt;
  std::string help_quote(std::string const& s) const { return sub().quote(s); }
  std::string quote(std::string const& s) const { return shell_quote(s); }
  template <class Val>
  std::string effective_str(Val const& val, conf_expr_base const& conf) const {
    return sub().quote(conf.opt->get_leaf_value(val));
  }

  template <class Action>
  void print_conf_val_line(Action const& action, std::string const& val, conf_expr_base const& conf) const {
    sub().print_name_val_line(action, sub().option_name(conf), val, conf);
  }

 public:
  template <class Val>
  void tree_action(show_example_config example, Val* pval, conf_expr_base const& conf) const {
    if (too_verbose(conf)) return;
    sub().tree_action_unrecognized(example, pval, conf);
    sub().print_action_close(example, pval, conf);
  }
  template <class Val>
  void tree_action(show_effective_config effective, Val* pval, conf_expr_base const& conf) const {
    if (too_verbose(conf)) return;
    sub().tree_action_unrecognized(effective, pval, conf);
    sub().print_action_close(effective, pval, conf);
  }

  template <class Val>
  void tree_action_unrecognized(show_example_config example, Val* pval, conf_expr_base const& conf) const {
    conf_opt const& opt = *conf.opt;
    if (opt.allows_unrecognized())
      sub().print_name_val_line(example, "[ANY-STRING]: ", "ANY-STRING-VALUE", conf);
  }
  template <class Val>
  void tree_action_unrecognized(show_effective_config effective, Val* pval, conf_expr_base const& conf) const {
    conf_opt const& opt = *conf.opt;
    if (opt.allows_unrecognized()) {
      unrecognized_opts const* unrecognized = opt.allow_unrecognized->unrecognized_storage;
      if (unrecognized)
        for (unrecognized_opts::const_iterator i = unrecognized->begin(), e = unrecognized->end(); i != e; ++i)
          sub().print_name_val_line(effective, i->first, i->second, conf_expr_base(conf, i->first));
    }
  }

  template <class Val>
  void leaf_action(show_effective_config effective, Val* pval, conf_expr_base const& conf) const {
    if (too_verbose(conf)) return;
    std::string s = sub().effective_str(*pval, conf);
    sub().print_conf_val_line(effective, s, conf);
  }
  template <class Val>
  void leaf_action(show_example_config example, Val* pval, conf_expr_base const& conf) const {
    if (too_verbose(conf)) return;
    std::string s = conf.opt->get_eg(*pval);
    if (!s.empty()) sub().print_conf_val_line(example, s, conf);
  }
};

struct config_help_backend : configure_backend_base<config_help_backend> {
  typedef configure_backend_base<config_help_backend> base;
  explicit config_help_backend(string_consumer const& warn, int verbose_max = default_verbose_max)
      : base(warn, verbose_max) {}
};

template <class Action, class RootVal>
void configure_help_action(Action const& action, RootVal* pval, string_consumer const& warn = warn_consumer(),
                           opt_path const& root_path = opt_path()) {
  configure_action(config_help_backend(warn), action, pval, warn, root_path);
}

template <class RootVal>
void check_configure(RootVal* pval, string_consumer const& warn = warn_consumer()) {
  configure_help_action(check_config(), pval, warn);
}

// causes initialization using .init() options
template <class RootVal>
void configure_init(RootVal* pval, string_consumer const& warn = warn_consumer()) {
  configure_help_action(init_config(), pval, warn);
}

/** Writes help text to stream.
 */
template <class RootVal>
void help(std::ostream& out, RootVal* pval, string_consumer const& warn = warn_consumer(),
          opt_path const& root_path = opt_path()) {
  init(pval);
  configure_help_action(help_config(out), pval, warn, root_path);
}

template <class RootVal>
void validate_stored(RootVal* pval, string_consumer const& warn = warn_consumer()) {
  configure_help_action(validate_config(), pval, warn);
}

template <class RootVal>
void show_example(std::ostream& o, RootVal* pval, string_consumer const& warn = warn_consumer()) {
  init(pval);
  configure_help_action(show_example_config(o), pval, warn);
}

template <class RootVal>
void show_effective(std::ostream& o, RootVal* pval, string_consumer const& warn = warn_consumer()) {
  configure_help_action(show_effective_config(o), pval, warn);
}

template <class Val>
struct configure_defaulted {
  Val val;
  Val* operator&() { return &val; }
  operator Val&() { return val; }
  operator Val const&() const { return val; }
  configure_defaulted() { configure_init(&val); }
};


}

#endif
