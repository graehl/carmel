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
#ifndef GRAEHL_SHARED__CMDLINE_MAIN_HPP
#define GRAEHL_SHARED__CMDLINE_MAIN_HPP
#pragma once

/* yourmain inherit from graehl::main (
   overriding:
   virtual run()
   set_defaults() maybe calling set_defaults_base() etc.)
   then INT_MAIN(yourmain)

   and finally:

   add_options_extra()

   or (with -DGRAEHL_CMDLINE_MAIN_USE_CONFIGURE=1)

   and this->configurable(my_configurable)

   (see configure.hpp)
*/


#ifndef GRAEHL_CMDLINE_SAMPLE_MAIN
#ifdef GRAEHL_G1_MAIN
#define GRAEHL_CMDLINE_MAIN_USE_CONFIGURE 1
#define GRAEHL_CMDLINE_SAMPLE_MAIN 1
#else
#define GRAEHL_CMDLINE_SAMPLE_MAIN 0
#endif
#endif

#ifndef GRAEHL_CONFIG_FILE
#define GRAEHL_CONFIG_FILE "config"
#endif
#ifndef GRAEHL_IN_FILE
#define GRAEHL_IN_FILE "in"
#endif
#ifndef GRAEHL_LOG_FILE
#define GRAEHL_LOG_FILE "log"
#endif

#ifndef GRAEHL_DEBUGPRINT
#define GRAEHL_DEBUGPRINT 0
#endif

#ifndef GRAEHL_CMDLINE_MAIN_USE_CONFIGURE
#define GRAEHL_CMDLINE_MAIN_USE_CONFIGURE 1
#endif

#include <graehl/shared/cpp11.hpp>
#if GRAEHL_CMDLINE_MAIN_USE_CONFIGURE
#include <graehl/shared/configurable.hpp>
#include <graehl/shared/configure_program_options.hpp>
#endif

#include <graehl/shared/assign_traits.hpp>
#include <graehl/shared/command_line.hpp>
#include <graehl/shared/fileargs.hpp>
#include <graehl/shared/itoa.hpp>
#include <graehl/shared/program_options.hpp>
#include <graehl/shared/random.hpp>
#include <graehl/shared/string_to.hpp>
#include <graehl/shared/teestream.hpp>

#if GRAEHL_DEBUGPRINT
#include <graehl/shared/debugprint.hpp>
#endif
#include <graehl/shared/int_types.hpp>
#include <iostream>

#define INT_MAIN(main_class)         \
  int main(int argc, char* argv[]) { \
    main_class m;                    \
    return m.run_main(argc, argv);   \
  }

#define GRAEHL_MAIN_COMPILED " (compiled " __DATE__ " " __TIME__ ")"

namespace graehl {


typedef std::vector<istream_arg> istream_args;

struct base_options {
  char const* positional_help() const { return positional_in ? "; positional args ok too" : ""; }
  std::string input_help() const {
    std::ostringstream s;
    if (add_ins()) {
      s << "Multiple input files (- for STDIN) at least " << min_ins;
      if (has_max_ins()) s << " and at most " + itos(max_ins);
    } else if (add_in_file)
      s << "Input file (- for STDIN)";
    else
      return "No input file options";
    s << positional_help();
    return s.str();
  }

  bool add_ins() const { return min_ins || max_ins; }
  bool has_max_ins() const { return max_ins >= 0; }
  void no_ins() {
    max_ins = 0;
    min_ins = 0;
    positional_in = false;
    add_in_file = false;
  }
  void allow_ins(bool positional = true, int max_ins_ = -1) {
    max_ins = max_ins_;
    positional_in = positional;
  }
  void allow_in(bool positional = true) {
    add_in_file = true;
    positional_in = positional;
  }
  void require_ins(bool positional = true, int max_ins_ = -1) {
    allow_ins(positional, max_ins_);
    min_ins = 1;
  }
  void validate(istream_args const& ins) const {
    if (add_ins()) {
      unsigned n = (unsigned)ins.size();
      validate(n);
      for (unsigned i = 0; i < n; ++i)
        if (!*ins[i]) {
          throw std::runtime_error("Invalid input file #" + utos(i + 1) + " file " + ins[i].name);
        }
    }
  }
  void validate(int n) const {
    if (has_max_ins() && n > max_ins)
      throw std::runtime_error("Too many input files (max=" + itos(max_ins) + ", had " + itos(n) + ")");
    if (n < min_ins)
      throw std::runtime_error("Too few input files (min=" + itos(min_ins) + ", had " + itos(n) + ")");
  }
  void allow_random(bool val = true) { add_random = val; }
  int min_ins, max_ins;  // multiple inputs 0,1,... if max>min
  bool add_in_file, add_out_file, add_log_file, add_config_file, add_help, add_debug_level;
  bool positional_in, positional_out;
  bool add_quiet, add_verbose;
  bool add_random;
  base_options() {
    positional_out = positional_in = add_in_file = add_random = false;
    add_log_file = add_help = add_out_file = add_config_file = add_debug_level = true;
    min_ins = max_ins = 0;
    add_verbose = false;
    add_quiet = true;
  }
  void disable() {
    no_ins();
    add_out_file = false;
    add_config_file = false;
    add_help = false;
    add_verbose = false;
    add_random = false;
    add_debug_level = false;
    positional_in = positional_out = false;
  }
};

/**
   options for what options to add to your cmdline 'main' subclass.
*/
struct main_options : base_options {
  std::string name;
  std::string usage;
  std::string version;
  std::string compiled;
  bool multifile;
  bool random;
  bool input;

