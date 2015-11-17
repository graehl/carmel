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

    a template library for specifying config structs (see end of file for an
    example)

  TODO: skip boost program options entirely? it doesn't offer much that we can't directly with our own parser.
  also, slight difficulties using program options naively to:

  warn_unk (actually, depending on how you run PO, maybe we can get list of unparsed things, but then mapping
  them to hierarchy is just as hard as having our own parser)

  short names prefering parent (decide during init, which must be called before store)

*/

#ifndef GRAEHL_SHARED__CONFIGURE_PROGRAM_OPTIONS_HPP
#define GRAEHL_SHARED__CONFIGURE_PROGRAM_OPTIONS_HPP
#pragma once

//#define GRAEHL_CONFIGURE_SAMPLE_MAIN 1
#ifndef GRAEHL_CONFIGURE_SAMPLE_MAIN
#define GRAEHL_CONFIGURE_SAMPLE_MAIN 0
#endif

#include <graehl/shared/configure.hpp>
#include <graehl/shared/program_options.hpp>
#include <sstream>

namespace configure {

struct program_options_exec : boost::noncopyable {
  bool allow_unrecognized_opts;
  string_consumer warn;
  program_options_exec(string_consumer const& warn, std::string const& caption)
      : allow_unrecognized_opts(true), warn(warn), opt_desc(caption), argv0("main"), finished_store(false) {}

  /**
     TODO: directly generate regular options_description, use config facility
     for printing instead
  */
  typedef graehl::printable_options_description<std::ostream> ostream_options_description;


  boost::program_options::positional_options_description positional;
  boost::program_options::variables_map vm;  // TODO!
  ostream_options_description opt_desc;
  std::set<opt_path> specified_options;  // TODO: needed to check duplicates?
  typedef conf_opt::allow_unrecognized_args allow_unrecognized_args;
  typedef std::map<std::string, allow_unrecognized_args> unrecognized_map;
  unrecognized_map allow_unk_paths;  // TODO: most specific parent so you can have unrecognized nested stuff?
  typedef std::vector<std::string> strings;
  strings args;
  strings unrecognized_options;
  strings warn_missing;  // option names to warn about if value not present
  strings error_missing;  // truly required options
  std::string argv0;
  void set_main_argv(int argc, char** v)  // v[0] is program name and so ignored
  {
    argv0 = argc ? v[0] : "NO-ARGV";
    args.clear();
    for (int i = 1; i < argc; ++i) args.push_back(v[i]);
  }
  bool maybe_po_help(std::ostream& o) const {
    if (is_help()) {
      show_po_help(o);
      return true;
    }
    return false;
  }
  void show_po_help(std::ostream& o) const { o << opt_desc << '\n'; }
  bool is_help() const  // after args set by e.g. set_main_argv
  {
    for (strings::const_iterator i = args.begin(), e = args.end(); i != e; ++i)
      if (*i == "-h" || *i == "--help") return true;
    return false;
  }

  // TODO: allow program_options config files too?
  void check_required(graehl::string_consumer const& o, strings const& names, bool warn_only) {
    for (strings::const_iterator i = names.begin(), e = names.end(); i != e; ++i) {
      std::string const& name = *i;
      if (!vm.count(name) || vm[name].defaulted()) {
        // TODO: test
        std::string complaint("missing configuration key ");
        complaint += *i;
        o(complaint);
        if (!warn_only) throw config_exception(complaint);
      }
    }
  }
  bool finished_store;
  void finish_store() {
    if (finished_store) return;
    finished_store = true;
    using namespace boost::program_options;
    parsed_options po = opt_desc.parse_options(args, &positional, &unrecognized_options,
                                               allow_unrecognized_opts, false, argv0);
    store(po, vm);
    notify(vm);
    check_required(warn, warn_missing, true);
    check_required(warn, error_missing, false);
    check_unrecognized();
  }

  void allow_unrecognized(std::string const& pathname, allow_unrecognized_args const& args) {
    // TODO: test
    SHOWIF2(CONFEXPR, 1, "allow_unrecognized", pathname, args);
    if (args.enable) {
      if (pathname.empty()) {
        SHOWIF1(CONFEXPR, 1, "allow_unrecognized root", pathname);
        allow_unrecognized_opts = true;
      }
      allow_unk_paths[pathname] = args;
    }
  }
  std::string unrecognized_complaint(std::string arg, std::string parent, std::string prefix = "") {
    std::string complaint(prefix);
    (((complaint += arg) += " is an unknown option and parent ") += parent) += " doesn't like those!";
    warning(complaint);
    return complaint;
  }
  void warning(std::string const& s) const {
    if (warn) warn(s);
  }

