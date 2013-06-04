#ifndef GRAEHL_SHARED__CMDLINE_MAIN_HPP
#define GRAEHL_SHARED__CMDLINE_MAIN_HPP

/* yourmain inherit from graehl::main (
   overriding:
   virtual run()
   add_options_extra()
   set_defaults() maybe calling set_defaults_base() etc.)
   then INT_MAIN(yourmain)

   or (with -DZGRAEHL_CMDLINE_MAIN_USE_CONFIGURE=1)

   instead of add_options_extra, set_defaults, define a template configure()
   (see configure.hpp)
*/


#ifndef GRAEHL_CMDLINE_SAMPLE_MAIN
# ifdef GRAEHL_G1_MAIN
#  define ZGRAEHL_CMDLINE_MAIN_USE_CONFIGURE 1
#  define GRAEHL_CMDLINE_SAMPLE_MAIN 1
# else
#  define GRAEHL_CMDLINE_SAMPLE_MAIN 0
# endif
#endif

#ifndef GRAEHL_CONFIG_FILE
#define GRAEHL_CONFIG_FILE "config"
#endif
#ifndef GRAEHL_IN_FILE
#define GRAEHL_IN_FILE "in"
#endif
#ifndef GRAEHL_OUT_FILE
#define GRAEHL_OUT_FILE "out"
#endif
#ifndef GRAEHL_LOG_FILE
#define GRAEHL_LOG_FILE "log"
#endif

#ifndef GRAEHL_DEBUGPRINT
# define GRAEHL_DEBUGPRINT 0
#endif

#ifndef ZGRAEHL_CMDLINE_MAIN_USE_CONFIGURE
# define ZGRAEHL_CMDLINE_MAIN_USE_CONFIGURE 1
#endif

#if ZGRAEHL_CMDLINE_MAIN_USE_CONFIGURE
# include <graehl/shared/configure_program_options.hpp>
#endif

#include <graehl/shared/assign_traits.hpp>
#include <graehl/shared/program_options.hpp>
#include <graehl/shared/itoa.hpp>
#include <graehl/shared/command_line.hpp>
#include <graehl/shared/fileargs.hpp>
#include <graehl/shared/teestream.hpp>
#include <graehl/shared/makestr.hpp>
#include <graehl/shared/string_to.hpp>
#include <graehl/shared/random.hpp>
//#include <graehl/shared/verbose_exception.hpp>

#if GRAEHL_DEBUGPRINT
# include <graehl/shared/debugprint.hpp>
#endif
#include <iostream>

#define INT_MAIN(main_class) int main(int argc,char **argv) { main_class m; return m.run_main(argc,argv); }

#define GRAEHL_MAIN_COMPILED " (compiled " MAKESTR_DATE ")"

namespace graehl {

struct main {
  typedef main self_type;

  friend inline std::ostream &operator<<(std::ostream &o,self_type const& s)
  {
    s.print(o);
    return o;
  }

  void allow_random() {
    bopt.allow_random();
  }

  virtual void print(std::ostream &o) const {
    o << appname<<"-version={{{" << get_version() << "}}} "<<appname<<"-cmdline={{{"<<cmdline_str<<"}}}";
  }

  std::string get_name() const
  {
    return appname+"-"+get_version();
  }

  std::string get_version() const
  {
    return version+compiled;
  }

  typedef istream_arg in_arg;
  typedef std::vector<in_arg> in_args;
  struct base_options {
    char const* positional_help() const {
      return positional_in?"; positional args ok too":"";
    }
    std::string input_help() const {
      std::ostringstream s;
      if (add_ins()) {
        s<<"Multiple input files (- for STDIN) at least "<<min_ins;
        if (has_max_ins())
          s<<" and at most "+itos(max_ins);
      } else if (add_in_file)
        s<<"Input file (- for STDIN)";
      else
        return "No input file options";
      s<<positional_help();
      return s.str();
    }