  void defaults() {
    multifile = false;
    input = true;
    random = false;
    version = "v1";
    compiled = GRAEHL_MAIN_COMPILED;
  }

  main_options() { defaults(); }
  static main_options no_input() {
    main_options opt;
    opt.input = false;
    return opt;
  }

  void init() {
    if (random) allow_random();
    if (input) {
      if (multifile) {
        require_ins();
      } else {
        allow_in(true);
      }
    }
  }
  void no_ins() {
    base_options::no_ins();
    input = false;
  }
};

struct main {
  typedef std::vector<istream_arg> istream_args;

  main_options opt;

  typedef main configure_validate;
  void validate() { opt.validate(ins); }

  typedef main self_type;

  friend inline std::ostream& operator<<(std::ostream& o, self_type const& s) {
    s.print(o);
    return o;
  }

  virtual void print(std::ostream& o) const {
    o << opt.name << "-version={{{" << get_version() << "}}} " << opt.name << "-cmdline={{{" << cmdline_str
      << "}}}";
  }

  std::string const& name() const { return opt.name.empty() ? exename : opt.name; }

  std::string name_version() const { return name() + "-" + get_version(); }

  std::string get_version() const { return opt.version + opt.compiled; }

  typedef istream_arg in_arg;
  friend inline void init_default(main&) {}

  int debug_lvl;
  bool help;
  bool quiet;
  int verbose;
  ostream_arg log_file, out_file;
  istream_arg in_file, config_file;
  istream_args ins;  // this will also have the single in_file if you opt.allow_in()

  istream_arg const& first_input() const { return ins.empty() ? in_file : ins[0]; }

  std::string cmdname, cmdline_str;
  std::ostream* log_stream;
#if !GRAEHL_CPP11
  std::auto_ptr<teebuf> teebufptr;
  std::auto_ptr<std::ostream> teestreamptr;
#else
  std::unique_ptr<teebuf> teebufptr;
  std::unique_ptr<std::ostream> teestreamptr;
#endif
  uint32_t random_seed;

  int help_exitcode;

  /// using this constructor, you must call init before any other methods (e.g. configurable)
  main() : general("General options"), cosmetic("Cosmetic options"), all_options_("Options") {}

  void init() {
#if GRAEHL_CMDLINE_MAIN_USE_CONFIGURE
    exec.init(to_cerr, "Options");
#endif
    opt.init();
    options_added = configure_finished = false;
    help_exitcode = 0;
    verbose = 1;
    random_seed = default_random_seed();
    out_file = stdout_arg();
    log_file = stderr_arg();
    in_file = stdin_arg();
    quiet = false;
    help = false;
  }


  explicit main(main_options const& opt)
      : opt(opt), general("General options"), cosmetic("Cosmetic options"), all_options_("Options") {
    this->opt = opt;
    init();
  }

  main(std::string const& name, std::string const& usage = "usage undocumented",
       std::string const& version = "v1", bool multifile = false, bool random = false, bool input = true,
       std::string const& compiled = GRAEHL_MAIN_COMPILED)
      : general("General options"), cosmetic("Cosmetic options"), all_options_("Options") {
    opt.name = name;
    opt.usage = usage;
    opt.version = version;
    opt.compiled = compiled;
    opt.version = version;
    opt.multifile = multifile;
    opt.random = random;
    opt.input = input;
    init();
  }

  typedef printable_options_description<std::ostream> OD;
  OD general, cosmetic;
  OD all_options_;
  OD& all_options() {
#if GRAEHL_CMDLINE_MAIN_USE_CONFIGURE
    return exec.p->opt_desc;
#else
    return all_options_;
#endif
  }