  void check_unrecognized() {
    using namespace std;
    for (size_t i = 0, n = unrecognized_options.size(); i != n; ++i) {
      // TODO: test
      string const& arg = unrecognized_options[i];
      if (arg.size() < 2 || arg[0] != '-' || arg[1] != '-')
        throw config_exception(
            "ERROR: unrecognized command line argument is not of the form --key[=val] if this is an option "
            "value then use --key=val rather than --key val.");
      string::const_iterator start = arg.begin();
      string::size_type equals = arg.find('=');
      bool no_val = equals == string::npos;
      string key(start + 2, no_val ? arg.end() : start + equals);
      std::string const& parent = parent_option_name(key);
      SHOWIF2(CONFEXPR, 1, "allow unk?", key, parent);
      unrecognized_map::const_iterator f = allow_unk_paths.find(parent);
      if (f == allow_unk_paths.end()) throw config_exception(unrecognized_complaint(arg, parent, "ERROR: "));
      allow_unrecognized_args const& allow = f->second;
      std::string val(no_val ? start : start + equals + 1, no_val ? start : arg.end());
      SHOWIF3(CONFEXPR, 1, "allow unk complain?", key, val, allow);
      if (allow.warn) unrecognized_complaint(arg, parent);
      unrecognized_opts* store = allow.unrecognized_storage;
      if (store) (*store)[key] = val;
    }
  }

  // TODO: short names override by parent? or enable parent only?
  void print(std::ostream& o) const {
    o << "&" << &opt_desc << ":\n";
    opt_desc.print(o, vm, graehl::SHOW_ALL);
  }
  friend std::ostream& operator<<(std::ostream& o, program_options_exec const& self) {
    self.print(o);
    return o;
  }
};

typedef shared_ptr<program_options_exec> program_options_exec_ptr;
struct program_options_exec_new {
  program_options_exec_ptr p;
  program_options_exec* operator->() const { return p.get(); }
  program_options_exec_new() {}
  void init(string_consumer const& warn, std::string const& caption) {
    p.reset(new program_options_exec(warn, caption));
  }
  program_options_exec_new(string_consumer const& warn, std::string const& caption)
      : p(new program_options_exec(warn, caption)) {}
  program_options_exec_new(program_options_exec_new const& o) : p(o.p) {}
};


// TODO: add non-leaf help info so program_options formatted descriptions have at least one-level-nesting
// headers. or does usage(str) do that already?
struct configure_program_options : configure_backend_base<configure_program_options> {
  typedef configure_backend_base<configure_program_options> base;
  FORWARD_BASE_CONFIGURE_ACTIONS(base)
  program_options_exec_ptr popt;
  configure_program_options(configure_program_options const& o) : base(o), popt(o.popt) {}
  configure_program_options(program_options_exec_new const& popt_, string_consumer const& warn_to,
                            int verbose_max = default_verbose_max)
      : base(warn_to, verbose_max), popt(popt_.p) {
    popt->warn = warn_to;
  }
  explicit configure_program_options(program_options_exec_new const& popt_,
                                     int verbose_max = default_verbose_max)
      : base(popt_.p->warn, verbose_max), popt(popt_.p) {}


  template <class Val>
  graehl::option_options<Val> po(std::string const& pathname, conf_opt const& opt, Val*) const {
    graehl::option_options<Val> po;
    if (opt.is_implicit()) po.implicit_value_str = opt.implicit->value;
    if (opt.is_init())
      po.default_value_str = opt.init->value;  // even though we don't need to rely on po lib to handle
    // setting initial values or generating usage for us.

    if (opt.is_required_err())
      po.required = true;  // TODO: we actually check this ourselves, but it's ok to have p opt lib check it
    // too (unless we want to support multiple sources. this way it knows to require
    // >=1 arg?

    if (opt.is_deprecated()) {
      po.notify0 = opt.deprecate->get_notify0(warn, pathname);
      SHOWIF2(CONFEXPR, 1, "deprecated ", pathname, opt);
    }
    po.is = opt.is;
    po.hidden = opt.is_too_verbose(verbose_max);
    return po;
  }

