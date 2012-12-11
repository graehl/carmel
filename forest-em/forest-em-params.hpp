

#ifndef GRAEHL_TT__FOREST_EM_PARAMS_HPP
#define GRAEHL_TT__FOREST_EM_PARAMS_HPP

#define FOREST_EM_VERSION "v19"

#include <graehl/shared/em.hpp>
#include <graehl/shared/myassert.h>
#include <boost/config.hpp>
#include <graehl/shared/tree.hpp>
#include <graehl/shared/fileargs.hpp>
#include <graehl/shared/weight.h>
#include <iostream>
#include <graehl/shared/teestream.hpp>
#include <graehl/shared/io.hpp>
#include <graehl/shared/command_line.hpp>
#include <boost/program_options.hpp>
#include <graehl/shared/command_line.hpp>
#include <graehl/shared/backtrace.hpp>
#include <graehl/shared/makestr.hpp>
#include <graehl/shared/gibbs.hpp>
#include "forest-em.README.hpp" // note: dependency/makefile won't make #include <graehl/tt...
#define FOREST_EM_COMPILED " (compiled " MAKESTR_DATE ")"

//TODO: pass ref to this to Forests (even though it increases coupling - pretty
//much copies of all of these are used anyway) to simplify
//constructor/prepare_em


namespace graehl {
  static const std::string rule_format_version=" filetype=rule version=1.0";

  struct ForestEmParams {
    bool double_precision;
    bool random_set;
    unsigned max_iter;
    int log_level;
    double converge_ratio;
    double converge_delta;
    unsigned random_restarts;
    unsigned watch_rule;
    unsigned watch_depth;
    unsigned forest_tick_period;
    Weight prior_counts;
    bool help,human_probs,normalize_initial,initial_1_params,checkpoint_parameters,zero_zerocounts;
    std::string tempfile_prefix,byid_prob_field,byid_count_field;
    bool viterbi_enable,per_forest_counts_enable;
    size_t viterbi_per,per_forest_counts_per;
    std::string checkpoint_prefix;
    Weight count_report_threshold,prob_report_threshold;
    bool count_report_enable;
    unsigned random_seed;
    Weight add_k_smoothing;
    size_t max_forest_nodes,max_normgroup_size,prealloc_params;
    unsigned watch_period;
    istream_arg initparam_file,priorcounts_file,byid_rule_file;
    ifstream_arg rules_file,forests_file,normgroups_file; // can't be STDIN
    ostream_arg outviterbi_file,out_score_per_forest,out_per_forest_counts_file,outparam_file,log_file,byid_output_file,outcounts_file;
    std::ostream *log_stream;
    std::string cmdline_str;
    std::auto_ptr<teebuf> teebufptr;
    std::auto_ptr<std::ostream> teestreamptr;

    bool gibbs() {return gopt.iter>0; }
    gibbs_opts gopt;