  bool options_added, configure_finished;

  void print_version(std::ostream& o) { o << opt.name << ' ' << opt.version << ' ' << opt.compiled << '\n'; }

  virtual int run_exit() {
    run();
    return 0;
  }

  virtual void run() { print_version(out()); }

  virtual void set_defaults() {
    set_defaults_base();
    set_defaults_extra();
  }

  virtual void validate_parameters() {
    validate_parameters_base();
    validate_parameters_extra();
    set_random_seed(random_seed);
  }

  virtual void validate_parameters_extra() {}

  virtual void add_options(OD& optionsDesc) {
    if (options_added) return;
    add_options_base(optionsDesc);
    add_options_extra(optionsDesc);
    finish_options(optionsDesc);
  }
  // this should only be called once. (called after set_defaults)
  virtual void add_options_extra(OD& optionsDesc) {}

  virtual void finish_options(OD& optionsDesc) {
#if !GRAEHL_CMDLINE_MAIN_USE_CONFIGURE
    if (general.size()) optionsDesc.add(general);
    if (cosmetic.size()) optionsDesc.add(cosmetic);
#endif
    options_added = true;
  }

  virtual void log_invocation() {
    if (verbose > 1) log_invocation_base();
  }

  virtual void set_defaults_base() { init(); }

  virtual void set_defaults_extra() {}

  std::ostream& log() const { return *log_stream; }

  std::istream& in() const { return *in_file; }

  std::ostream& out() const { return *out_file; }

  std::string outName() const { return out_file.name; }

  void log_invocation_base() {
    log() << "### COMMAND LINE:\n" << cmdline_str << "\n";
    log() << "### USING OPTIONS:\n";
#if GRAEHL_CMDLINE_MAIN_USE_CONFIGURE
    confs.effective(log(), to_cerr);
#else
    all_options().print(log(), get_vm(), SHOW_DEFAULTED | SHOW_HIERARCHY);
#endif
    log() << "###\n";
    log() << "###\n";
  }


  void validate_parameters_base() {
    validate();

    log_stream = log_file.get();
    if (!log_stream)
      log_stream = &std::cerr;
    else if (!is_default_log(log_file)) {
      // tee to cerr
      teebufptr.reset(new teebuf(log_stream->rdbuf(), std::cerr.rdbuf()));
      teestreamptr.reset(log_stream = new std::ostream(teebufptr.get()));
    }
    if (quiet) verbose = 0;
    if (in_file && ins.empty()) {
      ins.push_back(istream_arg());
      swap(ins.back(), in_file);
    }

#if GRAEHL_DEBUGPRINT && defined(DEBUG)
    DBP::set_logstream(&log());
    DBP::set_loglevel(log_level);
#endif
  }

  template <class Conf>
  void configure(Conf& c) {
    c.is(opt.name);
    if (opt.add_help) c("help", &help)('h').flag()("show usage/documentation").verbose();

    if (opt.add_quiet)
      c("quiet", &quiet)('q').flag()(
          "use log only for warnings - e.g. no banner of command line options used");

    if (opt.add_verbose)
      c("verbose",
        &verbose)('v')("e.g. verbosity level >1 means show banner of command line options. 0 means don't");

    if (opt.input)
      if (opt.add_ins()) {
        int nin = opt.max_ins;
        if (nin < 0) nin = 0;
        c(GRAEHL_IN_FILE, &ins)('i')(opt.input_help()).eg("infileN.gz").positional(opt.positional_in, nin);
      } else if (opt.add_in_file)
        c(GRAEHL_IN_FILE, &in_file)('i')(opt.input_help()).eg("infile.gz").positional(opt.positional_in);

    if (opt.add_out_file) {
      c("output", &out_file).alias();
      c("out", &out_file)('o')
          .positional(opt.positional_out)("Output here (instead of STDOUT)")
          .eg("outfile.gz");
    }

    if (opt.add_config_file)
      c(GRAEHL_CONFIG_FILE, &config_file)('c')("load boost program options config from file")
          .eg("config.ini");

    if (opt.add_log_file)
      c(GRAEHL_LOG_FILE, &log_file)('l')("Send log messages here (as well as to STDERR)").eg("log.gz");

#if GRAEHL_DEBUGPRINT
    if (opt.add_debug_level) c("debug-level", &debug_lvl)("Debugging output level (0 = off, 0xFFFF = max)");
#endif

    if (opt.add_random)
      c("random-seed", &random_seed)(
          "Random seed - if specified, reproducible pseudorandomness. Otherwise, seed is random.");
  }

