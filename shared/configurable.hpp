/** \file

    used primarily by cmdline_main, allowing you to declare additional
    configurable option structs - see configure_by_prototype for a more flexible
    type erasure
*/

#ifndef GRAEHL__SHARED__CONFIGURABLE_HPP
#define GRAEHL__SHARED__CONFIGURABLE_HPP
#pragma once

#include <graehl/shared/configure.hpp>
#include <graehl/shared/null_deleter.hpp>
#include <boost/make_shared.hpp>

namespace configure {

using graehl::null_deleter;

/** Interface for configurable objects of unspecified type. The configurable
   either is the configured Val, or has a pointer to it. In other words, this is
   for type-erased closures over the Backend and Val template arguments to all
the configure actions. */
struct configurable
{
  virtual configure_backend::ptr get_backend() const = 0;
  virtual void set_backend(configure_backend::ptr const&) = 0;
  // simple factory: default constructed prototype object that's cloned then configured
  virtual void init(string_consumer const& warn, opt_path const& root_path = opt_path()) const = 0;
  virtual void store(string_consumer const& warn, opt_path const& root_path = opt_path()) const = 0;
  virtual void validate(string_consumer const& warn, opt_path const& root_path = opt_path()) const = 0;
  virtual void help(std::ostream &o, string_consumer const& warn, opt_path const& root_path = opt_path()) const = 0;
  virtual void standard_help(std::ostream &o, string_consumer const& warn, opt_path const& root_path) const = 0;
  virtual void effective(std::ostream &o, string_consumer const& warn, opt_path const& root_path = opt_path()) const = 0;
  virtual void example(std::ostream &o, string_consumer const& warn, opt_path const& root_path = opt_path()) const = 0;
  void init_store(string_consumer const& warn, opt_path const& root_path = opt_path())
  {
    init(warn, root_path);
    store(warn, root_path);
  }
  void init_example(std::ostream &o, string_consumer const& warn, opt_path const& root_path = opt_path())
  {
    init(warn, root_path);
    example(o, warn, root_path);
  }
  virtual ~configurable() {}
  typedef boost::shared_ptr<configurable> ptr;
};

template <class Prototype, class Backend>
struct configure_prototype : configurable
{
  typedef boost::shared_ptr<Prototype> prototype_ptr;
  prototype_ptr proto;
  configure_backend::ptr backend;

  configure_prototype() {}
  configure_prototype(Prototype *proto, Backend const& backend)
      : proto(proto, null_deleter()), backend(boost::make_shared<Backend>(backend)) {}
  // backends are intended to be lightweight-copy value-semantic options so
  // might be stack allocated - thus the make_shared copy construction instead
  // of null_deleter
  configure_prototype(prototype_ptr const& proto, Backend const& backend)
      : proto(proto), backend(boost::make_shared<Backend>(backend)) {}

  configure_prototype(Prototype *proto, configure_backend::ptr const& backend)
      : proto(proto, null_deleter()), backend(backend) {}
  configure_prototype(prototype_ptr const& proto, configure_backend::ptr const& backend)
      : proto(proto), backend(backend) {}

  configure_prototype(Prototype *proto)
      : proto(proto, null_deleter()) {}
  explicit configure_prototype(prototype_ptr const& proto)
      : proto(proto) {}

  Backend const& backend_impl() const {
    return dynamic_cast<Backend const&>(*backend); // could be static
  }

  Prototype *prototype() const {
    return proto.get();
  }


  virtual void set_backend(configure_backend::ptr const& p) { backend = p; }
  virtual configure_backend::ptr get_backend() const { return backend; }
  virtual void init(string_consumer const& warn, opt_path const& root_path) const
  {
    configure_action(backend_impl(), init_config(), prototype(), warn, root_path);
  }
  virtual void store(string_consumer const& warn, opt_path const& root_path) const
  {
    configure_action(backend_impl(), store_config(), prototype(), warn, root_path);
  }
  virtual void validate(string_consumer const& warn, opt_path const& root_path) const
  {
    configure_action(backend_impl(), validate_config(), prototype(), warn, root_path);
  }
  virtual void help(std::ostream &o, string_consumer const& warn, opt_path const& root_path) const {
    configure_action(backend_impl(), help_config(o), prototype(), warn, root_path);
  }
  virtual void standard_help(std::ostream &o, string_consumer const& warn, opt_path const& root_path) const {
    configure::help(o, prototype(), warn, root_path);
  }
  virtual void effective(std::ostream &o, string_consumer const& warn, opt_path const& root_path) const {
    configure_action(backend_impl(), show_effective_config(o), prototype(), warn, root_path);
  }
  virtual void example(std::ostream &o, string_consumer const& warn, opt_path const& root_path) const {
    configure_action(backend_impl(), show_example_config(o), prototype(), warn, root_path);
  }
};

template <class Prototype, class Backend>
configure_prototype<Prototype, Backend> *new_configurable(Prototype *prototype, Backend const& backend)
{
  return new configure_prototype<Prototype, Backend>(prototype, backend);
}

//TODO: configure_backend_any - would have to save expression tree from configure templates then execute it later?.

/** Type erasure of a configurable (built with configure_prototype). */
struct configure_any : configurable
{
  configurable::ptr p;
  opt_path prefix; // root for this thing
  std::string help_prefix;

  configure_any() {}

