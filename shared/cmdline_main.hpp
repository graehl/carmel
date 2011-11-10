#ifndef GRAEHL_SHARED__CMDLINE_MAIN_HPP
#define GRAEHL_SHARED__CMDLINE_MAIN_HPP

/* yourmain inherit from graehl::main (
   overriding:
   virtual run()
   add_options_extra()
   set_defaults() maybe calling set_defaults_base() etc.)
   then INT_MAIN(yourmain)
*/

#ifndef GRAEHL_CONFIG_FILE
#define GRAEHL_CONFIG_FILE "config-file"
#endif
#ifndef GRAEHL_IN_FILE
#define GRAEHL_IN_FILE "in-file"
#endif
#ifndef GRAEHL_OUT_FILE
#define GRAEHL_OUT_FILE "out-file"
#endif
#ifndef GRAEHL_LOG_FILE
#define GRAEHL_LOG_FILE "log-file"
#endif

#ifndef GRAEHL_DEBUGPRINT
# define GRAEHL_DEBUGPRINT 0
#endif

#include <graehl/shared/program_options.hpp>
#include <graehl/shared/itoa.hpp>
#include <graehl/shared/command_line.hpp>
#include <graehl/shared/fileargs.hpp>
#include <graehl/shared/teestream.hpp>
#include <graehl/shared/makestr.hpp>
#if GRAEHL_DEBUGPRINT
# include <graehl/shared/debugprint.hpp>
#endif
#include <iostream>

#define INT_MAIN(main_class) main_class m;                              \
  int main(int argc,char **argv) { return m.run_main(argc,argv); }

#define GRAEHL_MAIN_COMPILED " (compiled " MAKESTR_DATE ")"

namespace graehl {

struct main {
  typedef main self_type;

  friend inline std::ostream &operator<<(std::ostream &o,self_type const& s)
  {
    s.print(o);
    return o;
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

  typedef std::vector<istream_arg> in_files;
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
    void allow_ins(int max_ins_=-1) {
      max_ins=max_ins_;
    }
    void allow_in(bool positional=true) {
      add_in_file=positional_in=true;
    }
    void require_ins(int max_ins_=-1) {
      min_ins=1;
      max_ins=max_ins_;
    }
    void validate(in_files const& ins) const {
      validate(ins.size());
    }
    void validate(int n) const {
      if (has_max_ins() && n>max_ins)
        throw std::runtime_error("Too many input files (max="+itos(max_ins)+", had "+itos(n)+")");
      if (n<min_ins)
        throw std::runtime_error("Too few input files (min="+itos(min_ins)+", had "+itos(n)+")");
    }

    int min_ins,max_ins; //multiple inputs 0,1,... if max>min
    bool add_in_file,add_out_file,add_log_file,add_config_file,add_help,add_debug_level;
    bool positional_in,positional_out;
    base_options() {
      positional_out=positional_in=add_in_file=false;
      add_log_file=add_help=add_out_file=add_config_file=add_debug_level=true;
      min_ins=max_ins=0;
    }
  };
  base_options bopt;

  int debug_lvl;
  bool help;
  ostream_arg log_file,out_file;
  istream_arg in_file,config_file;
  in_files ins;

  std::string cmdname,cmdline_str;
  std::ostream *log_stream;
  std::auto_ptr<teebuf> teebufptr;
  std::auto_ptr<std::ostream> teestreamptr;

  std::string appname,version,compiled,usage;

  //FIXME: segfault if version is a char const* global in subclass xtor, why?
  main(std::string const& name="main",std::string const& usage="usage undocumented\n",std::string const& version="v1",std::string const& compiled=GRAEHL_MAIN_COMPILED)  : appname(name),version(version),compiled(compiled),usage(usage),general("General options"),cosmetic("Cosmetic options"),all_options("Options"),options_added(false)
  {
  }

  typedef printable_options_description<std::ostream> OD;
  OD general,cosmetic,all_options;
  bool options_added;

  void print_version(std::ostream &o) {
    o << cmdname << ' ' << version << ' ' << compiled << std::endl;
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
  }

  virtual void validate_parameters_extra() {}

  // this should only be called once.  (called after set_defaults)
  virtual void add_options(OD &all)
  {
    if (options_added)
      return;
    add_options_base(all);
    add_options_extra(all);
    finish_options(all);
  }


  virtual void finish_options(OD &all)
  {
    all.add(general).add(cosmetic);
    options_added=true;
  }

  virtual void add_options_extra(OD &all) {}

  virtual void log_invocation()
  {
    log_invocation_base();
  }