  void add_options_base(OD& optionsDesc) {
#if !GRAEHL_CMDLINE_MAIN_USE_CONFIGURE
    if (opt.add_help)
      optionsDesc.add_options()("help,h", boost::program_options::bool_switch(&help),
                                "show usage/documentation");

    if (opt.add_quiet)
      optionsDesc.add_options()("quiet,q", boost::program_options::bool_switch(&quiet),
                                "use log only for warnings - e.g. no banner of command line options used");

    if (opt.add_verbose)
      optionsDesc.add_options()(
          "verbose,v", defaulted_value(&verbose),
          "e.g. verbosity level >1 means show banner of command line options. 0 means don't");

    if (opt.add_out_file) {
      optionsDesc.add_options()("out,o", defaulted_value(&out_file), "Output here (instead of STDOUT)");
      if (opt.positional_out) output_positional();
    }

    if (opt.input)
      if (opt.add_ins()) {
        optionsDesc.add_options()(GRAEHL_IN_FILE ",i", optional_value(&ins)->multitoken(), opt.input_help());
        if (opt.positional_in) input_positional(opt.max_ins);
      } else if (opt.add_in_file) {
        optionsDesc.add_options()(GRAEHL_IN_FILE ",i", defaulted_value(&in_file), opt.input_help());
        if (opt.positional_in) input_positional();
      }

    if (opt.add_config_file)
      optionsDesc.add_options()(GRAEHL_CONFIG_FILE, optional_value(&config_file),
                                "load boost program options config from file");

    if (opt.add_log_file)
      optionsDesc.add_options()(GRAEHL_LOG_FILE ",l", defaulted_value(&log_file),
                                "Send log messages here (as well as to STDERR)");

#if GRAEHL_DEBUGPRINT
    if (opt.add_debug_level)
      optionsDesc.add_options()("debug-level,d", defaulted_value(&debug_lvl),
                                "Debugging output level (0 = off, 0xFFFF = max)");
#endif

    if (opt.add_random)
      optionsDesc.add_options()("random-seed,R", defaulted_value(&random_seed), "Random seed");
#endif
  }


  // called once, before any config actions are run
  void finish_configure() {
    before_finish_configure();
    if (configure_finished) return;
    configure_finished = true;
#if GRAEHL_CMDLINE_MAIN_USE_CONFIGURE
    SHOWIF1(CONFEXPR, 1, "finish_configure", this);
    // deferred until subclass etc has change to declare postitional etc
    configurable(this);
#endif
    finish_configure_extra();
    declare_configurable();
  }
  virtual void before_finish_configure() {}
  virtual void finish_configure_extra() {}
  virtual void declare_configurable() {}

#if GRAEHL_CMDLINE_MAIN_USE_CONFIGURE
  typedef configure::configure_list configure_list;

 private:
  configure_list confs;

 public:
  configure_list& get_confs() {
    finish_configure();
    return confs;
  }

  configure::configure_program_options conf() const { return configure::configure_program_options(exec); }

  template <class Val>
  void configurable(Val* pval, std::string const& groupname, std::string const& help_prefix = "") {
    configurable(help_prefix, pval, configure::opt_path(1, groupname));
  }

  template <class Val>
  void configurable(std::string const& help_prefix, Val* pval,
                    configure::opt_path const& prefix = configure::opt_path()) {
    confs.add(pval, conf(), prefix, help_prefix);
  }

  template <class Val>
  void configurable(Val* pval, configure::opt_path const& prefix = configure::opt_path()) {
    confs.add(pval, conf(), prefix);
  }

  boost::program_options::positional_options_description& get_positional() { return exec.p->positional; }
  boost::program_options::variables_map& get_vm() { return exec.p->vm; }
#else
  boost::program_options::variables_map vm;
  boost::program_options::positional_options_description positional;
  boost::program_options::positional_options_description& get_positional() { return positional; }
  boost::program_options::variables_map& get_vm() { return vm; }
#endif

#if GRAEHL_CMDLINE_MAIN_USE_CONFIGURE
// actually, these are set inside configure()
#else
  void add_positional(std::string const& name, int n = 1) { get_positional().add(name.c_str(), n); }
  void input_positional(int n = 1) { get_positional().add(GRAEHL_IN_FILE, n); }
  void output_positional() { get_positional().add(GRAEHL_OUT_FILE, 1); }
  void log_positional() { get_positional().add(GRAEHL_LOG_FILE, 1); }
  void all_positional() {
    input_positional();
    output_positional();
    log_positional();
  }
#endif


  void show_usage(std::ostream& o) {
    o << "\n" << name_version() << "\n\n";
    o << opt.usage << "\n\n";
  }