  template <class Val>
  void leaf_action(init_config, Val* pval, conf_expr_base const& conf) const {
    this->check_leaf_impl(pval, conf);
    conf_opt const& opt = *conf.opt;
    std::string const& pathname = conf.path_name();
    using graehl::add;
    if (opt.is_required_warn()) add(popt->warn_missing, pathname);
    if (opt.is_required_err()) add(popt->error_missing, pathname);
    popt->opt_desc.option(poname(conf), pval, opt.get_usage() + opt.get_init_or_eg_suffix_quote(*this),
                          po(pathname, opt, pval));
    SHOWIF3(CONFEXPR, 1, "declare: option added: ", pval, pathname, conf);
    if (opt.is_positional()) {
      SHOWIF2(CONFEXPR, 1, "positional ", pathname, conf);
      popt->positional.add(
          conf.path_name().c_str(),  // ok to pass c_str() since it's stored in a string immediately
          opt.positional->max <= 0 ? -1 : opt.positional->max);
    }
  }
  template <class Val>
  void sequence_action(init_config const& a, Val* pval, conf_expr_base const& conf) const {
    leaf_action(a, pval, conf);
  }
  template <class Val>
  void set_action(init_config const& a, Val* pval, conf_expr_base const& conf) const {
    leaf_action(a, pval, conf);
  }

  template <class Val>
  void tree_action(init_config, Val* pval, conf_expr_base const& conf) const {
    conf_opt const& opt = *conf.opt;
    if (opt.allows_unrecognized()) popt->allow_unrecognized(conf.path_name(), *opt.allow_unrecognized);
  }

  bool init_action(init_config) const { return true; }

  bool init_action(store_config) const {
    popt->finish_store();
    // SHOWIF1(CONFEXPR,1, "stored",*popt);
    return false;
  }

  bool init_action(help_config const& c) const {
    popt->show_po_help(*c.o);
    // SHOWIF1(CONFEXPR,1, "helped",*popt);
    return false;
  }

  template <class Val>
  void leaf_action(show_example_config a, Val* pval, conf_expr_base const& conf) const {
    base::leaf_action(a, pval, conf);
    //*usage.o << conf.opt->get_leaf_value(*pval, " --"+conf.path_name()+"=","\n");
  }


  static inline std::string poname(conf_expr_base const& conf) {
    conf_opt const& opt = *conf.opt;
    std::string name = conf.path_name();
    if (opt.charname) {
      name.append(",");
      name.append(1, *opt.charname);
    }
    return name;
  }
};

template <class Action, class RootVal>
void program_options_action(program_options_exec_new const& popt, Action const& action, RootVal* val,
                            string_consumer const& warn_to) {
  configure_action(configure_program_options(popt, warn_to), action, val, warn_to);
}

template <class Action, class RootVal>
void program_options_action(program_options_exec_new const& popt, Action const& action, RootVal* val) {
  configure_action(configure_program_options(popt, default_verbose_max), action, val, popt.p->warn);
}

template <class RootVal>
bool program_options_maybe_help(std::ostream& o, program_options_exec_new const& popt, RootVal* pval,
                                string_consumer const& warn_to = warn_consumer()) {
  if (popt.p->is_help()) {
    program_options_action(popt, help_config(o), pval, warn_to);
    return true;
  }
  return false;
}
}

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>

BOOST_AUTO_TEST_CASE(configure_program_options_test) {
  using namespace graehl;
  BOOST_CHECK(true);
}

#endif


#if GRAEHL_CONFIGURE_SAMPLE_MAIN

namespace my {

using namespace std;
using namespace boost;
using namespace graehl;

enum AB { kA, kB };

inline std::string to_string_impl(AB ab) {
  return ab == kA ? "A" : "B";
}

inline void string_to_impl(std::string const& abstr, AB& ab) {
  ab = abstr == "A" ? kA : kB;
}

inline std::string type_string(AB) {
  return "A|B";
}

struct Thing {
  template <class O>
  void print(O& o) const {
    throw "Thing is not a leaf!";
  }
  template <class Ch, class Tr>
  friend std::basic_ostream<Ch, Tr>& operator<<(std::basic_ostream<Ch, Tr>& o, Thing const& self) {
    self.print(o);
    return o;
  }