  virtual void set_defaults_base()
  {
    out_file=stdout_arg();
    log_file=stderr_arg();
    in_file=stdin_arg();
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
    all_options.print(log(),vm,OD::SHOW_DEFAULTED | OD::SHOW_HIERARCHY);
    log() << "###\n";
  }

  void validate_parameters_base()
  {
    bopt.validate(ins);
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

  void add_options_base(OD &all)
  {

    if (bopt.add_help)
      all.add_options()
        ("help,h", boost::program_options::bool_switch(&help),
         "show usage/documentation")
        ;

    if (bopt.add_out_file) {
      all.add_options()
        (GRAEHL_OUT_FILE",o",defaulted_value(&out_file),
         "Output here (instead of STDOUT)");
      if (bopt.positional_out)
        output_positional();
    }

    if (bopt.add_ins()) {
      all.add_options()
        (GRAEHL_IN_FILE",i",optional_value(&ins)->multitoken(),
         bopt.input_help()
          )
        ;
      if (bopt.positional_in)
        input_positional(bopt.max_ins);
    } else if (bopt.add_in_file) {
      all.add_options()
        (GRAEHL_IN_FILE",i",defaulted_value(&in_file),
         bopt.input_help());
      if (bopt.positional_in)
        input_positional();
    }

    if (bopt.add_config_file)
      all.add_options()
        (GRAEHL_CONFIG_FILE,optional_value(&config_file),
         "load boost program options config from file");

    if (bopt.add_log_file)
      all.add_options()
        (GRAEHL_LOG_FILE",l",defaulted_value(&log_file),
         "Send logs messages here (as well as to STDERR)");

#if GRAEHL_DEBUGPRINT
    if (bopt.add_debug_level)
      all.add_options()
        ("debug-level,d",defaulted_value(&debug_lvl),
         "Debugging output level (0 = off, 0xFFFF = max)")
#endif
        ;

  }

  boost::program_options::variables_map vm;
  boost::program_options::positional_options_description positional;

  void add_positional(std::string const& name,int n=1) {
    positional.add(name.c_str(),n);
  }
  void input_positional(int n=1) {
    positional.add(GRAEHL_IN_FILE,n);
  }
  void output_positional() {
    positional.add("out-file",1);
  }
  void log_positional() {
    positional.add("log-file",1);
  }
  void all_positional() {
    input_positional();
    output_positional();
    log_positional();
  }

  bool parse_args(int argc, char **argv)
  {
    using namespace std;
    using namespace boost::program_options;

    add_options(all_options);

    try {
      cmdline_str=graehl::get_command_line(argc,argv,NULL);
      parsed_options po=all_options.parse_options(argc,argv,&positional);
      store(po,vm);
      //notify(vm); // variables aren't set until notify?
      if (maybe_get(vm,GRAEHL_CONFIG_FILE,config_file)) {
        try {
//          config_file.set(get_string(vm,GRAEHL_CONFIG_FILE));
          store(parse_config_file(*config_file,all_options),vm); /*Stores in 'm' all options that are defined in 'options'. If 'm' already has a non-defaulted value of an option, that value is not changed, even if 'options' specify some value. */
          //NOTE: this means that cmdline opts have precedence. hooray.
          config_file.close();
        } catch(exception const& e) {
          std::cerr << "ERROR: parsing "<<cmdname<<" config file "<<config_file.name<<" - "<<e.what();
          throw;
        }
      }
      notify(vm); // are multiple notifies idempotent? depends on user fns registered?

      if (help) {
        cout << "\n" << get_name() << "\n\n";
        cout << general_options_desc();
        cout << usage << "\n";
        cout << all_options << "\n";
        return false;
      }
    } catch (std::exception &e) {
      std::cerr << "ERROR: "<<e.what() << " (parsing "<<cmdname<<" options)\nTry '" << argv[0] << " -h' for help\n\n";
      throw;
    }
    return true;
  }


  //FIXME: defaults cannot change after first parse_args
  int run_main(int argc, char** argv)
  {
    cmdname=argv[0];
    set_defaults();
    try {
      if (!parse_args(argc,argv))
        return 1;
      validate_parameters();
      log_invocation();
      run();
    }
    catch(std::bad_alloc& e) {
      return carp("ran out of memory\nTry descreasing -m or -M, and setting an accurate -P if you're using initial parameters.");
    }
    catch(std::exception& e) {
      return carp(e.what());
    }
    catch(char const* e) {
      return carp(e);
    }
    catch(...) {
      return carp("Exception of unknown type!");
    }
    return 0;
  }

  template <class C>
  int carp(C const& c) const
  {
    log() << "\nERROR: " << c << "\n\n" << "Try '" << cmdname << " -h' for documentation\n";
    return 1;
  }

  virtual ~main() {}
};



}



#endif
