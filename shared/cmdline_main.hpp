#ifndef GRAEHL_SHARED__CMDLINE_MAIN_HPP
#define GRAEHL_SHARED__CMDLINE_MAIN_HPP

/* yourmain inherit from graehl::main (
   overriding:
     virtual run()
     add_options_extra()
     set_defaults() maybe calling set_defaults_base() etc.)
   then INT_MAIN(yourmain)
*/

#define GRAEHL_CONFIG_FILE "config-file"
#define GRAEHL_IN_FILE "in-file"
#define GRAEHL_OUT_FILE "out-file"
#define GRAEHL_LOG_FILE "log-file"

#ifndef GRAEHL_DEBUGPRINT
# define GRAEHL_DEBUGPRINT 0
#endif

#include <graehl/shared/program_options.hpp>
#include <graehl/shared/command_line.hpp>
#include <graehl/shared/fileargs.hpp>
#include <graehl/shared/teestream.hpp>
#include <graehl/shared/makestr.hpp>
#if GRAEHL_DEBUGPRINT
# include <graehl/shared/debugprint.hpp>
#endif
#include <iostream>

#define INT_MAIN(main_class) main_class m;                      \
  int main(int argc,char **argv) { return m.run(argc,argv); }

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


  int debug_lvl;
  bool help;
  ostream_arg log_file,out_file;
  istream_arg in_file,config_file;
  std::string cmdname,cmdline_str;
  std::ostream *log_stream;
  std::auto_ptr<teebuf> teebufptr;
  std::auto_ptr<std::ostream> teestreamptr;

  std::string appname,version,compiled,usage;

  //FIXME: segfault if version is a char const* global in subclass xtor, why?
  main(std::string const& name="main",std::string const& usage="usage undocumented\n",std::string const& version="v1",std::string const& compiled=GRAEHL_MAIN_COMPILED)  : appname(name),version(version),compiled(compiled),usage(usage),general("General options"),cosmetic("Cosmetic options"),all_options("Allowed options"),options_added(false)
  {
  }

  typedef printable_options_description<std::ostream> OD;
  OD general,cosmetic,all_options;
  bool options_added;

  virtual void run()
  {
    out() << cmdname << ' ' << version << ' ' << compiled << std::endl;
  }

  virtual void set_defaults()
  {
    set_defaults_base();
    set_defaults_extra();
  }

  virtual void validate_parameters()
  {
    validate_parameters_base();
  }

  virtual void add_options_base(OD &all)
  {
    add_options_base(all,true,true);
  }

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
    log() << "### COMMAND LINE:\n" << cmdline_str << "\n\n";
    log() << "### CHOSEN OPTIONS:\n";
    all_options.print(log(),vm,OD::SHOW_DEFAULTED | OD::SHOW_HIERARCHY);
  }

  void validate_parameters_base()
  {
    log_stream=log_file.get();
    if (!is_default_log(log_file)) // tee to cerr
    {
      teebufptr.reset(new teebuf(log_stream->rdbuf(),std::cerr.rdbuf()));
      teestreamptr.reset(log_stream=new std::ostream(teebufptr.get()));
    }
#if GRAEHL_DEBUGPRINT && defined(DEBUG)
    DBP::set_logstream(&log());
    DBP::set_loglevel(log_level);
#endif

  }

  void add_options_base(OD &all,bool add_in_file,bool add_out_file=true,bool use_config_file=true)
  {
    using boost::program_options::bool_switch;


    general.add_options()
      ("help,h", bool_switch(&help),
       "show usage/documentation")
      ;

    if (add_out_file)
      general.add_options()
        (GRAEHL_OUT_FILE",o",defaulted_value(&out_file),
         "Output here (instead of STDOUT)");

    if (add_in_file) {
      general.add_options()
        (GRAEHL_IN_FILE",i",defaulted_value(&in_file),
         "Output here (instead of STDIN)");
    }

    if (use_config_file)
      general.add_options()
        (GRAEHL_CONFIG_FILE,optional_value(&config_file),
        "load boost program options config from file");

    general.add_options()
      (GRAEHL_LOG_FILE",l",defaulted_value(&log_file),
       "Send logs messages here (as well as to STDERR)");
#if GRAEHL_DEBUGPRINT
    general.add_options()
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
  void input_positional() {
    positional.add("in-file",1);
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
          throw std::runtime_error("ERROR: while parsing options config file "+config_file.name+" - "+e.what());
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
      std::cerr << "ERROR:"<<e.what() << "\nTry '" << argv[0] << " -h' for help\n\n";
      throw;
    }
    return true;
  }


  //FIXME: defaults cannot change after first parse_args
  int run(int argc, char** argv)
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
    catch(const char * e) {
      return carp(e);
    }
    catch(...) {
      return carp("FATAL Exception of unknown type!");
    }
    return 0;

  }

  template <class C>
  int carp(C const& c) const
  {
    log() << "\nERROR: " << c << "\n\n";
    log() << "Try '" << cmdname << " -h' for documentation\n";
    return 1;
  }

  virtual ~main() {}
};



}



#endif