    bool add_ins() const {
      return min_ins || max_ins;
    }
    bool has_max_ins() const {
      return max_ins>=0;
    }
    void allow_ins(bool positional=true,int max_ins_=-1) {
      max_ins=max_ins_;
      positional_in=positional;
    }
    void allow_in(bool positional=true) {
      add_in_file=true;
      positional_in=positional;
    }
    void require_ins(bool positional=true,int max_ins_=-1) {
      allow_ins(positional,max_ins_);
      min_ins=1;
    }
    void validate(in_args const& ins) const {
      unsigned n=(unsigned)ins.size();
      validate(n);
      for (unsigned i=0;i<n;++i)
        if (!*ins[i]) {
          // VTHROW_MSG("invalid input #"<<utos(i+1)<<" file "+ins[i].name);
          throw std::runtime_error("Invalid input file #"+utos(i+1)+" file "+ins[i].name);
        }
    }
    void validate(int n) const {
      if (has_max_ins() && n>max_ins)
        throw std::runtime_error("Too many input files (max="+itos(max_ins)+", had "+itos(n)+")");
      if (n<min_ins)
        throw std::runtime_error("Too few input files (min="+itos(min_ins)+", had "+itos(n)+")");
    }
    void allow_random(bool val=true)
    {
      add_random=val;
    }
    int min_ins,max_ins; //multiple inputs 0,1,... if max>min
    bool add_in_file,add_out_file,add_log_file,add_config_file,add_help,add_debug_level;
    bool positional_in,positional_out;
    bool add_quiet,add_verbose;
    bool add_random;
    base_options() {
      positional_out=positional_in=add_in_file=add_random=false;
      add_log_file=add_help=add_out_file=add_config_file=add_debug_level=true;
      min_ins=max_ins=0;
      add_verbose=false;
      add_quiet=true;
    }
  };
  friend inline void init_default(main &) {}
  base_options bopt;

  int debug_lvl;
  bool help;
  bool quiet;
  int verbose;
  ostream_arg log_file,out_file;
  istream_arg in_file,config_file;
  in_args ins; // this will also have the single in_file if you bopt.allow_in()

  istream_arg const& first_input() const
  {
    return ins.empty() ? in_file : ins[0];
  }

  std::string cmdname,cmdline_str;
  std::ostream *log_stream;
  std::auto_ptr<teebuf> teebufptr;
  std::auto_ptr<std::ostream> teestreamptr;

  std::string appname,version,compiled,usage;
  boost::uint32_t random_seed;

  //FIXME: segfault if version is a char const* global in subclass xtor, why?
  main(std::string const& name="main",std::string const& usage="usage undocumented\n",std::string const& version="v1",bool multifile=false,bool random=false,bool input=true,std::string const& compiled=GRAEHL_MAIN_COMPILED)
    : appname(name),version(version),compiled(compiled),usage(usage)
    , general("General options"),cosmetic("Cosmetic options"),all_options_("Options"),options_added(false),configure_finished(false)
#if ZGRAEHL_CMDLINE_MAIN_USE_CONFIGURE
    , to_cerr(),exec(to_cerr,"Options")
#else
#endif
  {
    if (random)
      allow_random();
    if (input) {
      if (multifile) {
        bopt.require_ins();
      } else {
        bopt.allow_in(true);
      }
    }
    verbose=1;
    random_seed=default_random_seed();
    init();
  }

  typedef printable_options_description<std::ostream> OD;
  OD general,cosmetic;
  OD all_options_;
  OD &all_options()
  {
#if ZGRAEHL_CMDLINE_MAIN_USE_CONFIGURE
    return exec.p->opt_desc;
#else
    return all_options_;
#endif
  }

  bool options_added,configure_finished;

  void print_version(std::ostream &o) {
    o << cmdname << ' ' << version << ' ' << compiled << std::endl;
  }

  virtual int run_exit()
  {
    run();
    return 0;
  }

  virtual void run()
  {
    print_version(out());
  }

  virtual void set_defaults()
  {
    set_defaults_base();
    set_defaults_extra();
  }

  virtual void validate_parameters()
  {
    validate_parameters_base();
    validate_parameters_extra();
    set_random_seed(random_seed);
  }

  virtual void validate_parameters_extra() {}

  virtual void add_options(OD &optionsDesc)
  {
    if (options_added)
      return;
    add_options_base(optionsDesc);
    add_options_extra(optionsDesc);
    finish_options(optionsDesc);
  }
  // this should only be called once. (called after set_defaults)
  virtual void add_options_extra(OD &optionsDesc) {}