    template <class OD>
    void add_options(OD &all)
    {
      using boost::program_options::bool_switch;

      OD general("General options");
      general.add_options()
        ("help,h", bool_switch(&help),
         "show usage/documentation")
        ("random-seed",  defaulted_value(&random_seed),
         "specify a 32-bit unsigned random seed for exact repeatability")
        ("use-double-precision,U", bool_switch(&double_precision),
         "use double-precision floats (8 bytes instead of 4) for params and counts")
        ;
      OD training("Training options (use '-' to specify STDIN)");
      training.add_options()
        ("forests-file,f",defaulted_value(&forests_file),
         "derivation forests (required)")
        ("normgroups-file,n",defaulted_value(&normgroups_file),
         "Normalization groups file (required) - e.g. ((1 2 20) (30 31))")
        ("max-forest-nodes,m", defaulted_value(&max_forest_nodes),
         "Maximum size of a single forest, in nodes (overestimate to get fewer swap files)")
        ("max-normgroup-size,M", defaulted_value(&max_normgroup_size),
         "Maximum number of parameters in a single normalization group (overestimate to get fewer swap files)")
        ("prealloc-params,P", defaulted_value(&prealloc_params),
         "Number of initial parameters expected - memory might run out unless you declare a tight upper bound here")
        ("zero-zerocounts,z",bool_switch(&zero_zerocounts),
         "when a normgroup has all 0 counts for its parameters, give them 0 (not uniform) prob")
        ("max-iter,i", defaulted_value(&max_iter),
         "Maximum number of iterations")
        ("converge,e", defaulted_value(&converge_ratio),
         "Converge if relative change in per-derivation average log likelihood is less than epsilon")
        ("deltaparam-epsilon,d", defaulted_value(&converge_delta),
         "Converge if all parameter changes are no more than epsilon")
        ("random-restarts,r",defaulted_value(&random_restarts),
         "Number of times to randomly initialize parameters (after doing one iteration with supplied parameters)")
        ("random-set",bool_switch(&random_set),
         "If no initial parameters supplied, use a random init rather than uniform")
        ("prior-counts-per,p",defaulted_value(&prior_counts),
         "Initial per-example (Dirichlet prior) counts given to each parameter")
        ("add-k-smoothing,k",defaulted_value(&add_k_smoothing),
         "Counts added to denominator in normalization - think of this as accounting for unseen events (this especially penalizes low-count normalization groups).")
        ("outparam-file,o",defaulted_value(&outparam_file),
         "Write final parameters here (note: 1-based; nth line = param #n)")
        ("outcounts-file,O",defaulted_value(&outcounts_file),
         "Write unnormalized em counts here (1-based)")
        ("outviterbi-file,v",defaulted_value(&outviterbi_file),
         "Write one-per-line 'prob <viterbi derivation forest>' e.g. 'e^-10.5 (1 (2 3))'")
        ("out-per-forest-counts-file,E",defaulted_value(&out_per_forest_counts_file),
         "Write one-line-per-example '(ruleid:rulecounts ...)' e.g. '(2:e^-10.5 5:e^0 6:e^2.4)'")
        ("out-per-forest-inside-sum,S",defaulted_value(&out_score_per_forest),
         "Write one-line-per-example inside score (sum of score of all derivations in that forest)")
        ("initparam-file,I",defaulted_value(&initparam_file),
         "Initial parameter values (1-based; 1st line = param #1)")
        ("normalize-initial,N", bool_switch(&normalize_initial),
         "Normalize parameters before first EM iteration; useful with --initparam-file file -i 0")
        ("initial-1-params,u", bool_switch(&initial_1_params),
         "If no initial parameters supplied, set initial parameters to (unnormalized) 1")
        //        ("priorcounts-file",defaulted_value(&priorcounts_file),
        //        "Additional (Dirichlet prior) counts for each parameter")
        ;
      OD cosmetic("Cosmetic options");
      cosmetic.add_options()
        ("log-level,L",defaulted_value(&log_level),
         "Higher log-level => more status messages to logfile (0 means absolutely quiet except for final statistics)")
        ("watch-rule,w",defaulted_value(&watch_rule),
         "Watch the top rule weights for the normalization group holding the given rule id")
        ("watch-depth,D",defaulted_value(&watch_depth),
         "Size of the top rule watch list")
        ("watch-period,W",defaulted_value(&watch_period),
         "How many iterations to skip between dumping checkpoint files, and between --watch-rule")
        ("tempfile-prefix,t",defaulted_value(&tempfile_prefix),
         "Intermediate filenames will be <tempfile-prefix>.some_suffix")
        ("checkpoint-prefix,x",defaulted_value(&checkpoint_prefix),
         "every watch-period iterations, write info to files named <checkpoint-prefix>.<checkpoint-type>.restart.N.iteration.M")
        ("checkpoint-parameters,c",bool_switch(&checkpoint_parameters),
         "Store 'params' and 'counts' files per checkpoint-prefix")
        ("checkpoint-viterbi-per-examples,V",defaulted_value(&viterbi_per),
         "Store 'viterbi' files per <checkpoint-prefix>, for every Vth forest (0 = disable)")
        ("checkpoint-per-forest-counts,Z",defaulted_value(&per_forest_counts_per),
         "Store 'per-forest-counts' files per <checkpoint-prefix>, for every Zth forest (0 = disable)")
        ("rules-file,R",defaulted_value(&rules_file),
         "(optional) rule description file, one rule per line, starting index 1; used by watch-rule")
        ("log-file,l",defaulted_value(&log_file),
         "Send logs messages here (as well as to STDERR)")
        ("human-probs,H",bool_switch(&human_probs),
         "Display probabilities as plain real numbers instead of logs")
        ("forest-tick-period,T",defaulted_value(&forest_tick_period),
         "Display a tick (.) after processing n forests, unless n=0")
        ("byid-rule-file,b",defaulted_value(&byid_rule_file),
         "Unsorted rules file with id=N attribute")
        ("byid-prob-field,F",defaulted_value(&byid_prob_field),
         "If byid-rule-file is given, \" byid-prob-field=param[N]\" will be written after each id=N in it")
        ("byid-count-field,C",defaulted_value(&byid_count_field),
         "If byid-rule-file is given, \" byid-count-field=count[N]\" will be written after each id=N in it")
        ("byid-output-file,B",defaulted_value(&byid_output_file),
         "(useful only with byid-rule-file) write modified rules here")
        ("report-counts-exceeding,X",defaulted_value(&count_report_threshold),
         "log the number of parameters with count of at least X, every watch-report iterations")
        ("report-probs-exceeding,Y",defaulted_value(&prob_report_threshold),
         "log number of params with prob at least Y, every watch-report iterations")
        ;

      OD gibbs(gibbs_opts::desc());
      gopt.add_options(gibbs,false,true);

      all.add(general).add(training).add(cosmetic).add(gibbs);
    }

