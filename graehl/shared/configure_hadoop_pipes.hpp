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

#include <graehl/shared/configure.hpp>
#include <graehl/shared/string_builder.hpp>
#include <graehl/shared/warn.hpp>
#include <algorithm>
#include <sstream>

#if GRAEHL_TEST
namespace HadoopPipes {
struct JobConf {
  mutable std::string str;
  virtual bool hasKey(std::string const& k) const { return k.find('!') == std::string::npos; }
  virtual std::string const& get(std::string const& k) const {
    str = graehl::to_string(k.size());
    return str;
  }
  virtual bool getBoolean(std::string const& k) const { return graehl::string_to<bool>(get(k)); }
  virtual float getFloat(std::string const& k) const { return graehl::string_to<float>(get(k)); }
  virtual int getInt(std::string const& k) const { return graehl::string_to<int>(get(k)); }
};
}
#else
#include <hadoop/Pipes.hh>
#endif

namespace configure {

struct MapAsHadoopJobConf : HadoopPipes::JobConf {
  typedef std::map<std::string, std::string> Map;
  Map const* map_;
  MapAsHadoopJobConf(Map const* map = 0) : map_(map) {}
  bool hasKey(std::string const& k) const override { return map_->find(k) != map_->end(); }
  std::string const& get(std::string const& k) const override { return map_->find(k)->second; }
  bool getBoolean(std::string const& k) const override { return graehl::string_to<bool>(get(k)); }
  float getFloat(std::string const& k) const override { return graehl::string_to<float>(get(k)); }
  int getInt(std::string const& k) const override { return graehl::string_to<int>(get(k)); }
};

// TODO: add non-leaf help info so hadoop_pipes formatted descriptions have at least one-level-nesting
// headers. or does usage(str) do that already?
struct configure_hadoop_pipes : configure_backend_base<configure_hadoop_pipes> {
  typedef configure_backend_base<configure_hadoop_pipes> base;
  FORWARD_BASE_CONFIGURE_ACTIONS(base)
  typedef HadoopPipes::JobConf const* JobConfP;
  JobConfP jobconf_;
  bool tryMinusForUnderscore_;

  configure_hadoop_pipes(configure_hadoop_pipes const& o) = default;
  configure_hadoop_pipes(JobConfP jobconf, string_consumer const& warn_to = graehl::warn_consumer(),
                         int verbose_max = default_verbose_max, bool tryMinusForUnderscore = true)
      : base(warn_to, verbose_max), jobconf_(jobconf), tryMinusForUnderscore_(tryMinusForUnderscore) {}

  std::string const* get(std::string const& key) const {
    return hasKey(key) ? &jobconf_->get(key) : getEither(key, '-', '_');
  }

  template <class Val>
  void leaf_action(store_config, Val* pval, conf_expr_base const& conf) const {
    std::string const& pathname = conf.path_name();
    auto& opt = *conf.opt;
    if (std::string const* v = get(pathname))
      opt.apply_string_value(*v, pval, pathname, warn);
    else if (opt.is_required_err())
      missing(pathname, false);
    else if (opt.is_required_warn())
      missing(pathname, true);
  }

  template <class Val>
  void sequence_action(store_config c, Val* pval, conf_expr_base const& conf) const {
    leaf_action(c, pval, conf);
  }