  struct SubThing {
    std::vector<int> xs;
    string str;
    template <class Config>
    void configure(Config& c) {
      c.is("SubThing");  // used to describe this type in usage.
      c("str", &str).positional(1).desire();  // desire = like require but just warn on abscence, don't throw
      c("xs", &xs)('X')
          .eg("abc:2")
          .is("2 102 4 ... (even numbers)")
          .require();  // short option name 'X' synonym (single char)
      /* require in this context means at least one element must be provided (since xs is a vector)
         note that command line --a.xs 2 4 --b.xs 4 is equivalent to --a.xs 2 --b.xs 4 --a.xs 4, and you may
         need to use -- to terminate a vector option so that positional arguments aren't consumed.
       */
      // note: c(key, &val)(string): string is description for usage. is(string) is a name for the type of
      // value only.
    }
  };
  struct LeafThing {
    int i;
    // these free fns will be found via argument dependent lookup:
    friend void validate(LeafThing& x) {
      if (x.i > 10) throw "too big";
    }  // backend note: throwing strings as exception should result in pretty error messages.
    friend void init(LeafThing& x) {
      x.i = -1;
    }  // non-default default if used in vector or map, or asked for with .init()
    typedef void
        leaf_configure;  // this means the following 3 free functions are used instead of a .configure(...)
    friend std::string to_string_impl(LeafThing const& x) {
      return lexical_cast<string>(x.i);
    }  // overrides for leaf_configure
    friend void string_to_impl(string const& str, LeafThing& x) { graehl::string_to(str, x.i); }
    friend string type_string(LeafThing const& x) { return "LeafThing"; }  // TODO: ADL
  };

  SubThing a, b;
  optional<int> deceasedYear;  // note: helpful to tell if the user specified the option or not
  bool verbose;
  LeafThing leaf;  // default constructed
  int even;
  AB ab;
  std::map<string, string> otherKeyVals;
  template <class Config>
  void configure(Config& c) {
    c("An example of mapping configuration options to an object tree. This should be word wrapped at 80 "
      "columns, I hope. "
      "Hereisaverylongwordwithnospaceswhichshouldalsobewordwrappedsinceitwouldbesadifitwerentabletofitinour80"
      "columnterminal!.ok!");
    c.is("Thing");
    c.allow_unrecognized(true);  // true=warn
    c("death-year", &deceasedYear).desire().positional();  // .validate(configure::bounded_range(1900,2012));
    c("multiple-of-2", &even)('m').eg(4).init(2)(
        "an even number");  // if not specified, error. short cmdline name = -m
    c("verbose", &verbose).flag()("Log more status.");  // flag() is a hint to provide no-value shorthand for
    // command line options parsers: no-value --verbose or
    // --no-verbose flags. for YAML, flag() should be
    // ignored (you still have to specify a true or false
    // value)
    c("year", &deceasedYear).eg(1995).deprecate("in favor of --death-year");  // instead of checking if the
    /*
     int has its default value still, this allows you to know for sure that the
     default was used. note that eg merely needs to lexical_cast<string>
    */
    c("a",
      &a);  // this means we have command line options --a.numbers --a.xys, and YAML paths a.numbers and a.xys
    c("b", &b)
        .is("SubThing 2")
        .allow_unrecognized();  // overrides default "SubThing" as "SubThing 2". gives us b-numbers and b-xys.
    c("leaf", &leaf).init_default();  // just like: int x; c("leaf", &x).is("LeafThing"); init_default() means
    // init_default(LeafThing &) will be called, which at least
    // default-constructs
    c("ab", &ab)("an enum!").init(kB);
  }
};
}

// LEAF_CONFIGURABLE_EXTERNAL(my::AB)


int main(int argc, char* argv[]) {
  using namespace std;
  using namespace boost;
  configure::program_options_exec_new exec(configure::warn_consumer(), "SAMPLE");
  using my::Thing;
  Thing thing;
  typedef map<string, int> map_;
  graehl::warn_consumer to_cerr;
  try {
    cout << "default help:\n\n";
    configure::help(cout, &thing);
    cout << "\ndefault help done.\n\n";
    configure::program_options_action(exec, configure::init_config(), &thing, to_cerr);
    exec.p->set_main_argv(argc, argv);
    if (program_options_maybe_help(cout, exec, &thing, to_cerr)) return 0;
    configure::program_options_action(exec, configure::store_config(), &thing, to_cerr);
    configure::validate_stored(&thing, to_cerr);
    cout << "\neffective:\n";
    configure::show_effective(cout, &thing);
  } catch (std::exception& e) {
    cerr << "ERROR: " << e.what() << "\n";
  }
  return 0;
}
#endif

#endif
