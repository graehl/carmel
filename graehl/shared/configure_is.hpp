// Copyright 2014 Jonathan Graehl-http://graehl.org/
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

#include <graehl/shared/configure_noop.hpp>
#include <string>
#include <vector>

namespace configure {

struct configure_info {
  std::string is, also, usage;
};


template <class Val>
struct is_expr : noop {
  Val* val;
  configure_info* pinfo;

  is_expr const& is(std::string const& is) const {
    if (pinfo) pinfo->is = is;
    return *this;
  }
  is_expr const& is_also(std::string const& is) const {
    if (pinfo) pinfo->also = is;
    return *this;
  }
  is_expr const& operator()(std::string const& usage) const {
    if (pinfo) pinfo->usage = usage;
    return *this;
  }
  is_expr const& operator()(std::vector<char> const& usage) const {
    if (pinfo) pinfo->usage.assign(usage.begin(), usage.end());
    return *this;
  }
  noop operator()(std::string const& name, void* child) const {
    return noop();
  }
  is_expr const& operator()(char charname) const { return *this; }

  is_expr(Val* val, configure_info* pinfo) : val(val), pinfo(pinfo) {}
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
configure_info configinfo(Val* val) {
  configure_info r;
  is_expr<Val> config(val, &r);
  val->configure(config);
  return r;
}

// TODO: vector of map of <is> from string to <is> etc. - generic type_string.hpp
template <class Val>
std::string configure_is() {
  return configure_info_for<Val>().is;
}

template <class Val>
std::string configure_is(Val * val) {
  return configinfo(val).is;
}

template <class Val>
std::string configure_usage() {
  return configure_info_for<Val>().usage;
}


}

#endif