  static inline void missing(graehl::string_consumer const& o, std::string const& name, bool warn_only) {
    std::string complaint("missing environment key ");
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

 private:
  bool hasKey(std::string const& key) const {
    bool r = jobconf_->hasKey(key);
    if (this->warn) {
      graehl::string_builder b;
      b("environment key '")(key);
      if (r)
        b("' => '")(jobconf_->get(key))('\'');
      else
        b("' missing.");
      this->warn(b);
    }
    return r;
  }

  std::string const* getAlternate(std::string const& key, char from, char to) const {
    if (key.find(from) != std::string::npos) {
      std::string k2 = key;
      std::replace(k2.begin(), k2.end(), from, to);
      if (hasKey(k2)) return &jobconf_->get(k2);
    }
    return 0;
  }

  std::string const* getEither(std::string const& key, char a, char b) const {
    std::string const* r = getAlternate(key, a, b);
    return r ? r : getAlternate(key, b, a);
  }
};

template <class RootVal>
void hadoop_configure(HadoopPipes::JobConf const* jobconf, RootVal* val, bool tryMinusForUnderscore = true,
                      string_consumer const& warn = graehl::warn_consumer(), bool init = true,
                      bool validate = true) {
  if (init) configure::configure_init(val, warn);
  configure_hadoop_pipes conf(jobconf, warn, default_verbose_max, tryMinusForUnderscore);
  configure_action(conf, store_config(), val, warn);
  if (validate) configure::validate_stored(val, warn);
}

template <class RootVal>
void map_configure(HadoopPipes::JobConf const& jobconf, RootVal* val, bool tryMinusForUnderscore = true,
                   string_consumer const& warn = graehl::warn_consumer(), bool init = true, bool validate = true) {
  hadoop_configure(&jobconf, val, tryMinusForUnderscore, warn, init, validate);
}

template <class RootVal>
void map_configure(std::map<std::string, std::string> const& map, RootVal* val,
                   bool tryMinusForUnderscore = true, string_consumer const& warn = graehl::warn_consumer(),
                   bool init = true, bool validate = true) {
  MapAsHadoopJobConf jobconf{&map};
  hadoop_configure(&jobconf, val, tryMinusForUnderscore, warn, init, validate);
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
    config("x_x", &x).init(0);
    config("!y", &y).init("y");
  }
};

struct HadoopThing2 {
  HadoopThing a, b;
  float z;
  template <class Config>
  void configure(Config& config) {
    config.is("HadoopThing2");
    config("a", &a);
    config("b-c", &b);
    config("!z", &z).init(0.5);
  }
};

BOOST_AUTO_TEST_CASE(configure_hadoop_pipes) {
  HadoopPipes::JobConf conf;
  HadoopThing2 t;
  configure::hadoop_configure(&conf, &t);
  BOOST_CHECK_EQUAL(t.z, 0.5);
  BOOST_CHECK_EQUAL(t.a.x, 5);
  BOOST_CHECK_EQUAL(t.a.y, "y");
  BOOST_CHECK_EQUAL(t.b.y, "y");
  BOOST_CHECK_EQUAL(t.b.x, 7);
  BOOST_CHECK_EQUAL(t.b.y, "y");
  configure::hadoop_configure(&conf, &t.a);
  BOOST_CHECK_EQUAL(t.a.x, 3);
  BOOST_CHECK_EQUAL(t.a.y, "y");
}

BOOST_AUTO_TEST_CASE(configure_hadoop_pipes_map) {
  std::map<std::string, std::string> map{{"!z", "0.25"}, {"a.x-x", "1"}, {"b-c.!y", "by"}};
  HadoopThing2 t;
  configure::map_configure(map, &t);
  BOOST_CHECK_EQUAL(t.z, 0.25);
  BOOST_CHECK_EQUAL(t.a.x, 1);
  BOOST_CHECK_EQUAL(t.a.y, "y");
  BOOST_CHECK_EQUAL(t.b.y, "by");
  BOOST_CHECK_EQUAL(t.b.x, 0);
}

BOOST_AUTO_TEST_CASE(configure_hadoop_pipes_map2) {
  std::map<std::string, std::string> map{{"!z", "0.25"}, {"a.x_x", "1"}, {"b_c.!y", "by"}};
  HadoopThing2 t;
  configure::map_configure(map, &t);
  BOOST_CHECK_EQUAL(t.z, 0.25);
  BOOST_CHECK_EQUAL(t.a.x, 1);
  BOOST_CHECK_EQUAL(t.a.y, "y");
  BOOST_CHECK_EQUAL(t.b.y, "by");
  BOOST_CHECK_EQUAL(t.b.x, 0);
}

#endif

#endif
