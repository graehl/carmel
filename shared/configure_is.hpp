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

    stub implementation of the configure protocol (see configure.hpp) that ignores
    all operations except .is(str), which is saved to allow std::string configure_is(configurable)
*/

#ifndef CONFIGURE_IS_JG_2013_05_23_HPP
#define CONFIGURE_IS_JG_2013_05_23_HPP
#pragma once

#include <string>

namespace configure {

struct configure_info {
  std::string is, also, usage;
};


template <class Val>
struct is_expr
{
  is_expr const& is(std::string const& is) const { if (pinfo) pinfo->is = is; return *this; }
  is_expr const& is_also(std::string const& is) const { if (pinfo) pinfo->also = is; return *this; }
  is_expr const& operator()(std::string const& usage) const { if (pinfo) pinfo->usage = usage; return *this; }

  template <class Child>
  is_expr<Child> operator()(std::string const& name, Child *child) const {
    return is_expr<Child>(child, 0);
  }
  template <class V2>
  is_expr const& init(V2 const& v2) const {
    return *this;
  }
  template <class V2>
  is_expr const& init(bool enable, V2 const& v2) const
  {
    return init(v2);
  }
  // similar concept to implicit except that you have the --key implicit true, and --no-key implicit false
  is_expr const& flag(bool is_to = false) const {
    return init(is_to);
  }
  template <class V2>
  is_expr const& inits(V2 const& v2) {
    return *this;
  }

  is_expr(Val *val, configure_info *pinfo)
      : val(val), pinfo(pinfo)
  {}

  Val *val;
  configure_info *pinfo;

  /// the rest of the protocol are all no-ops

  is_expr const& null_ok(Val const& val = Val()) const { return *this; }
  is_expr const& alias() const { return *this; }
  template <class V2>
  is_expr const& eg(V2 const& eg) const { return *this; }
  is_expr const& operator()(char charname) const { return *this; }
  is_expr const& deprecate(std::string const& info = "", bool deprecated = true) const { return *this; }
  is_expr const& is_default(bool enable = true) const { return *this; }
  is_expr const& todo(bool enable = true) const { return *this; }
  is_expr const& verbose(int verbosity = 1) const { return *this; }
  is_expr const& positional(bool enable = true, int max = 1) const { return *this; }
  template <class unrecognized_opts>
  is_expr const& allow_unrecognized(bool enable = true, bool warn = false, unrecognized_opts *unrecognized_storage = 0) const
  { return *this; }
  is_expr const& require(bool enable = true, bool just_warn = false) const { return *this; }
  is_expr const& desire(bool enable = true) const { return *this; }
  template <class V2>
  is_expr const& implicit(bool enable, V2 const& v2) const { return *this; }
  template <class V2>
  is_expr const& implicit(V2 const& v2) const { return *this; }
  is_expr const& self_init(bool enable = true) const { return *this; }
  template <class V2>
  is_expr const& validate(V2 const& validator) const { return *this; }
};

template <class Val>
struct configure_info_for : configure_info {
  Val val;
  configure_info_for() {
    is_expr<Val> config(&val, this);
    val.configure(config);
  }
};

template <class Val>
configure_info configinfo(Val *val) {
  configure_info r;
  is_expr<Val> config(val, &r);
  val->configure(config);
  return r;
}

template <class Val>
std::string configure_is() {
  return configure_info_for<Val>().is;
}

template <class Val>
std::string configure_usage() {
  return configure_info_for<Val>().usage;
}


}

#endif