  virtual void finish_options(OD &optionsDesc)
  {
#if !ZGRAEHL_CMDLINE_MAIN_USE_CONFIGURE
    if (general.size())
      optionsDesc.add(general);
    if (cosmetic.size())
      optionsDesc.add(cosmetic);
#endif
    options_added=true;
  }

  virtual void log_invocation()
  {
    if (verbose>0)
      log_invocation_base();
  }


  void init()
  {
    out_file=stdout_arg();
    log_file=stderr_arg();
    in_file=stdin_arg();
    quiet=false;
    help=false;
  }

  virtual void set_defaults_base()
  {
    init();
  }

  virtual void set_defaults_extra() {}

  inline std::ostream &log() const {
    return *log_stream;
  }

  inline std::istream &in() const {
    return *in_file;
  }

  inline std::ostream &out() const {
    return *out_file;
  }

  void log_invocation_base()
  {
    log() << "### COMMAND LINE:\n" << cmdline_str << "\n";
    log() << "### USING OPTIONS:\n";
#if ZGRAEHL_CMDLINE_MAIN_USE_CONFIGURE
    confs.effective(log(),to_cerr);
#else
    all_options().print(log(),get_vm(),SHOW_DEFAULTED | SHOW_HIERARCHY);
#endif
    log() << "###\n";
    log() << "###\n";
  }


  void validate_parameters_base()
  {
    if (quiet) {
      if (verbose>0)
        log()<<"setting verbose to 0 since --quiet, even though you set it to "<<verbose<<"\n";
      verbose=0;
    }
    if (bopt.add_ins())
      bopt.validate(ins);
    if (in_file && ins.empty())
      ins.push_back(in_file);

    log_stream=log_file.get();
    if (!log_stream)
      log_stream=&std::cerr;
    else if (!is_default_log(log_file)) {
      // tee to cerr
      teebufptr.reset(new teebuf(log_stream->rdbuf(),std::cerr.rdbuf()));
      teestreamptr.reset(log_stream=new std::ostream(teebufptr.get()));
    }
#if GRAEHL_DEBUGPRINT && defined(DEBUG)
    DBP::set_logstream(&log());
    DBP::set_loglevel(log_level);
#endif

  }

  template <class Conf> void configure(Conf &c)
  {
    help=false;
    quiet=false;
    c.is(appname);
    if (bopt.add_help)
      c("help",&help)('h').flag()("show usage/documentation").verbose();

    if (bopt.add_quiet)
      c("quiet",&quiet)('q').flag()(
        "use log only for warnings - e.g. no banner of command line options used");

    if (bopt.add_verbose)
      c("verbose",&verbose)('v')(
        "e.g. verbosity level >1 means show banner of command line options. 0 means don't");

    if (bopt.add_out_file)
      c(GRAEHL_OUT_FILE,&out_file)('o').positional(bopt.positional_out)(
        "Output here (instead of STDOUT)").eg("outfile.gz");

    if (bopt.add_ins()) {
      int nin=bopt.max_ins;
      if (nin<0) nin=0;
      c(GRAEHL_IN_FILE,&ins)('i')(bopt.input_help()).eg("infileN.gz").positional(bopt.positional_in,nin);
    } else if (bopt.add_in_file)
      c(GRAEHL_IN_FILE,&in_file)('i')(bopt.input_help()).eg("infile.gz").positional(bopt.positional_in);

    if (bopt.add_config_file)
      c(GRAEHL_CONFIG_FILE,&config_file)('c')(
        "load boost program options config from file").eg("config.ini");

    if (bopt.add_log_file)
      c(GRAEHL_LOG_FILE,&log_file)('l')(
        "Send log messages here (as well as to STDERR)").eg("log.gz");

#if GRAEHL_DEBUGPRINT
    if (bopt.add_debug_level)
      c("debug-level",&debug_lvl)('d')(
        "Debugging output level (0 = off, 0xFFFF = max)");
#endif

    if (bopt.add_random)
      c("random-seed",&random_seed)(
        "Random seed - if specified, reproducible pseudorandomness. Otherwise, seed is random.");
  }

