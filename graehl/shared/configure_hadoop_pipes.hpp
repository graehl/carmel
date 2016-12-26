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

    like configure_program_options, but compensates for defect of
    HadoopPipes::JobConf - you can't enumerate all the keys that have been set:

    struct JobConf {
      virtual bool hasKey(const std::string& key) const = 0;
      virtual const std::string& get(const std::string& key) const = 0;
    };

*/

#ifndef GRAEHL_SHARED__CONFIGURE_HADOOP_PIPES_HPP
#define GRAEHL_SHARED__CONFIGURE_HADOOP_PIPES_HPP
#pragma once

#include <graehl/shared/warn.hpp>
#include <graehl/shared/configure.hpp>
#if GRAEHL_TEST
#include <graehl/shared/string_builder.hpp>
namespace HadoopPipes {
struct JobConf {
  bool hasKey(std::string const& k) const { return k.find('!') == std::string::npos; }
  std::string get(std::string const& k) const { return graehl::to_string(k.size()); }
};
}
#else
#include <hadoop/Pipes.hh>
#endif
#include <sstream>

namespace configure {

// TODO: add non-leaf help info so hadoop_pipes formatted descriptions have at least one-level-nesting
// headers. or does usage(str) do that already?
struct configure_hadoop_pipes : configure_backend_base<configure_hadoop_pipes> {
  typedef configure_backend_base<configure_hadoop_pipes> base;
  FORWARD_BASE_CONFIGURE_ACTIONS(base)
  typedef HadoopPipes::JobConf const* JobConfP;
  JobConfP jobconf_;

  configure_hadoop_pipes(configure_hadoop_pipes const& o) = default;
  configure_hadoop_pipes(JobConfP jobconf, string_consumer const& warn_to, int verbose_max = default_verbose_max)
      : base(warn_to, verbose_max), jobconf_(jobconf) {}

  template <class Val>
  void leaf_action(store_config, Val* pval, conf_expr_base const& conf) const {
    std::string const& pathname = conf.path_name();
    auto& opt = *conf.opt;
    if (jobconf_->hasKey(pathname))
      opt.apply_string_value(jobconf_->get(pathname), pval, pathname, warn);
    else if (opt.is_required_err())
      missing(pathname, false);
    else if (opt.is_required_warn())
      missing(pathname, true);
  }

  static inline void missing(graehl::string_consumer const& o, std::string const& name, bool warn_only) {
    std::string complaint("missing configuration key ");
    o(complaint += name);
    if (!warn_only) throw config_exception(complaint);
  }

  void missing(std::string const& name, bool warn_only) const { missing(this->warn, name, warn_only); }


  template <class Val>
  void tree_action(init_config, Val* pval, conf_expr_base const& conf) const {}

  bool init_action(init_config) const { return true; }

  bool init_action(store_config) const { return true; }

  bool init_action(help_config const& c) const { return false; }

  template <class Val>
  void leaf_action(show_example_config a, Val* pval, conf_expr_base const& conf) const {
    base::leaf_action(a, pval, conf);
  }
};

template <class RootVal>
void hadoop_configure(HadoopPipes::JobConf const* jobconf, RootVal* val,
                      string_consumer const& warn = graehl::warn_consumer(), bool init = true,
                      bool validate = true) {
  if (init) configure::configure_init(val, warn);
  configure_hadoop_pipes conf(jobconf, warn, default_verbose_max);
  configure_action(conf, store_config(), val, warn);
  if (validate) configure::validate_stored(val, warn);
}
}


#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>

struct HadoopThing {
  int x;
  std::string y;
  template <class Config>
  void configure(Config& config) {
    config.is("HadoopThing");
    config("blah");
    config("x", &x).init(0);
    config("!y", &y).init("y");
  }
};

struct HadoopThing2 {
  HadoopThing a, b;
  float z;
  template <class Config>
  void configure(Config& config) {
    config.is("HadoopThing2");
    config("!a", &a);
    config("b", &b);
    config("!z", &z).init(0.5);
  }
};

BOOST_AUTO_TEST_CASE(configure_program_options_test) {
  HadoopPipes::JobConf conf;
  HadoopThing2 t;
  configure::hadoop_configure(&conf, &t);
  BOOST_CHECK_EQUAL(t.z, 0.5);
  BOOST_CHECK_EQUAL(t.a.x, 0);
  BOOST_CHECK_EQUAL(t.a.y, "y");
  BOOST_CHECK_EQUAL(t.b.y, "y");
  BOOST_CHECK_EQUAL(t.b.x, 3);
  BOOST_CHECK_EQUAL(t.b.y, "y");
  configure::hadoop_configure(&conf, &t.a);
  BOOST_CHECK_EQUAL(t.a.x, 1);
  BOOST_CHECK_EQUAL(t.a.y, "y");
}
#endif

#endif
