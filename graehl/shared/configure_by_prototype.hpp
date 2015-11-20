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

   for configurable objects that with prototypes registered by name.

   for hierarchical configure backends, for `key: val` the key determines which
   prototype is configured with val

   //TODO: needs some work to allow switching backends - likely a type-erased backend / conf_expr<Backend,
   ..., Val>
*/

#ifndef GRAEHL__SHARED__CONFIGURE_BY_PROTOTYPE_HPP
#define GRAEHL__SHARED__CONFIGURE_BY_PROTOTYPE_HPP
#pragma once

#include <graehl/shared/configurable.hpp>
#include <map>

namespace configure {

struct configure_by_standard {};


template <class Backend>
struct configure_by {
  typedef shared_ptr<Backend> BackendPtr;
  typedef shared_ptr<configure_by> ptr;

  virtual ~configure_by() {}

  virtual configure_by* clone() const = 0;
  ptr clone_ptr() const { return ptr(clone()); }

#define CONFIGURE__FOR_ACTIONS(x) x(init) x(validate) x(help) x(show_example) x(show_effective) x(store)

#define CONFIGURE__CONFIGURE_FOR_CONF_EXPR_TYPE(init)                             \
  typedef conf_expr<Backend, init##_config, configure_by> init;                   \
  typedef conf_expr<config_help_backend, init##_config, configure_by> init##_std; \
  virtual void configure(init& config) = 0;                                       \
  virtual void configure(init##_std& config) = 0;

  CONFIGURE__FOR_ACTIONS(CONFIGURE__CONFIGURE_FOR_CONF_EXPR_TYPE)
};

template <class Val, class Backend>
struct configure_by_prototype : configure_by<Backend> {

  typedef shared_ptr<Prototype> prototype_ptr;
  prototype_ptr proto;
  configure_backend::ptr backend;

  configure_by_prototype() {}

  configure_by_prototype(Prototype* proto) : proto(proto, null_deleter()) {}
  explicit configure_by_prototype(prototype_ptr const& proto) : proto(proto) {}

  configure_by_prototype* clone_self() const {
    return new configure_by_prototype(new Prototype(*proto), backend);
  }

  typedef configure_by<Backend> ConfigureBy;

  ConfigureBy* clone() const override { return clone_self(); }

#define CONFIGURE__CONFIGURE_DYNMIC_FOR_CONF_EXPR_TYPE(init)          \
  void configure(init& config) override { proto->configure(config); } \
  void configure(init##_std& config) override { proto->configure(config); }


  CONFIGURE__FOR_ACTIONS(CONFIGURE__CONFIGURE_DYNMIC_FOR_CONF_EXPR_TYPE)
};

enum { kSingleAllowedInstance = 1, kUnlimitedInstances = -1 };


/// Base arg allows separate prototypes registries and to
/// return a CRTP Base * instead of just configure_by<Backend>;

/// Backend is required because there's no
/// configure_backend_any yet
template <class Backend, class Base = configure_by<Backend> >
struct configure_dynamic {
  typedef configure_by<Backend> ConfigureBy;
  typedef shared_ptr<Configurable> ConfigureByPtr;

  typedef std::map<std::string, ConfigureByPtr> Prototypes;
  static std::map<std::string, ConfigureByPtr> prototypes;

  typedef Prototypes::value_type TypePrototype;
  static void prototypePtr(std::string const& type, ConfigureByPtr const& proto) {
    prototypes.insert(TypePrototype(type, proto));
  }
  template <class Proto>
  static void prototype(std::string const& type, Proto const& proto) {
    prototypes.insert(TypePrototype(type, new_configurable_ptr(proto)));
  }
  Prototypes instances;
  friend inline void validate(configure_dynamic& x) { x.validate(); }
  void validate() {
    if (instances.size() > kMaxAllowedInstances)
      throw config_exception("configure_dynamic must have at most " + to_string(kMaxAllowedInstances)
                             + " type: val defined");
  }
  shared_ptr<Base> value;  // the first instance goes here
  std::string type;
  int limit;
  configure_dynamic(int limit = kUnlimitedInstances) : limit(limit) {
    for (Prototypes::const_iterator i = prototypes.begin(), e = prototypes.end(); i != e; ++i) {
      std::string const& type = i->first;
      TypePrototype typeProto(i->first, i->second->clone_ptr());
      instances.insert(typeProto);
    }
  }

  bool empty() const { return instances.empty(); }
  std::size_t size() const { return instances.size(); }
  operator bool() const { return !empty(); }

  void template <class Config>
  void configure(Config& config) {
    config.is("configure_dynamic");
    config.limit(limit);
    for (Prototypes::const_iterator i = instancse.begin(), e = instances.end(); i != e; ++i)
      config(i->first, i->second.get());  // will pick up usage from user configure_by type
  }

  bool contains(std::string const& type) const { return instances.find(type) != instances.end(); }

  typedef<Concrete> Concrete& getRef(std::string const& type) const {
    assert(contains(type));
    return *instances.find(type)->second;
  }

  typedef<Concrete> shared_ptr<Concrete> get(std::string const& type) const {
    shared_ptr<Concrete> r;
    get(type, r);
    return r;
  }

  typedef<Concrete> shared_ptr<Concrete>& get(std::string const& type, shared_ptr<Concrete>& r) const {
    typedef Prototypes::const_iterator i = instances.find(type);
    if (i == end())
      r.reset();
    else
      r = dynamic_pointer_cast<Concrete>(i->second);  // could be static_pointer_cast
    return r;
  }
};

template <class Backend, class Base>
typename configure_dynamic<Backend, Base>::Prototypes configure_dynamic<Backend, Base>::prototypes;


}

#endif