  void add_options_base(OD &optionsDesc)
  {
#if !ZGRAEHL_CMDLINE_MAIN_USE_CONFIGURE
    if (bopt.add_help)
      optionsDesc.add_options()
        ("help,h", boost::program_options::bool_switch(&help),
         "show usage/documentation")
        ;

    if (bopt.add_quiet)
      optionsDesc.add_options()
        ("quiet,q", boost::program_options::bool_switch(&quiet),
         "use log only for warnings - e.g. no banner of command line options used")
        ;

    if (bopt.add_verbose)
      optionsDesc.add_options()
        ("verbose,v", defaulted_value(&verbose),
         "e.g. verbosity level >1 means show banner of command line options. 0 means don't")
        ;

    if (bopt.add_out_file) {
      optionsDesc.add_options()
        (GRAEHL_OUT_FILE",o",defaulted_value(&out_file),
         "Output here (instead of STDOUT)");
      if (bopt.positional_out)
        output_positional();
    }

    if (bopt.add_ins()) {
      optionsDesc.add_options()
        (GRAEHL_IN_FILE",i",optional_value(&ins)->multitoken(),
         bopt.input_help()
          )
        ;
      if (bopt.positional_in)
        input_positional(bopt.max_ins);
    } else if (bopt.add_in_file) {
      optionsDesc.add_options()
        (GRAEHL_IN_FILE",i",defaulted_value(&in_file),
         bopt.input_help());
      if (bopt.positional_in)
        input_positional();
    }

    if (bopt.add_config_file)
      optionsDesc.add_options()
        (GRAEHL_CONFIG_FILE,optional_value(&config_file),
         "load boost program options config from file");

    if (bopt.add_log_file)
      optionsDesc.add_options()
        (GRAEHL_LOG_FILE",l",defaulted_value(&log_file),
         "Send log messages here (as well as to STDERR)");

#if GRAEHL_DEBUGPRINT
    if (bopt.add_debug_level)
      optionsDesc.add_options()
        ("debug-level,d",defaulted_value(&debug_lvl),
         "Debugging output level (0 = off, 0xFFFF = max)")
#endif

        if (bopt.add_random)
          optionsDesc.add_options()
            ("random-seed,R",defaulted_value(&random_seed),
             "Random seed")
            ;
#endif
  }


  // called once, before any config actions are run
  void finish_configure()
  {
    if (configure_finished)
      return;
    configure_finished=true;
#if ZGRAEHL_CMDLINE_MAIN_USE_CONFIGURE
    SHOWIF1(CONFEXPR,1,"finish_configure",this);
    // deferred until subclass etc has change to declare postitional etc
    configurable(this);
#endif
    finish_configure_extra();
  }
  virtual void finish_configure_extra()
  {
  }

#if ZGRAEHL_CMDLINE_MAIN_USE_CONFIGURE
  typedef configure::configure_list configure_list;
private:
  configure_list confs;
public:
  configure_list &get_confs()
  {
    finish_configure();
    return confs;
  }

  configure::configure_program_options conf() const
  {
    return configure::configure_program_options(exec);
  }

  template <class Val>
  void configurable(Val *pval,std::string const& groupname,std::string const& help_prefix="")
  {
    configurable(help_prefix,pval,configure::opt_path(1,groupname));
  }

  template <class Val>
  void configurable(std::string const& help_prefix,Val *pval,configure::opt_path const& prefix=configure::opt_path())
  {
    confs.add(pval,conf(),prefix,help_prefix);
  }

  template <class Val>
  void configurable(Val *pval,configure::opt_path const& prefix=configure::opt_path())
  {
    confs.add(pval,conf(),prefix);
  }

  boost::program_options::positional_options_description &get_positional()
  {
    return exec.p->positional;
  }
  boost::program_options::variables_map &get_vm()
  {
    return exec.p->vm;
  }
#else
  boost::program_options::variables_map vm;
  boost::program_options::positional_options_description positional;
  boost::program_options::positional_options_description &get_positional()
  {
    return positional;
  }
  boost::program_options::variables_map &get_vm()
  {
    return vm;
  }
#endif

#if ZGRAEHL_CMDLINE_MAIN_USE_CONFIGURE
  // actually, these are set inside configure()
#else
  void add_positional(std::string const& name,int n=1) {
    get_positional().add(name.c_str(),n);
  }
  void input_positional(int n=1) {
    get_positional().add(GRAEHL_IN_FILE,n);
  }
  void output_positional() {
    get_positional().add(GRAEHL_OUT_FILE,1);
  }
  void log_positional() {
    get_positional().add(GRAEHL_LOG_FILE,1);
  }
  void all_positional() {
    input_positional();
    output_positional();
    log_positional();
  }
#endif