    void set_defaults()
    {
      count_report_enable=false;

      const unsigned MEGS=1024*1024;
      zero_zerocounts=false;

      help=false;
      double_precision=false;
      normalize_initial=false;
      initial_1_params=false;
      checkpoint_parameters=false;
      human_probs=false;

      random_seed=default_random_seed();
      max_forest_nodes=50 * MEGS;
      max_normgroup_size=50 * MEGS;
      prealloc_params=165 * MEGS;
      max_iter=1000;
      converge_ratio=1./65536;
      converge_delta=0;
      random_restarts=0;
      prior_counts=0;
      add_k_smoothing=0;
      outparam_file=ostream_arg();
      outcounts_file=ostream_arg();
      outviterbi_file=ostream_arg();
      out_per_forest_counts_file=ostream_arg();
      initparam_file=istream_arg();
      priorcounts_file=istream_arg();
      log_level=1;
      watch_rule=0;
      watch_depth=20;
      watch_period=10;
      tempfile_prefix="/tmp/forest-em";
      checkpoint_prefix="";
      viterbi_per=0;
      per_forest_counts_per=0;
      log_file=graehl::stderr_arg();
      forest_tick_period=10000;
      byid_rule_file=istream_arg();
      byid_prob_field="emprob";
      byid_count_field="emcount";
      byid_output_file=ostream_arg();
      count_report_threshold=Weight::INF();
      prob_report_threshold=Weight::INF();
    }

    bool parse_args(int argc, char * argv[])
    {
      using namespace std;
      using namespace boost::program_options;
      typedef printable_options_description<std::ostream> OD;

      OD all("Allowed options");
      add_options(all);

      const char *progname=argv[0];
      try {
        cmdline_str=graehl::get_command_line(argc,argv,NULL);
        //MAKESTRS(...,print_command_line(os,argc,argv,NULL));
        variables_map vm;
        all.parse_options_and_notify(argc,argv,vm);

        if (help) {
          cout << "\n" << progname << ' ' << "version " << get_version() << "\n\n";
          cout << usage_str << "\n";
          cout << all << "\n";
          return false;
        }
        log() << "### COMMAND LINE:\n" << cmdline_str << "\n\n";
        log() << "### CHOSEN OPTIONS:\n";
        all.print(log(),vm,SHOW_DEFAULTED | SHOW_HIERARCHY);
      } catch (std::exception &e) {
        std::cerr << "ERROR:"<<e.what() << "\nTry '" << argv[0] << " -h' for help\n\n";
        throw;
      }
      return true;
    }

    template <class C>
    int carp(C const& c,const char*progname="forest-em") const
    {
      log() << "ERROR: " << c << "\n\n";
      log() << "Try '" << progname << " -h' for documentation\n";
      BackTrace::print(log());
      return 1;
    }

    int main(int argc, char **argv)
    {
      const char *pr=argv[0];
      set_defaults();
      try {
        if (!parse_args(argc,argv))
          return 1;
      } catch(...) {
        return 1;
      }
      try {
        validate_parameters();
        run();
      }
      catch(std::bad_alloc& e) {
        return carp("ran out of memory\nTry descreasing -m or -M, and setting an accurate -P if you're using initial parameters.",pr);
      }
      catch(std::exception& e) {
        return carp(e.what(),pr);
      }
      catch(const char * e) {
        return carp(e,pr);
      }
      catch(...) {
        return carp("FATAL Exception of unknown type!",pr);
      }
      return 0;
    }

    ForestEmParams()
    {
      set_defaults();
    }

    void validate_parameters();
    template <class Float>
    void perform_forest_em();
    void run();
    inline std::ostream &log() const {
      return log_stream ? *log_stream : std::cerr;
    }
    inline static std::string get_version() {
      return FOREST_EM_VERSION FOREST_EM_COMPILED;
    }
    inline const char *float_typename() const {
      return double_precision ? "double" : "float";
    }
    inline void print(std::ostream &o) const {
      o << "forest-em-version={{{" << get_version() << "}}} floating-point-precision={{{" << float_typename() << "}}} forest-em-cmdline={{{"<<cmdline_str<<"}}}";
    }
    typedef ForestEmParams self_type;

  };
  inline std::ostream & operator << (std::ostream &o,const graehl::ForestEmParams &params) {
    params.print(o);
    return o;
  }

# ifndef GRAEHL__SINGLE_MAIN
  extern
#endif
  ForestEmParams forest_em_param;

} //ns


#ifdef GRAEHL__SINGLE_MAIN
# include <graehl/forest-em/forest-em-params.cpp>
#endif


#endif