  /** Given a backend and a value, creates a configure_prototype for you. */
  template <class RootVal, class Backend>
  configure_any(RootVal *pval, Backend const& backendv, opt_path const& prefix, std::string const& help_prefix="")
      : prefix(prefix), help_prefix(help_prefix)
  {
    typedef configure_prototype<RootVal, Backend> Impl;
    Impl *impl = new Impl(pval, backendv);
    p.reset(impl);
  }


  virtual void set_backend(configure_backend::ptr const& backend) { p->set_backend(backend); }
  virtual configure_backend::ptr get_backend() const { return p->get_backend(); }

  template <class Backend>
  Backend const& backend()
  {
    return dynamic_cast<Backend const&>(*p->get_backend());
  }


  virtual void init(string_consumer const& warn) const
  {
    p->init(warn, prefix);
  }
  virtual void store(string_consumer const& warn) const
  {
    p->store(warn, prefix);
  }
  virtual void validate(string_consumer const& warn) const {
    p->validate(warn, prefix);
  }
  virtual void help(std::ostream &o, string_consumer const& warn) const {
    o << help_prefix;
    p->help(o, warn, prefix);
  }
  virtual void standard_help(std::ostream &o, string_consumer const& warn) const {
    o << help_prefix;
    p->standard_help(o, warn, prefix);
  }
  virtual void effective(std::ostream &o, string_consumer const& warn) const {
    p->effective(o, warn, prefix);
  }
  virtual void example(std::ostream &o, string_consumer const& warn) const {
    p->example(o, warn, prefix);
  }

  virtual void validate(string_consumer const& warn, opt_path const& root_path) const {
    p->validate(warn, root_path);
  }
  virtual void init(string_consumer const& warn, opt_path const& root_path) const
  {
    p->init(warn, root_path);
  }
  virtual void store(string_consumer const& warn, opt_path const& root_path) const
  {
    p->store(warn, root_path);
  }
  virtual void help(std::ostream &o, string_consumer const& warn, opt_path const& root_path) const {
    o << help_prefix;
    p->help(o, warn, root_path);
  }
  virtual void standard_help(std::ostream &o, string_consumer const& warn, opt_path const& root_path) const {
    o << help_prefix;
    p->standard_help(o, warn, root_path);
  }
  virtual void effective(std::ostream &o, string_consumer const& warn, opt_path const& root_path) const {
    p->effective(o, warn, root_path);
  }
  virtual void example(std::ostream &o, string_consumer const& warn, opt_path const& root_path) const {
    p->example(o, warn, root_path);
  }
};

inline opt_path concat(opt_path base, opt_path const& extend)
{
  base.insert(base.end(), extend.begin(), extend.end());
  return base;
}

/** a configurable which is a list of configurables. */
struct configure_list : configurable
{
  typedef std::vector<configure_any> configurables;
  configurables confs;

  virtual void set_backend(configure_backend::ptr const& backend)
  {
    for (configurables::iterator p = confs.begin(), e = confs.end(); p!=e; ++p)
      p->set_backend(backend);
  }

  virtual configure_backend::ptr get_backend() const {
    for (configurables::const_iterator p = confs.begin(), e = confs.end(); p!=e; ++p)
      return p->get_backend();
    return configure_backend::ptr();
  }

  virtual void init(string_consumer const& warn, opt_path const& root_path = opt_path()) const
  {
    for (configurables::const_iterator p = confs.begin(), e = confs.end(); p!=e; ++p)
      p->init(warn, concat(root_path, p->prefix));
  }
  virtual void store(string_consumer const& warn, opt_path const& root_path = opt_path()) const
  {
    for (configurables::const_iterator p = confs.begin(), e = confs.end(); p!=e; ++p)
      p->store(warn, concat(root_path, p->prefix));
  }
  virtual void validate(string_consumer const& warn, opt_path const& root_path = opt_path()) const
  {
    for (configurables::const_iterator p = confs.begin(), e = confs.end(); p!=e; ++p)
      p->validate(warn, concat(root_path, p->prefix));
  }
  virtual void help(std::ostream &o, string_consumer const& warn, opt_path const& root_path = opt_path()) const {
    for (configurables::const_iterator p = confs.begin(), e = confs.end(); p!=e; ++p)
      p->help(o, warn, concat(root_path, p->prefix));
  }
  virtual void standard_help(std::ostream &o, string_consumer const& warn, opt_path const& root_path = opt_path()) const {
    for (configurables::const_iterator p = confs.begin(), e = confs.end(); p!=e; ++p)
      p->standard_help(o, warn, concat(root_path, p->prefix));
  }
  virtual void effective(std::ostream &o, string_consumer const& warn, opt_path const& root_path = opt_path()) const {
    for (configurables::const_iterator p = confs.begin(), e = confs.end(); p!=e; ++p)
      p->effective(o, warn, concat(root_path, p->prefix));
  }
  virtual void example(std::ostream &o, string_consumer const& warn, opt_path const& root_path = opt_path()) const {
    for (configurables::const_iterator p = confs.begin(), e = confs.end(); p!=e; ++p)
      p->example(o, warn, concat(root_path, p->prefix));
  }
  void add(configure_any const& c)
  {
    confs.push_back(c);
  }
  template <class RootVal, class Backend>
  void add(RootVal *pval, Backend const& backend, opt_path const& prefix = opt_path(), std::string const& help_prefix="")
  {
    confs.push_back(configure_any(pval, backend, prefix, help_prefix));
  }
};


}

#endif