  void show_help(std::ostream &o)
  {
    o << "\n" << get_name() << "\n\n";
    o << usage << "\n\n";
#if ZGRAEHL_CMDLINE_MAIN_USE_CONFIGURE
    confs.standard_help(o,to_cerr); // currently repeats --help each time since it's shown through po lib
    o << "\n\n" << appname << " [OPTIONS]:\n";
    exec->show_po_help(o);
#else
    o << general_options_desc()<<"\n";
    o << all_options() << "\n";
#endif
  }


  bool parse_args(int argc, char **argv)
  {
    set_defaults();
    using namespace std;
    using namespace boost::program_options;
    cmdline_str=graehl::get_command_line(argc,argv,NULL);
    add_options(all_options());
    try {
#if ZGRAEHL_CMDLINE_MAIN_USE_CONFIGURE
      SHOWIF1(CONFEXPR,1,"parse_args",cmdline_str);
      finish_configure();
      confs.init(to_cerr);
      exec->set_main_argv(argc,argv);
      if (exec->is_help()) {
        show_help(std::cout);
        return false;
      }
      confs.store(to_cerr);
#else
      parsed_options po=all_options().parse_options(argc,argv,&get_positional());
      store(po,get_vm());
      //notify(get_vm()); // variables aren't set until notify?
      if (maybe_get(get_vm(),GRAEHL_CONFIG_FILE,config_file)) {
        try {
          // config_file.set(get_string(get_vm(),GRAEHL_CONFIG_FILE));
          store(parse_config_file(*config_file,all_options()),get_vm()); /*Stores in 'm' all options that are defined in 'options'. If 'm' already has a non-defaulted value of an option, that value is not changed, even if 'options' specify some value. */
          //NOTE: this means that cmdline opts have precedence. hooray.
          config_file.close();
        } catch(exception &e) {
          std::cerr << "ERROR: parsing "<<cmdname<<" config file "<<config_file.name<<" - "<<e.what();
          throw;
        }
      }
      notify(get_vm()); // are multiple notifies idempotent? depends on user fns registered?
      if (help) {
        show_help(std::cout);
        return false;
      }
#endif
    } catch (std::exception &e) {
      std::cerr << "ERROR: "<<e.what() << "\n while parsing "<<cmdname<<" options:\n"<<cmdline_str<<"\n\n" << argv[0] << " -h\n for help\n\n";
  //show_help(std::cerr);
      throw;
    }
    return true;
  }

#if ZGRAEHL_CMDLINE_MAIN_USE_CONFIGURE
  configure::warn_consumer to_cerr; //TODO: set stream to logfile
  configure::program_options_exec_new exec;
#endif
//FIXME: defaults cannot change after first parse_args
  int run_main(int argc, char **argv)
  {
    cmdname=argv[0];
    try {
      if (!parse_args(argc,argv))
        return 1;
      validate_parameters();
      log_invocation();
      return run_exit();
    }
    catch(std::bad_alloc&) {
      return carpexcept("ran out of memory\nTry descreasing -m or -M, and setting an accurate -P if you're using initial parameters.");
    }
    catch(std::exception& e) {
      return carpexcept(e.what());
    }
    catch(char const* e) {
      return carpexcept(e);
    }
    catch(...) {
      return carpexcept("Exception of unknown type!");
    }
    return 0;
  }

// for some reason i see segfault with log() when exiting from main. race condition? weird.
  template <class C>
  int carpexcept(C const& c) const
  {
    std::cerr << "\nERROR: " << c << "\n\n" << "Try '" << cmdname << " -h' for documentation\n";
    return 1;
  }
  template <class C>
  int carp(C const& c) const
  {
    log() << "\nERROR: " << c << "\n\n" << "Try '" << cmdname << " -h' for documentation\n";
    return 1;
  }
  template <class C>
  void warn(C const& c) const
  {
    log() << "\nWARNING: " << c << std::endl;
  }

  virtual ~main() {}
};

template<>
struct assign_traits<main,void> : no_assign
{};


}

#if GRAEHL_CMDLINE_SAMPLE_MAIN
struct sample_main : graehl::main
{
};
graehl::main sample_m;

int main(int argc,char **argv)
{
  return sample_m.run_main(argc,argv);
}
#endif

#endif