  void show_help(std::ostream& o) {
    show_usage(o);
#if GRAEHL_CMDLINE_MAIN_USE_CONFIGURE
    confs.standard_help(o, to_cerr);  // currently repeats --help each time since it's shown through po lib
    o << "\n\n" << opt.name << " [OPTIONS]:\n";
    exec->show_po_help(o);
#else
    o << general_options_desc() << "\n";
    o << all_options() << "\n";
#endif
  }


  /// \return false = help, true = success (else exception)
  int parse_args(int argc, char** argv) {
    std::string const v = "-v";
    if (opt.add_verbose)
      for (int i = argc - 2; i > 0; --i)
        if (argv[i] == v) {
          verbose = atoi_nows(argv[i + 1]);
          break;
        }
    set_defaults();
    using namespace std;
    using namespace boost::program_options;
    cmdline_str = graehl::get_command_line(argc, argv, NULL);
    add_options(all_options());
    try {
#if GRAEHL_CMDLINE_MAIN_USE_CONFIGURE
      SHOWIF1(CONFEXPR, 1, "parse_args", cmdline_str);
      finish_configure();
      confs.init(to_cerr);
      exec->set_main_argv(argc, argv);
      if (opt.add_help && exec->maybe_po_help(std::cout)) {
        show_usage(std::cout << '\n');
        std::cout << '\n';
        return false;
      }
      confs.store(to_cerr);
      confs.validate(to_cerr);
      if (verbose > 1) confs.effective(std::cerr, to_cerr);
#else
      parsed_options po = all_options().parse_options(argc, argv, &get_positional());
      variables_map& vm = get_vm();
      store(po, vm);
      if (vm.count("help")) {
        show_help(std::cout);
        return false;
      }
      if (maybe_get(vm, GRAEHL_CONFIG_FILE, config_file)) {
        try {
          store(parse_config_file(*config_file, all_options()), vm); /*Stores in 'm' all options that are
                                                                        defined in 'options'. If 'm' already
                                                                        has a non-defaulted value of an
                                                                        option, that value is not changed,
                                                                        even if 'options' specify some value.
                                                                        */
          // NOTE: this means that cmdline opts have precedence. hooray.
          config_file.close();
        } catch (exception& e) {
          std::cerr << "ERROR: parsing " << name << " config file " << config_file.name << " - " << e.what();
          throw;
        }
      }
      notify(vm);  // are multiple notifies idempotent? depends on user fns registered?
      if (help) {
        show_help(std::cout);
        return false;
      }
#endif
    } catch (std::exception& e) {
      std::cerr << "ERROR: " << e.what() << "\n while parsing " << opt.name << " options:\n"
                << cmdline_str << "\n\n"
                << argv[0] << " -h\n for help\n\n";
      throw;
    }
    return true;
  }

#if GRAEHL_CMDLINE_MAIN_USE_CONFIGURE
  graehl::warn_consumer to_cerr;  // TODO: set stream to logfile
  configure::program_options_exec_new exec;
#endif
  std::string exename;
  // FIXME: defaults cannot change after first parse_args
  int run_main(int argc, char** argv) {
    exename = argv[0];
    try {
      if (!parse_args(argc, argv)) return help_exitcode;  // help is ok!
      validate_parameters();
      log_invocation();
      return run_exit();
    } catch (std::bad_alloc&) {
      return carpexcept(
          "ran out of memory\nTry descreasing -m or -M, and setting an accurate -P if you're using initial "
          "parameters.");
    } catch (std::exception& e) {
      return carpexcept(e.what());
    } catch (char const* e) {
      return carpexcept(e);
    } catch (...) {
      return carpexcept("Exception of unknown type!");
    }
    return 0;
  }

  // for some reason i see segfault with log() when exiting from main. race condition? weird.
  template <class C>
  int carpexcept(C const& c) const {
    std::cerr << "\nERROR: " << c << "\n\n"
              << "Try '" << name() << " -h' for documentation\n";
    return 1;
  }
  template <class C>
  int carp(C const& c) const {
    log() << "\nERROR: " << c << "\n\n"
          << "Try '" << name() << " -h' for documentation\n";
    return 1;
  }
  template <class C>
  void warn(C const& c) const {
    log() << "\nWARNING: " << c << '\n';
  }

  virtual ~main() {}
};

template <>
struct assign_traits<main, void> : no_assign {};
}

#if GRAEHL_CMDLINE_SAMPLE_MAIN
struct sample_main : graehl::main {};
graehl::main sample_m;

int main(int argc, char** argv) {
  return sample_m.run_main(argc, argv);
}
#endif


#endif
