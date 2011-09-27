/* to decide:

   show0: make printing never drop 0 prob arcs, provide option to remove them explicitly (so that --fem-param etc. are always lined up) - for now just forcing print of 0 arcs
   -b batch compose: what should happen w/ cascade?  disallowed combination w/ --fem-param and many other options?

   avoid train or compose entirely when just writing -fem-param and -fem-norm?

   --save-fem-param after training?
 */

//#define MARCU
//TODO: training_progress(i) Log() ...... quiet mode for batch composition?
#define GRAEHL__SINGLE_MAIN
// -: = cache em derivations including reverse structure (uses more memory but faster iterations)
// -? = cache em derivations; currently limited to memory; should change to use disk
// -= 2.0  = square weights then normalize
// -o = learning rate exponent growth ratio (default 1.0)
// -K = don't assume state names are indexes if the final state is an integer
// -q = quiet (default logs computation progress)
// -w w = prune paths ratio (1 = keep only best path, 10 = keep paths up to 10 times worse)
// -z n = keep at most n states
// -U = treat pre-training weights as prior counts
// -Y write graphviz
// -1 = randomly scale weights (of unlocked arcs) after composition uniformly by (0..1]
// -@ = for -g, print input/output pairs
#include <graehl/shared/config.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cctype>
#include <string>
#include <ctime>
#include <graehl/carmel/src/fst.h>
#include <graehl/carmel/src/cascade.h>
#include <graehl/shared/myassert.h>
#include <graehl/shared/string_to.hpp>
#include <graehl/shared/split.hpp>
#include <boost/config.hpp>
#include <graehl/shared/random.hpp>

using namespace graehl;
#ifdef USE_OPENFST
# define OPENFST_VERSION "+openfst"
#else
# define OPENFST_VERSION ""
#endif

#define CARMEL_VERSION "6.12" OPENFST_VERSION

#ifdef MARCU
#include <graehl/carmel/src/models.h>
char *MarcuArgs[]={
    "marcu-carmel",
    "-IEsriqk",
    "1"
};
#endif

static unsigned seed;
static bool seed_shown=false;

static void show_seed()
{
    if (seed_shown) return;
    seed_shown=true;
    Config::log() << "Using random seed -R "<<seed<<endl;
}

static void setOutputFormat(bool *flags,ostream *fstout) {
    WFST::output_format(flags,fstout);
    return;
    /*
    if (fstout) {
    if ( flags['B'] )
        Weight::out_log10(*fstout);
    else if (flags['2'])
        Weight::out_ln(*fstout);
    else
        Weight::out_exp(*fstout);
    if ( flags['Z'] )
        Weight::out_always_log(*fstout);
    else
        Weight::out_sometimes_log(*fstout);
    if ( flags['D'] )
        Weight::out_never_log(*fstout);
    if ( flags['J'] )
        WFST::out_arc_full(*fstout);
    else
        WFST::out_arc_brief(*fstout);
    if ( flags['H'] )
        WFST::out_arc_per_line(*fstout);
    else
        WFST::out_state_per_line(*fstout);
    //    fstout->clear(); //FIXME: trying to eliminate valgrind uninit when doing output to Config::debug().  will this help?
    }
    if ( flags['B'] )
        Weight::default_log10();
    else if (flags['2'])
        Weight::default_ln();
    else
        Weight::default_exp();
    if ( flags['Z'] )
        Weight::default_always_log();
    else
        Weight::default_sometimes_log();
    if ( flags['D'] )
        Weight::default_never_log();
    WFST::set_arc_default_format(flags['J'] ? WFST::FULL : WFST::BRIEF);
    WFST::set_arc_default_per(flags['H'] ? WFST::ARC : WFST::STATE);
    //    fstout->clear(); //FIXME: trying to eliminate valgrind uninit when doing output to Config::debug().  will this help?
    */
}

static void printSeq(Alphabet<StringKey,StringPool> &a,int *seq,int maxSize) {

    for ( int i = 0 ; i < maxSize && seq[i] != 0; ++i) {
        if (i>0)
            cout << ' ';
        cout << a[seq[i]];
    }
}

template <class T>
void readParam(T *t, char const*from, char sw) {
    istringstream is(from);
    is >> *t;
    if ( is.fail() ) {
        Config::warn() << "Expected a number after -" << sw << " switch, (instead got \'" << from << "\' - as a number, " << *t << ").\n";
        exit(-1);
    }
}

template <class T>
bool readParam(bool *argflags,T *t, char const*from, char sw) {
    if (argflags[sw]) {
        argflags[sw]=false;
        readParam(t,from,sw);
        return true;
    }
    return false;
}

int isSpecial(const char* psz) {
    if ( !(*psz == '*') )
        return 0;
    while ( *++psz ) ;
    return *--psz == '*';
}


void outWithoutQuotes(const char *str, ostream &out) {
    if ( *str != '\"') {
        out << str;
        return;
    }
    const char *psz = str;
    while ( *++psz ) ;
    if ( *--psz == '\"' ) {
        while ( ++str < psz )
            out << *str;
    } else
        out << str;
}

void out_maybe_quote(char const* str,ostream &out,bool quote)
{
    if (quote)
        out << str;
    else
        outWithoutQuotes(str,out);
}


struct wfst_paths_printer {
    bool SIDETRACKS_ONLY;
    WFST &wfst;
    unsigned n_paths;
    Weight best_w;
    /*
    Weight w;
    ostream &out;
    bool *flags;
    typedef std::vector<int> Output;
    Output output;
    graehl::word_spacer sp;
    unsigned src; //FIXME: if sidetracks_only then you don't show the real arc source, but the end of last sidetrack.
    wfst_paths_printer(WFST &_wfst,ostream &_out,bool *_flags):wfst(_wfst),out(_out),flags(_flags) {
        n_paths=0;
        SIDETRACKS_ONLY=flags['%'];
        }*/
    WFST::path_print pp;
    wfst_paths_printer(WFST &_wfst,ostream &_out,bool *_flags)
        : SIDETRACKS_ONLY(_flags['%']),wfst(_wfst),pp(_flags) {
        pp.set_out(_out);
        n_paths=0;
    }
    void start_path(unsigned k,Weight path_w) { // called with k=rank of path (1-best, 2-best, etc.) and cost=sum of arcs from start to finish
        if (k==1)
            best_w=path_w;
        ++n_paths;
        /*
        w=path_w;
        output.clear();
        sp.reset();
        src=0;
        */
        pp.start(wfst);
    }
    void end_path() {
        pp.finish(wfst);
        /*
        if (flags['@'] ) {
            out << endl;
            sp.reset();
            for (Output::const_iterator i=output.begin(),e=output.end();i!=e;++i)
                out << sp << wfst.outLetter(*i);
            out << endl;
        } else {
            if (!flags['W'])
                out << sp << w;
            out << endl;
        }
        */
#ifdef DEBUGKBEST
        Config::debug() << endl;
#endif
    }
    void visit_best_arc(FSTArc &arc) {
        pp.arc(wfst,arc);
        /*
//        path.push_back(&arc);
        if (flags['@'] ) {
            int inid=arc.in,outid=arc.out;
            if (outid!=WFST::epsilon_index)
                output.push_back(outid);
            if (inid!=WFST::epsilon_index)
                out << sp << wfst.inLetter(inid);
        } else {
            if ( flags['O'] || flags['I'] ) {
                int id = flags['O'] ? arc.out : arc.in;
                if ( !(flags['E'] && id==WFST::epsilon_index) ) {
                    out << sp;
                    out_maybe_quote(flags['O'] ? wfst.outLetter(id) : wfst.inLetter(id),out,!flags['Q']);
                }
            } else {
                out << sp;
                wfst.printArc(arc,src,out);
            }
        }
        src=(unsigned)arc.dest;
        */
    }
    void visit_sidetrack_arc(FSTArc &a) {
        visit_best_arc(a);
    }

};

void printPath(bool *flags,const List<PathArc> *pli) {
    /*
    Weight w = 1.0;
    const char * outSym;

    for (List<PathArc>::const_iterator li=pli->const_begin(),end=pli->const_end(); li != end; ++li ) {
        const WFST *f=li->wfst;
        if ( flags['O'] || flags['I'] ) {
            if ( flags['O'] )
                outSym = f->outLetter(li->out);
            else
                outSym = f->inLetter(li->in);
            if ( !(flags['E'] && isSpecial(outSym)) ) {
                if ( flags['Q'] )
                    outWithoutQuotes(outSym, cout);
                else
                    cout << outSym;
                cout << " ";
            }
        } else {
            cout << *li << " ";
        }
        w = w * li->weight;
    }
    if ( !flags['W'] )
        cout << w;
    cout << "\n";
    */
    if (pli->empty()) cout<<"\n";
    else {
        WFST const&w=*pli->front().wfst;
        WFST::path_print pp(flags);
        pp.set_out(cout);
/*        pp.start(w);
        for (List<PathArc>::const_iterator li=pli->const_begin(),end=pli->const_end(); li != end; ++li )
            pp.arc(*li);
        pp.finish(w);
*/
        pp(w,*pli);
    }
}

void usageHelp(void);
void WFSTformatHelp(void);

typedef std::map<std::string,double> long_opts_t;
typedef std::map<std::string,std::string> text_long_opts_t;

struct carmel_main
{
    ifstream post_b;
    graehl::gibbs_opts gopt;
    WFST::path_print printer;
    WFST::train_opts topt;

    void force_cascade_derivs()
    {
        flags['t']=true;
        long_opts["train-cascade"]=1;
    }

    bool parse_cache_opts()
    {
        WFST::deriv_cache_opts &copt=topt.cache;
        copt.cache_level=have_opt("matrix-fb") ? WFST::matrix_fb : (flags[':'] ? WFST::cache_forward_backward : (flags['?'] ? WFST::cache_forward : WFST::cache_nothing));
        copt.do_prune=!have_opt("cache-no-prune");
        if (have_opt("disk-cache-derivations")) {
            copt.cache_level=WFST::cache_disk;
            copt.disk_cache_filename=set_default_text("disk-cache-derivations","/tmp/carmel.derivations.XXXXXX");
            get_default_opt("disk-cache-bufsize",copt.disk_cache_bufsize,"1M");
            Config::log()<<"Disk cache of derivations will be created at "<<copt.disk_cache_filename<<" using read buffer of "<<copt.disk_cache_bufsize<<" bytes.\n";
        }
    }

    bool gibbs;

    void parse_gibbs_opts()
    {
        printer.set_flags(flags);
        gibbs = have_opt("crp");
        if (gibbs) {
            unsigned m;
            get_opt("crp",m);
            if (m>1) {
                Config::log()<<"Replacing -M "<<topt.max_iter<<" with --crp="<<m<<endl;
                topt.max_iter=m;
            }
            if (topt.cache.cache_level==WFST::cache_nothing)
                topt.cache.cache_level=(gopt.expectation?WFST::cache_forward_backward:WFST::cache_forward);
            force_cascade_derivs();
            gopt.expectation=have_opt("expectation");
            gopt.include_self=have_opt("include-self");
            gopt.random_start=have_opt("random-start");
            get_opt("crp-restarts",gopt.restarts);
            gopt.argmax_final=have_opt("crp-argmax-final");
            gopt.argmax_sum=have_opt("crp-argmax-sum");
            gopt.norm_order=have_opt("norm-order");
            gopt.exclude_prior=have_opt("crp-exclude-prior");
            get_opt("init-em",gopt.init_em);
            gopt.em_p0=have_opt("em-p0");
            get_opt("burnin",gopt.burnin);
            get_opt("width",gopt.width);
            get_opt("print-from",gopt.print_from);
            get_opt("print-to",gopt.print_to);
            get_opt("print-norms-from",gopt.print_norms_from);
            get_opt("print-norms-to",gopt.print_norms_to);
            get_opt("print-counts-from",gopt.print_counts_from);
            get_opt("print-counts-to",gopt.print_counts_to);
            get_opt("print-counts-sparse",gopt.print_counts_sparse);
            get_opt("print-counts-rich",gopt.rich_counts);
            get_opt("print-every",gopt.print_every);
            get_opt("high-temp",gopt.high_temp);
            get_opt("low-temp",gopt.low_temp);
            gopt.prior_inference_restart_fresh=have_opt("prior-inference-restart-fresh");
            gopt.prior_inference_show=have_opt("prior-inference-show");
            gopt.prior_inference_global=have_opt("prior-inference-global");
            get_opt("prior-inference-stddev",gopt.prior_inference_stddev);

//        gopt.cache_prob=have_opt("cache-prob");
            gopt.cheap_prob=have_opt("sample-prob");
            if (!(gopt.cache_prob || gopt.cheap_prob)) gopt.no_prob=have_opt("no-prob");
            gopt.uniformp0=have_opt("uniform-p0");
            gopt.init_from_p0=have_opt("init-from-p0");
            gopt.dirichlet_p0=have_opt("dirichlet-p0");
            gopt.final_counts=have_opt("final-counts");
        }
    }

    void log_ppx(double n_pairs,Weight prod_prob,unsigned n_0prob=0)
    {
        std::ostream &o=Config::log();
        o<<"product of probs="<<prod_prob<<", ";
        prod_prob.print_ppx(o,n_symbols,n_pairs,"per-input-symbol-perplexity","per-line-perplexity");
        /*
          o<<"per-example perplexity(N="<<n_pairs<<")=";
          prod_prob.root(n_pairs).inverse().print_base(o,2);
          o<<"per-input-symbol perplexity(Nsym="<<n_symbols<<")=";
          prod_prob.root(n_symbols).inverse().print_base(o,2);
        */
        if (n_0prob)
            o<<", excluding "<<n_0prob<<" 0 probabilities (i.e. real ppx is infinite).";
        o<<std::endl;
    }

    istream *open_postb()
    {
        if (have_opt("post-b")) {
            post_b.open(text_long_opts["post-b"].c_str(),ifstream::in);
            return &post_b;
        } else
            return NULL;
    }

    Weight prod_viterbi;
    Weight prod_sum;
    unsigned n_0prob;
    unsigned n_prob;
    unsigned n_lines() const { return n_0prob+n_prob; }
    unsigned n_pre_lines() const
    {
        return pre_n_0prob+pre_n_prob;
    }
    unsigned pre_n_0prob;
    unsigned pre_n_prob;
    Weight prod_sum_pre; // for post-b
//    double n_isymbols;
    double n_symbols;

    void non0_viterbi_prob(Weight p)
    {
        ++n_prob;
        prod_viterbi*=p;
    }

    void report_0probs(char const* name,unsigned n,unsigned N) const
    {
        std::ostream &o=Config::log();
        if (n)
            o << "No derivations found for "<<n<<" of ";
        else
            o << "Derivations found for all ";
        o<<N<<" "<<name<<"\n";

    }

    void report_batch()
    {
        std::ostream &o=Config::log();
        bool postb=have_opt("post-b");
        bool sump=have_opt("sum");
        unsigned N=n_lines();
        unsigned Npre=n_pre_lines();
        if (Npre) {
            report_0probs(postb?"inputs just before --post-b":" inputs",pre_n_0prob,Npre);
            if (postb) o << "Just before --post-b, sum-all-paths ";
            log_ppx(n_prob,prod_sum_pre,pre_n_0prob);
        }
        if (!N) return;
        report_0probs("inputs",n_0prob,N);
        o << "Viterbi (best path) ";
        log_ppx(n_prob,prod_viterbi,n_0prob);
        if (sump) {
            o << "Sum (all paths) ";
            log_ppx(n_prob,prod_sum,n_0prob);
            if (postb) {
                o << "Conditional (final divided by previous sum-all-paths) ";
                log_ppx(n_prob,prod_sum/prod_sum_pre,n_0prob);
            }
        }
    }

    void print_kbest(unsigned kPaths,WFST *result)
    {
        unsigned kPathsLeft=kPaths;
        if ( result->valid() ) {
            wfst_paths_printer pp(*result,cout,flags);
            result->visit_kbest(kPaths,pp);
            kPathsLeft -= pp.n_paths;
            if (pp.best_w.isZero())
                ++n_0prob;
            else {
                non0_viterbi_prob(pp.best_w);
            }
        } else {
            n_0prob++;
        }
        for ( unsigned fill = 0 ; fill < kPathsLeft ; ++fill ) {
            if ( !(flags['W']||flags['@']) )
                cout << 0;
            cout << "\n";
        }
    }

    bool *flags;
    long_opts_t &long_opts;
    text_long_opts_t &text_long_opts;
    Weight prune_wt;
    char const**filenames;
    unsigned nInputs;
    void set_inputs(char const** fnm, unsigned N)
    {
        filenames=fnm;
        nInputs=N;
    }



    typedef WFST::NormalizeMethod NM;
    NM norm_method;
    typedef dynamic_array<NM> NMs;
    NMs nms;
    template <class V>
    struct field
    {
        typedef V arg_type;
        static void set(V &v,V const& to) {v=to;}
        static V const& get(V const&v) { return v; }
    };
    /// a cell acts like a value V object but once constructed, copies of cell have ref to same object
    template <class V>
    struct cell
    {
        V *v;
        cell() : v(new V()) {}
        template <class C0>
        cell(C0 const& c0) : v(new V(c0)) {}
        template <class C0,class C1>
        cell(C0 const& c0,C1 const& c1) : v(new V(c0,c1)) {}
        ~cell() {delete v;}
        operator V &() { return *v; }
        operator V const&() const { return *v; }
    };
    template <class O,class Setter>
    struct Putter
    {
        typedef typename Setter::arg_type arg_type;
        cell<O> o;
        Putter(O o) : o(o) {}
        template <class Str>
        bool operator()(Str const& s)
        {
            Setter::set(*o,string_to<arg_type>(s));
            ++o;
            return true;
        }
    };
    template <class S,class O>
    Putter<O,S> make_putter(O const& o)
    {
        return Putter<O,S>(o);
    }

    template <class Setter,class V>
    unsigned set_vector(std::string const& key,dynamic_array<V> &pr,char const* osep = " ^ ",char const* isep=",")
    {
        std::ostream &o=Config::log();
        std::string const& opt=text_long_opts[key];
        if (!opt.empty()) {
            unsigned n=split_noquote(opt,make_putter<Setter>(pr.begin()),isep,pr.size(),false,true);
            if (n>0) {
              o << "Using input WFST --"<<key<<":\n";
              unsigned i=0;
              for (i=0;i<n;++i) {
                if (i>=nInputs) {
                  std::cerr<<"#inputs "<<nInputs<<" "<<key<<"="<<opt<<std::endl;
                  throw std::runtime_error("Too many "+key+" for #inputs");
                }
                o << filenames[i];
                o << osep << Setter::get(pr[i]);
                o << std::endl;
              }
              o << std::endl;
              return n;
            }
        }
        return 0;
    }
    template <class V>
    void parse_vector(std::string const& key,dynamic_array<V> &pr,V const& zero=0,char const* osep = " ^ ",char const* isep=",")
    {
        pr.resize(nInputs);
        unsigned i=set_vector<field<V> >(key,pr,osep,isep);
        while (i<nInputs)
            pr[i++]=zero;
    }

    bool real_cascade() {
        return long_opts["train-cascade"] || long_opts["compose-cascade"];
    }

    NMs & norms()
    {
        unsigned N=real_cascade()?nInputs:1;
        nms.clear();
        if (gopt.prior_inference_stddev)
            if (norm_method.add_count<=0)
                norm_method.add_count=1;
        nms.push_back_n(norm_method,N);
        if (have_opt("normby"))
            set_vector<NM::f_group>("normby",nms," norm by ","");
        set_vector<NM::f_scale>("digamma",nms," digamma ",",");
        set_vector<NM::f_prior>("priors",nms," alpha ",",");
        set_vector<NM::f_priorgroup>("prior-groupby",nms," prior-inference-stddev prior inference scaling ","");
        return nms;
    }

    Weight keep_path_ratio;
    int max_states;

    bool have_opt(std::string const&key) const
    {
        return long_opts.find(key)!=long_opts.end();
    }

    template <class V>
    bool get_opt(std::string const&key,V &v) const
    {
        long_opts_t::const_iterator i=long_opts.find(key);
        if (i==long_opts.end()) return false;
        v=(V)i->second;
        return true;
    }

    bool set_text(std::string const& key,std::string &to)
    {
        return !(to=text_long_opts[key]).empty();
    }

    std::string const& set_default_text(std::string const& key,std::string const& default_val="")
    {
        std::string &val=text_long_opts[key];
        if (val.empty())
            val=default_val;
        return val;
    }

    template <class V>
    V const& get_default_opt(std::string const& key,V &v,std::string const& default_val)
    {
        return string_into(set_default_text(key,default_val),v);
    }


    carmel_main(bool *flags,long_opts_t &long_opts,text_long_opts_t &text_long_opts)
        :flags(flags),long_opts(long_opts),text_long_opts(text_long_opts),fems(true,false)
    {
        set_defaults();
    }

    bool prunePath() const
    {
        return flags['w'] || flags['z'];
    }

    void normalize(WFST *result)
    {
        result->normalize(norm_method);
    }

    void post_train_normalize(WFST *result)
    {
        if (flags['t']&&(flags['p'] || prunePath()))
            result->normalize(norm_method);

    }

    void maybe_constant_weight(WFST *result)
    {
        Weight c;
        if (get_opt("constant-weight",c)) {
            Config::log() << "Setting all (non-locked) arcs in result to weight "<<c<<std::endl;
            result->set_constant_weights(c);
        }
    }

    void maybe_project(WFST *result)
    {
        bool id=long_opts["project-identity-fsa"];
        if (long_opts["project-left"])
            result->project(State::input,id);
        if (long_opts["project-right"])
            result->project(State::output,id);
    }

    bool post_compose(WFST *&result)
    {
        bool sump=have_opt("sum");
        Weight s=1;

        if (sump) {
            s=result->sum_acyclic_paths();
            if (s.isZero())
                ++pre_n_0prob;
            else
                ++pre_n_prob;
        }

        prod_sum_pre*=s;

        if (have_opt("post-b")) {
            post_b >> ws;
            std::string buf;
            getline(post_b,buf);
            if (!post_b) {
                Config::warn() << "--post-b file didn't have as many lines as -b file.\n";
                return false;
            }
            WFST pb(buf.c_str());
            if ( !(pb.valid()) ) {
                Config::warn() << "For --post-b="<<long_opts["post-b"]<<", couldn't handle input line: " << buf << "\n";
                return false;
            }
            WFST *p=result;
            if (flags['r'])
                result=new WFST(pb,*p,flags['m'],flags['a']);
            else
                result=new WFST(*p,pb,flags['m'],flags['a']);
            if (!result->valid()) {
                ++n_0prob;
                return false;
            }
            result->ownAlphabet();
            delete p;
            if (sump) {
                s=result->sum_acyclic_paths();
            }
        }

        prod_sum*=s;


        maybe_constant_weight(result);
        maybe_sink(result);
        if ( flags['v'] )
            result->invert();
        if ( flags['1'] ) {
            show_seed();
            result->randomScale();
        }
        if ( long_opts["random-set"] ) {
            show_seed();
            result->randomSet();
        }
        if (flags['n'])
            normalize(result);

        return true;
    }

    void maybe_sink(WFST *result)
    {
        if ( long_opts["final-sink"] )
            result->ensure_final_sink();
    }


    void write_transducer(std::ostream &o,WFST *result)
    {
//        if (long_opts["test-as-pairs"])  WFST::as_pairs_fsa(*result,long_opts["test-as-pairs-epsilon"]);
        maybe_project(result);
        if ( flags ['Y'] )
            result->writeGraphViz(o);
        else {
            result->writeLegible(o,show0);
        }
    }

    void prune(WFST *result)
    {
        if ( flags['p'] )
            result->pruneArcs(prune_wt);
        if ( prunePath() )
            result->prunePaths(max_states,keep_path_ratio);
    }

    // return true if # of arcs change
    bool minimize(WFST *result)
    {
        unsigned n=result->numArcs();
        if ( flags['C'] )
            result->consolidateArcs(!long_opts["consolidate-max"],!long_opts["consolidate-unclamped"]);
        if ( !flags['d'] )
            result->reduce();
        return result->numArcs()!=n;
    }

    struct shrink_monitor
    {
        shrink_monitor(char const* name,WFST &result,bool print,bool &changed)
            : name(name),result(result),print(print),changed(changed)
        {
            st=result.size();
            arc=result.numArcs();
        }
        char const* name;
        WFST &result;
        bool print;
        bool &changed;
        unsigned st,arc;
        ~shrink_monitor()
        {
            unsigned nst=result.size(),narc=result.numArcs();
            if (nst!=st || narc!=arc) {
                changed=true;
                if (print)
                    Config::log() << ' '<<name<<"-> "<<nst<<'/'<<narc;
            }
        }

    };

    bool shrink(WFST *result,bool print=true,bool do_prune=true,bool openfst_min=false,char const* end="\n")
    {
        WFST &w=*result;
        bool changed=false;
        print=print&&!flags['q'];
        {
            shrink_monitor m("reduce",w,print,changed);
            minimize(result);
        }
        if (do_prune) {
            shrink_monitor m("prune",w,print,changed);
            prune(result);
        }
        if (openfst_min) {
            shrink_monitor m("openfst-minimize",w,print,changed);
            openfst_minimize(result);
        }
        if (print) Config::log()<<end;
        return changed;
    }

    template <class OpenFST>
    void openfst_minimize_type(WFST *result)
    {
#ifdef USE_OPENFST
        if (!flags['q'])
            Config::log() << " openfst " <<
                (long_opts["minimize-sum"]?"sum ":"tropical ")<<"minimize: "<<result->size()<<"/"<<result->numArcs();
        if (!result->minimize_openfst<OpenFST>(
                long_opts["minimize-determinize"] || long_opts["minimize-determinize-only"]
                , long_opts["minimize-rmepsilon"]
                ,!long_opts["minimize-determinize-only"]
                , !long_opts["minimize-no-connect"]
                , long_opts["minimize-inverted"]
                , long_opts["minimize-pairs"]
                , !long_opts["minimize-pairs-no-epsilon"]
                )
            )
            Config::log() << " (FST not input-determinized, try --minimize-determinize, which may not terminate)";
        if (!flags['q']) Config::log() << " minimized-> " << result->size() << "/" << result->numArcs() << "\n";
#endif
    }

    void openfst_minimize(WFST *result)
    {
#ifdef USE_OPENFST
        if (long_opts["minimize-sum"])
            openfst_minimize_type<fst::VectorFst<fst::LogArc> >(result);
        else
            openfst_minimize_type<fst::StdVectorFst>(result);
#endif
    }

    void openfst_roundtrip(WFST *result)
    {
#ifdef USE_OPENFST
        if (!flags['q']) Config::log() << "performing (meaningless test) carmel->openfst->carmel round trip\n";
        result->roundtrip_openfst<fst::StdVectorFst>();
#endif
    }

    cascade_parameters fems; // remembers chain no matter whether --train-cascade is enabled
    dynamic_array<std::string> fem_filenames;


    std::string fem_norm,fem_forest,fem_inparam,fem_outparam,fem_suffix,fem_early_outparam,fem_alpha;

    bool no_compose;

    bool show0;

    void set_defaults()
    {
        show0=false;
        gibbs=false;
        norm_method.group=WFST::CONDITIONAL;
        keep_path_ratio.setInfinity();
        max_states=WFST::UNLIMITED;
        n_0prob=0;
        n_prob=0;
        pre_n_0prob=0;
        pre_n_prob=0;
        n_symbols=0; // for per-symbol ppx - counted only for non-0prob
//        n_isymbols=0;
        prod_viterbi=1;
        prod_sum=1;
        prod_sum_pre=1;
        number_from=0;
        no_compose=false;
    }

    void parse_opts()
    {
        parse_cache_opts();
        parse_gibbs_opts();
        parse_fem_opts();
        no_compose=have_opt("no-compose");
    }

    void parse_fem_opts()
    {
        set_text("load-fem-param",fem_inparam);
        set_text("write-loaded",fem_suffix);
        show0|=set_text("fem-param",fem_outparam);
        show0|=set_text("fem-early-param",fem_early_outparam);
        show0|=set_text("fem-alpha",fem_alpha);
        show0|=set_text("fem-norm",fem_norm);
        show0|=set_text("fem-forest",fem_forest);
        if (!fem_forest.empty()) {
            topt.cache.out_derivfile=fem_forest;
            force_cascade_derivs();
        }
        show0|=get_opt("number-from",number_from);
    }

    unsigned number_from;

    void fem_normby()
    {
        if (have_opt("normby")) {
            Config::log()<<"Normalizing input transducers by --normby="<<text_long_opts["normby"]<<endl;
            fems.normalize(nms);
        }
    }

    void fem_add(WFST *w,char const* filename)
    {
        unsigned i=fems.size();
        fems.add(w);
        fem_filenames.push_back(filename);
    }

    void fem_in()
    {
        if (long_opts["random-set"]) {
            show_seed();
            fems.randomize(nms);
        }
        if (long_opts["constant-set"]) {
        }
        if (!fem_inparam.empty()) {
            Config::log()<<"Reading cascade weights from --load-fem-param="<<fem_inparam<<endl;
            std::ifstream i(fem_inparam.c_str());
            if (!i) {
                throw std::runtime_error("Missing --load-fem-param file.\n");
            }
            fems.read_params(i);
        }
        fem_normby();
        fem_out_param(fem_early_outparam);
        if (number_from>0) {
            Config::log()<<"Assigning unique group ids to each arc in input cascade starting at "<<number_from<<".\n";
            fems.number_from(number_from);
        }
        if (have_opt("write-loaded"))
            write_trained(fem_suffix);
    }

    void fem_out_param(std::string const& out)
    {
        if (!fem_outparam.empty()) {
            Config::log()<<"Writing cascade weights to --fem-param="<<fem_outparam<<endl;
            std::ofstream o(fem_outparam.c_str());
            fems.print_params(o);
        }
    }

    void fem_out()
    {
        fem_out_param(fem_outparam);
        if (!fem_norm.empty()) {
            Config::log()<<"Writing forest-em normgroups to --fem-norm="<<fem_norm<<endl;
            std::ofstream o(fem_norm.c_str());
            fems.fem_norms(o,nms);
        }
        if (!fem_alpha.empty()) {
            Config::log()<<"Writing forest-em alpha to --fem-alpha="<<fem_alpha<<endl;
            std::ofstream o(fem_alpha.c_str());
            fems.fem_alpha(o,nms);
        }
    }


    void fem_stats()
    {
        for (unsigned i=0,N=fems.size();i<N;++i) {
            Config::log()<<"\n";
            stats(Config::log(),fems.cascade[i],fem_filenames[i]);
        }
    }


    void stats(std::ostream &out,WFST *w,std::string const& name)
    {
        out << "Number of states in "<<name<<": " << w->size() << std::endl;
        out << "Number of arcs in "<<name<<": " << w->numArcs() << std::endl;
        if (w->numArcs()) {
            unsigned n_back;
            out << "Number of paths in "<<name<<" (valid for acyclic only; a cycle means infinitely many): " << w->numNoCyclePaths(&n_back) << std::endl;
            if (n_back)
                out<<"Number of cycle-causing arcs in "<<name<<": " << n_back;
            else
                out << name<<" is acyclic.";
        }
        out <<std::endl;
    }

    void write_trained(std::string const& suffix="trained")
    {
        fems.write_trained(suffix,flags,fem_filenames.begin(),show0);
    }

};



#ifndef TEST
int
#ifdef _MSC_VER
__cdecl
#endif
main(int argc, char *argv[]){
    try {
    INITLEAK;
#ifdef _MSC_VER
#ifdef MEMDEBUG
    //int tmpFlag = CrtSetDbgFlag(CRTDBGREPORTFLAG);
    //tmpFlag |= CRTDBGCHECKALWAYSDF;
    //CrtSetDbgFlag(tmpFlag);
#endif
#endif
#ifdef MARCU
    argc=sizeof(MarcuArgs)/sizeof(char *);
    argv=MarcuArgs;
#endif
    if ( argc == 1 ) {
        usageHelp();
        return 0;
    }
    int i;
    bool flags[256];
    bool argflags[256];
    for ( i = 0 ; i < 256 ; ++i ) argflags[i]=flags[i] = 0;
    char *pc;
    char const**parm = NEW char const *[argc-1];
    seed = default_random_seed();//(unsigned int )std::time(NULL);
    int nParms = 0;
    int kPaths = 0;
    int thresh = 32;
    Weight converge = 1E-4;
    Weight converge_pp_ratio = .999;
    bool converge_pp_flag = false;
    int convergeFlag = 0;
    Weight smoothFloor;
    Weight prod_prob=1;
    int n_pairs=0;
    int pruneFlag = 0;
    int floorFlag = 0;
    bool seedFlag = false;
    bool wrFlag = false;
    bool msFlag = false;
    bool learning_rate_growth_flag = false;
    int nGenerate = 0;
#define DEFAULT_MAX_GEN_ARCS 1000
    int maxGenArcs = 0;
    int labelStart = 0;
    int labelFlag = 0;
    bool rrFlag=false;
    bool isInChain;
    ostream *fstout = &cout;
    bool mean_field_oneshot_flag=false;
    bool exponent_flag=false;
    double exponent=1;
    long_opts_t long_opts;
    text_long_opts_t text_long_opts;

    carmel_main cm(flags,long_opts,text_long_opts);
    WFST::train_opts &train_opt=cm.topt;

    std::ios_base::sync_with_stdio(false);

    for ( i = 1 ; i < argc ; ++i ) {
        char *arg=argv[i];
        if (exponent_flag) {
            exponent_flag=false;
            readParam(&exponent,arg,'=');
        } else if ( labelFlag ) {
            labelFlag = 0;
            readParam(&labelStart,arg,'N');
        } else if ( converge_pp_flag ) {
            converge_pp_flag = false;
            readParam(&converge_pp_ratio,arg,'X');
        } else if ( learning_rate_growth_flag ) {
            learning_rate_growth_flag = false;
            readParam(&train_opt.learning_rate_growth_factor,arg,'o');
            if (train_opt.learning_rate_growth_factor < 1)
                train_opt.learning_rate_growth_factor=1;
        } else if (rrFlag) {
            rrFlag=false;
            readParam(&train_opt.ran_restarts,arg,'!');
        } else if (seedFlag) {
            seedFlag=false;
            readParam(&seed,arg,'R');
        } else if ( wrFlag ) {
            readParam(&cm.keep_path_ratio,arg,'w');
            if (cm.keep_path_ratio < 1)
                cm.keep_path_ratio = 1;
            wrFlag=false;
        } else if ( msFlag ) {
            readParam(&cm.max_states,arg,'z');
            if ( cm.max_states < 1 )
                cm.max_states = 1;
            msFlag=false;
        } else if ( kPaths == -1 ) {
            readParam(&kPaths,arg,'k');
            if ( kPaths < 1 )
                kPaths = 1;
        } else if ( nGenerate == -1 ) {
            readParam(&nGenerate,arg,'g');
            if ( nGenerate < 1 )
                nGenerate = 1;
        } else if ( readParam(argflags,&train_opt.max_iter,arg,'M')) {
        } else if ( maxGenArcs == -1 ) {
            readParam(&maxGenArcs,arg,'L');
            if ( maxGenArcs < 0 )
                maxGenArcs = 0;
        } else if ( thresh == -1 ) {
            readParam(&thresh,arg,'T');
            if ( thresh < 0 )
                thresh = 0;
        } else if ( convergeFlag ) {
            convergeFlag = 0;
            readParam(&converge,arg,'e');
        } else if ( floorFlag ) {
            floorFlag = 0;
            readParam(&smoothFloor,arg,'f');
        } else if ( pruneFlag ) {
            pruneFlag = 0;
            readParam(&cm.prune_wt,arg,'p');
        } else if ( fstout == NULL ) {
            fstout = NEW ofstream(arg);
            setOutputFormat(flags,fstout);
            if ( !*fstout ) {
                Config::warn() << "Could not create file " << arg << ".\n";
                return -8;
            }
        } else if (mean_field_oneshot_flag) {
            mean_field_oneshot_flag=0;
            cm.norm_method.scale.linear=false;
            readParam(&cm.norm_method.scale.alpha,arg,'+');
        } else if ((pc=arg)[0] == '-' && pc[1] != '\0') { // && !learning_rate_growth_flag && !convergeFlag && !floorFlag && !pruneFlag && !labelFlag && !converge_pp_flag && !wrFlag && !msFlag && !mean_field_oneshot_flag && !exponent_flag
            if (pc[1]=='-') {
                // long option
                char * s=pc+2;
                char  *e=s;
                bool have_val=false;
                for(;*e;++e)
                    if (*e== '=' ) {
                        have_val=true;
                        break;
                    }
                std::string key(pc+2,e);
                std::string val;
                double v=1;
                if (have_val) {
                    val=(e+1);
                    text_long_opts[key]=val;
                    istringstream is(val);
                    is >> v;
                    if (is.fail())
                        v=-1;
                }
                long_opts[key]=v;
                cerr << "option " << key << " = " << val << endl;
            } else {
                while ( *(++pc) ) {
                    if ( *pc == 'k' )
                        kPaths = -1;
                    else if ( *pc == '!' )
                        rrFlag=true;
                    else if ( *pc == 'o' )
                        learning_rate_growth_flag=true;
                    else if ( *pc == 'X' )
                        converge_pp_flag=true;
                    else if ( *pc == 'w' )
                        wrFlag=true;
                    else if ( *pc == 'z' )
                        msFlag=true;
                    else if ( *pc == 'R' )
                        seedFlag=true;
                    else if ( *pc == 'F' )
                        fstout = NULL;
                    else if ( *pc == 'T' )
                        thresh = -1;
                    else if ( *pc == 'e' )
                        convergeFlag = 1;
                    else if ( *pc == 'f' )
                        floorFlag = 1;
                    else if ( *pc == 'p' )
                        pruneFlag = 1;
                    else if ( *pc == 'g' || *pc == 'G' )
                        nGenerate = -1;
                    else if ( *pc == 'M' )
                        train_opt.max_iter=-1;
                    else if ( *pc == 'L' )
                        maxGenArcs = -1;
                    else if ( *pc == 'N' )
                        labelFlag = 1;
                    else if ( *pc == '+' )
                        mean_field_oneshot_flag=true;
                    else if ( *pc == 'j' )
                        cm.norm_method.group = WFST::JOINT;
                    else if ( *pc == 'u' )
                        cm.norm_method.group = WFST::NONE;
                    else if ( *pc == '=' )
                        exponent_flag=true;
                    flags[*pc] = 1;
                    argflags[*pc] = 1;
                }
            }
        } else {
            parm[nParms++] = arg;
        }
    }
    if (cm.have_opt("help")) {
        usageHelp();
        WFSTformatHelp();
        return 0;
    }
    bool prunePath = flags['w'] || flags['z'];

    srand(seed);
    set_random_seed(seed);
    setOutputFormat(flags,0); // set default for all streams
    setOutputFormat(flags,&cout);
    setOutputFormat(flags,&cerr);
    WFST::setIndexThreshold(thresh);
    if ( flags['h'] ) {
        cout << endl << endl;
        usageHelp();
        return 0;
    }
    if (flags['V']){
        std::cout << "Carmel Version: " << CARMEL_VERSION ;
        std::cout << ". Copyright C " << COPYRIGHT_YEAR << ", the University of Southern California.\n";
        return 0 ;
    }
    if ( flags['b'] && kPaths < 1 )
        kPaths = 1;
    istream **inputs, **files, **newed_inputs;
    char const**filenames;
    char const**alloced_filenames;
    int nInputs,nChain;
#ifdef MARCU
    initModels();
    int nModels=(int)Models.size();
#endif
    if ( flags['s'] ) {
        nChain=nInputs = nParms + 1;
#ifdef MARCU
        nChain+=(int)Models.size();
#endif
        newed_inputs = inputs = NEW istream *[nInputs];
        alloced_filenames = filenames = NEW char const*[nChain];
        if ( flags['r'] ) {
            inputs[nParms] = &cin;
            filenames[nParms] = "stdin";
            for ( i = 0 ; i < nParms ; ++i )
                filenames[i] = parm[i];
            files = inputs;
        } else {
            inputs[0] = &cin;
            filenames[0] = "stdin";
            for ( i = 0 ; i < nParms ; ++i )
                filenames[i+1] = parm[i];
            files = inputs + 1;
        }
    } else {
        nChain=nInputs = nParms;
        newed_inputs = inputs = NEW istream *[nInputs];
        files = inputs;
        filenames = parm;
    }
    istream *pairStream = NULL;
    cm.parse_opts();
    bool gibbs = cm.gibbs;
    cascade_parameters cascade(cm.real_cascade(),(unsigned)long_opts["debug-cascade"]);

    bool trainc=!cascade.trivial;
    if (trainc)
        flags['t']=1;
    if ( flags['t'] )
        flags['S'] = 1;
    if ( nChain < 1 || flags['A'] && nInputs < 2) {
        Config::warn() << "No inputs supplied.\n";
        return -12;
    }
    for ( i = 0 ; i < nParms ; ++i ) {
        //    if(parm[i][0]=='-' && parm[i][1] == '\0')
        //              files[i] = &cin;
        //      else
        files[i] = NEW ifstream(parm[i]);
#ifdef DEBUG
        //Config::debug() << "Created file " << i << " from " << parm[i] << " & " << files[i] <<"\n";
#endif
        if ( !*files[i] ) {
            Config::warn() << "File " << parm[i] << " could not be opened for input.\n";
            for ( int j = 0 ; j <=i ; ++j )
                delete files[j];
            delete[] parm;
            delete[] inputs;
            return -9;
        }
    }
    if ( flags['S'] ) {
        flags['b'] = flags['x'] = flags['y'] = 0;
        kPaths = 0;
        if (nInputs > 1) {
            --nInputs;
            --nChain;
            if ( flags['r'] )
                pairStream = inputs[nInputs];
            else {
                pairStream = inputs[0];
                ++filenames;
                ++inputs;
            }
        }
        if ( flags['s'] )
            ++files;
        else
            --nParms;
    }
    cm.open_postb();
    dynamic_array<double> exponents;
    cm.set_inputs(filenames,nInputs);
    cm.parse_vector("exponents",exponents,1.0," ^ ",",");

    carmel_main::NMs &nms=cm.norms();

    WFST *chainMemory = (WFST*)::operator new(nChain * sizeof(WFST));
    WFST *chain = chainMemory;
#ifdef MARCU
    chain+=nModels;
#endif
    int nTarget = -1; // chain[nTarget] is the linear acceptor built from input sequences
    istream *line_in=NULL;
    if ( flags['i'] || flags['b']||flags['P']) {// flags['P'] similar to 'i' but instead of simple transducer, produce permutation lattice.
        nTarget = flags['r'] ? nInputs-1 : 0;
        line_in=inputs[nTarget];
    }
    for ( i = 0 ; i < nInputs ; ++i ) {
        if ( i != nTarget ) {
            WFST *w=chain+i;
            PLACEMENT_NEW (w) WFST(*inputs[i],!flags['K']);
            cm.fem_add(w,filenames[i]);
            if (i < exponents.size())
                w->raisePower(exponents[i]);
            if ( !flags['m'] && nInputs > 1 )
                w->unNameStates();
            if ( inputs[i] != &cin ) {
#ifdef DEBUG
                //Config::debug() << "Deleting file " << i << " & " << inputs[i] <<"\n";
#endif

                delete inputs[i];
            }
            if ( !(w->valid()) ) {
                Config::warn() << "Bad format of transducer file: " << filenames[i] << "\n";
                //        for ( j = i+1 ; j < nInputs ; ++j )
                //          if ( inputs[i] != &cin )
                //            delete inputs[i];
                return -2;
            }
        }
    }

    string buf ;

    WFST *result = NULL;
    WFST *weightSource = NULL; // assign waits from this transducer to result by tie groups
    if ( flags['A'] ) {
        --nInputs;
        --nChain;
        if ( !flags['r'] ) {
            weightSource = chain;
            ++chain;
        } else
            weightSource = chain + nInputs;
    }
    int input_lineno=0;

#ifdef MARCU
    //chain,inputs,filenames,nTarget,nInputs
    for (int i=0,j=nModels;i<nInputs;++i,++j)
        filenames[j]=filenames[i];
    chain-=nModels;
    nTarget+=nModels;
    for (int j=0;j<nModels;++j) {
        PLACEMENT_NEW (&chain[j]) WFST(Models[j],!flags['K']);
        filenames[j] = "Models.builtin";
    }
#endif

    cm.fem_in();

    if (cm.no_compose) {
        cm.fem_stats();
    } else {
        if (cm.have_opt("cascade-stats")) cm.fem_stats();
    for ( ; ; ) { // input transducer from string line reading loop
        if (nTarget != -1) { // if to construct a finite state from input
            if ( !*line_in) {
            fail_ntarget:
                if (input_lineno == 0)
                    Config::warn() << "No lines of input provided.\n";
                PLACEMENT_NEW (&chain[nTarget])WFST();
                break;
            }

            //*line_in >> ws; // changed in Carmel 6.9 - don't skip empty lines; we want to treat them as the same as a line with *e* on it (empty string)
            if (!getline(*line_in,buf))
                goto fail_ntarget;
            int length;
            if (flags['P']){ // need a permutation lattice instead
                PLACEMENT_NEW (&chain[nTarget]) WFST(buf.c_str(),length,1);
            } else { // no permutation, just need input acceptor
                PLACEMENT_NEW (&chain[nTarget]) WFST(buf.c_str());
                length=chain[nTarget].numStates()-1; // single letter uses separate start + final surrounding it.  each addl letter adds 1 state.
            }
            cm.n_symbols+=length;

            CHECKLEAK(input_lineno);
            ++input_lineno;

            if (!flags['q'])
                Config::log() << "Input line " << input_lineno << ": " << buf.c_str();
#ifdef  DEBUGCOMPOSE
            Config::debug() << "\nprocessing input line " << input_lineno << ": " << buf.c_str() << " storing in chain[" << nTarget << "]\n";
#endif
            if ( !(chain[nTarget].valid()) ) {
                Config::warn() << "Couldn't handle input line: " << buf << "\n";
                return -3;
            }
        }


        bool r=flags['r'];
        result = (r ? &chain[nChain-1] :&chain[0]);
        cm.minimize(result);
        if (nInputs < 2)
            cm.prune(result);
#ifdef  DEBUGCOMPOSE
        Config::debug() << "\nStarting composition chain: result is chain[" << (int)(result-chain) <<"]\n";
#endif

        if (nChain<2 && !cascade.trivial) {
            cascade.set_trivial();
        }

        unsigned n_compositions=0;
        bool first=true;
        cascade.add(result);
        bool anycomposed=false;
        for ( i = (r ? nChain-2 : 1); (r ? i >= 0 : i < nChain) && result->valid() ; (r ? --i : ++i),first=false ) {
// composition loop
            ++n_compositions;
#ifdef  DEBUGCOMPOSE
            Config::debug() << "----------\ncomposing result with chain[" << i<<"] into next\n";
#endif
            // composition happens here:
            cascade.add(chain+i);
            if (first)
                cascade.prepare_compose();
            else
                cascade.prepare_compose(r);
            WFST &t1=(r ? chain[i] : *result);
            WFST &t2=(r ? *result : chain[i]);
            WFST *next = NEW WFST(cascade,t1,t2, flags['m'], flags['a']);
#ifndef NODELETE
            //      if (nTarget != -1) {
            //        (r ? next->stealOutAlphabet(*result) : next->stealInAlphabet(*result));
#ifdef  DEBUGCOMPOSE
            //        Config::debug() << "result will be going away - take its alphabet\n";
#endif
            //      }

            //      if (nTarget != -1) {
            // &&(result !=  &chain[nTarget]) ){
            // the above condition was removed - why?  because in -i -P or -b mode, the input transducer needs to be deleted.  was a memory leak.  we protect against deleting the chain twice at the end of main by checking nTarget
#ifdef DEBUGCOMPOSE
            Config::debug() << "deleting result and replacing it with next\n";
#endif
            if (!first)
                delete result;
            //      }
#endif
            result = next;

            int q_states=result->size();
            int q_arcs=result->numArcs();
            if (!flags['q'])
                Config::log() << "\n\t(" << result->size() << " states / " << result->numArcs() << " arcs";
#ifdef  DEBUGCOMPOSE
            Config::debug() <<"stats for the resulting composition for chain[" << i << "]\n";
            Config::debug() << "Number of states in result: " << result->size() << std::endl;
            Config::debug() << "Number of arcs in result: " << result->numArcs() << std::endl;
            //      Config::debug() << "Number of paths in result (without taking cycles): " << result->numNoCyclePaths() << std::endl;
            //sleep(100);
            // Config::debug() << "woke up\n";
#endif

            if ( !result->valid() ) {
                Config::warn() << ")\nEmpty or invalid result of composition with transducer \"" << filenames[i] << "\".\n";
                cm.print_kbest(kPaths,result);
                goto nextInput;
            }
            bool finalcompose = i == (r ? 0 : nChain-1);
            bool om=long_opts["minimize-compositions"]>=n_compositions || long_opts["minimize-all-compositions"];
            bool nok=!(kPaths>0 && finalcompose);
            bool arcs_changed=cm.shrink(result,true,!(kPaths>0 && finalcompose),nok&&om,")");
            /*
            bool arcs_changed=cm.minimize(result);
            if (!flags['q'] && (q_states != result->size() || arcs_changed ))
                Config::log()  << " reduce-> " << result->size() << "/" << result->numArcs();
            if (!(kPaths>0 && finalcompose)) { // pruning is at least as hard (and includes) finding best paths already; why duplicate effort?
                q_states=result->size();
                q_arcs=result->numArcs();
                cm.prune(result);
                if (!flags['q'] && (q_states != result->size() || q_arcs !=result->numArcs()))
                    Config::log()  << " prune-> " << result->size() << "/" << result->numArcs();
                    }
            if (!flags['q'])
                Config::log() << ")";
            */
            cascade.done_composing(result,(arcs_changed && long_opts["train-cascade-compress"]) || long_opts["train-cascade-compress-always"]);
            anycomposed=true;
        }
        if (!anycomposed)
          cascade.set_composed(result);
        if (!flags['q'])
            Config::log() << std::endl;


#ifdef  DEBUGCOMPOSE
        Config::debug() << "done chain of compositions  .. now processing result\n";
#endif

        if (long_opts["openfst-roundtrip"]) {
            cm.openfst_roundtrip(result);
        }
        if (!cm.post_compose(result))
            goto nextInput;
        if ( flags['A'] ) {
            Assert(weightSource);
            result->assignWeights(*weightSource);
        }
        if ( flags['N'] ) {
            if (labelStart > 0)
                result->numberArcsFrom(labelStart);
            else if ( labelStart == 0 )
                result->lockArcs();
            else
                result->unTieGroups();
        }
        if ( kPaths > 0  ) {
            cm.print_kbest(kPaths,result);
        } else if ( flags['x'] ) {
            result->listAlphabet(cout, 0);
        } else if ( flags['y'] ) {
            result->listAlphabet(cout, 1);
        }
        if ( flags['c'] ) {
            cm.stats(cout,result,"result");
        }

        if ( flags['t'] )
            flags['S'] = 0;
        if ( !flags['b'] ) {
            if ( flags['S'] ) {
                n_pairs=0;
                if (pairStream) {
                    for ( ; ; ) {
                        getline(*pairStream,buf);
                        if ( !*pairStream )
                            break;
                        ++input_lineno;
                        WFST::symbol_ids ins(*result,buf.c_str(),0,input_lineno);
                        getline(*pairStream,buf);
                        if ( !*pairStream )
                            break;
                        ++input_lineno;
                        WFST::symbol_ids outs(*result,buf.c_str(),1,input_lineno);
                        Weight prob=result->sumOfAllPaths(ins, outs);
                        ++n_pairs;
                        prod_prob*=prob;
                        cout << prob << std::endl;
                    }
                } else {
                    List<int> empty_list;
                    n_pairs=1;
                    cout << (prod_prob=result->sumOfAllPaths(empty_list, empty_list)) << std::endl;
                }
            } else if ( flags['t'] ) {
                show_seed();
                training_corpus corpus;
                if (pairStream) {
                    result->read_training_corpus(*pairStream,corpus);
                } else {
                    corpus.set_null();
                }
                if (gibbs) {
                  result->train_gibbs(cascade,corpus,nms,train_opt,cm.gopt,cm.printer);
                } else {
                    unsigned rr=train_opt.ran_restarts;
                    if (long_opts["final-restart"])
                        rr=(unsigned)long_opts["final-restart"];
                    WFST::random_restart_acceptor ran_accept(rr,long_opts["restart-tolerance"],long_opts["final-restart-tolerance"]);
                    train_opt.ra=ran_accept;
                    result->train(cascade,corpus,nms,flags['U'],smoothFloor,converge, converge_pp_ratio, train_opt);
                }

                if (trainc) {
                    cm.write_trained("trained");
                }
            } else if ( nGenerate > 0 ) {
              bool minimize=long_opts["minimize"] or long_opts["minimize-sum"] or long_opts["minimize-determinize"] or long_opts["minimize-determinize-only"] or long_opts["minimize-rmepsilon"] or long_opts["minimize-pairs"] or long_opts["minimize-pairs-no-epsilon"];
                cm.shrink(result,true,true,minimize,"\n");
//                cm.minimize(result);
                if ( maxGenArcs == 0 )
                    maxGenArcs = DEFAULT_MAX_GEN_ARCS;
                if ( flags['G'] ) {
                    show_seed();
                    for (int i=0; i<nGenerate; ) {
                        List<PathArc> l;
                        if (result->randomPath(&l) != -1) {
                            if (flags['@']) {
                                WFST::print_training_pair(cout,l);
                            } else {
                                printPath(flags,&l);
                            }
                            ++i;
                        }
                    }
                } else {
                    int maxSize = maxGenArcs+1;
                    int *inSeq = NEW int[maxSize];
                    int *outSeq = NEW int[maxSize];
                    for ( int s = 0 ; s < nGenerate ; ++s ) {
                        while ( !result->generate(inSeq, outSeq, 0, maxGenArcs) ) ;
                        printSeq(result->in_alph(),inSeq,maxGenArcs);
                        cout << std::endl;
                        printSeq(result->out_alph(),outSeq,maxGenArcs);
                        cout << std::endl;
                        /*for ( i = 0 ; i < maxSize ; ) {
                          if (outSeq[i] > 0)
                          cout << result->outLetter(outSeq[i]);
                          ++i;
                          if ( i < maxSize && outSeq[i] > 0 )
                          cout << ' ';
                          }
                          cout << std::endl;
                        */
                    }
                    delete[] inSeq;
                    delete[] outSeq;
                }
            }

            if ( (!flags['k'] && !flags['x'] && !flags['y'] && !flags['S'] && !flags['c'] && !flags['g'] && !flags['G'] && !trainc)
                 || flags['F'] ) {
                cm.shrink(result,true,true,long_opts["minimize"]||long_opts["minimize-determinize-only"],"\n");
//                cm.prune(result);
                cm.post_train_normalize(result);
//                cm.minimize(result);

                result->raisePower(exponent);
                cm.write_transducer(*fstout,result);
            }
        }
    nextInput:

#ifndef NODELETE

        // Now delete the compostion result to free up memory, this is important
        // when you are doing batch compostions (option -b).
        // But make sure  first that you are not deleting one of the main
        // WFSTs. That is why we check if the result is one of them and if it is
        // we don't delete it.
        isInChain = false ;
        for (int i = 0 ; i < nChain ; i++){
            if (result == &chain[i])
                isInChain = true ;
        }
        if (!isInChain){
#ifdef  DEBUGCOMPOSE
            Config::debug() << "deleting result at end of processing\n";
#endif
            delete result ;
        }
#ifdef  DEBUGCOMPOSE
        else
            Config::debug() << "can't delete result because it is part of chain .. \n";
#endif
#endif
        if (nTarget != -1) { // if to construct a finite state from input
            chain[nTarget].~WFST();
        }
        if ( !flags['b'] )
            break;
    } // end of all input
    cm.report_batch();
    }

    cm.fem_out();

    if ( flags['S'] && input_lineno > 0) {
        Config::log() << "-S corpus ";
        cm.log_ppx(n_pairs,prod_prob);
    }

#ifndef NODELETE
    for ( i = 0 ; i < nChain ; ++i )
        if (i!=nTarget)
            chainMemory[i].~WFST();
    //  if ( flags['A'] )
    //    chainMemory[i].~WFST();
#endif
    ::operator delete(chainMemory);
    delete[] parm; // alias filenames unless -s
    if(flags['s'])
        delete[] alloced_filenames;
    if(nTarget != -1) {
        if(line_in != &cin)
            delete line_in; // rest were deleted after transducers were read
            //FIXME: reenable deletion later after solving: this is a double deletion under carmel.debug -ri -k 5 -IEQ word.names.50000wds.transducer word-epron.names.55000wds.transducer epron-jpron.1.transducer jpron.transducer vowel-separator.transducer jpron-asciikana.transducer angela.str

    }
    if (pairStream != &cin)
        delete pairStream;
    delete[] newed_inputs;
    if ( fstout != &cout )
        delete fstout;
    return 0;
}catch(std::exception &e) {
    Config::log()<<"ERROR: "<<e.what()<<"\n";
    return -11;
}
}


#endif


void WFSTformatHelp(void)
{
  cout << "\nSee carmel/doc/FORMATS for a description of the transducer file format.\n";
}

void usageHelp(void)
{
    cout << "usage: carmel [switches] [file1 file2 ... filen]\n\ncomposes a seq";
    cout << "uence of weighted finite state transducers and writes the\nresul";
    cout << "t to the standard output.\n\n-l (default)\tleft associative comp";
    cout << "osition ((file1*file2) * file3 ... )\n-r\t\tright associative co";
    cout << "mposition (file1 * (file2*file3) ... )\n-s\t\tthe standard input";
    cout << " is prepended to the sequence of files (for\n\t\tleft associativ";
    cout << "e composition), or appended (if right\n\t\tassociative)\n-i\t\tt";
    cout << "he first input (depending on associativity) is interpreted as\n\t";
    cout << "\ta space-separated sequence of symbols, and translated into a\n";
    cout << "\t\ttransducer accepting only that sequence\n";
    cout << "-P Similar to (-i) but instead of building an acceptor with a\n\t\t";
    cout << "single arc, construct a permutaion lattice that accepts the\n\t\t";
    cout << "input in all possible reorderings.\n-k n\t\tthe n best ";
    cout << "paths through the resulting transducer are written\n\t\tto the s";
    cout << "tandard output in lieu of the transducer itself\n-b\t\tbatch com";
    cout << "postion - reads the sequence of transducers into\n\t\tmemory, ex";
    cout << "cept the first input (depending on associativity),\n\t\twhich co";
    cout << "nsists of sequences of space-separated input symbols\n\t\t(as in";
    cout << " -i) separated by newlines.  The best path(s) through\n\t\tthe r";
    cout << "esult of each composition are written to the standard\n\t\toutpu";
    cout << "t, one per line, in the same order as the inputs that\n\t\tgener";
    cout << "ated them\n-S\t\tas in -b, the input (file or stdin) is a newlin";
    cout << "e separated\n\t\tlist of symbol sequences, except that now the o";
    cout << "dd lines are\n\t\tinput sequences, with the subsequent sequence ";
    cout << "being the\n\t\tcorresponding output sequence\n\t\tthis command s";
    cout << "cores the input / output pairs by adding the sum\n\t\tof the wei";
    cout << "ghts of all possible paths producing them, printing\n\t\tthe wei";
    cout << "ghts one per line if -i is used, it will apply to the\n\t\tsecon";
    cout << "d input, as -i consumes the first\n-n\t\tnormalize the weights o";
    cout << "f arcs so that for each state, the\n\t\tweights all of the arcs ";
    cout << "with the same input symbol add to one";
    cout << "\n-j\t\tPerform joint rather than conditional normalization";
    cout << "\n-+ a\t\tUsing alpha a (recommended: 0), perform pseudo-Dirichlet-process normalization:\n\t\texp(digamma(alpha+w_i))/exp(digamma(alpha+sum{w_j}) instead of just w_i/sum{w_j}";
    cout << "\n-t\t\tgiven pairs of input/output sequences, as in -S, adjust the\n\t\tweights of the tra";
    cout << "nsducer so as to approximate the conditional\n\t\tdistribution o";
    cout << "f the output sequences given the input sequences\n\t\toptionally";
    cout << ", an extra line preceeding an input/output pair may\n\t\tcontain";
    cout << " a floating point number for how many times the \n\t\tinput/outp";
    cout << "ut pair should count in training (default is 1)\n-e w\t\tw is th";
    cout << "e convergence criteria for training (the minimum\n\t\tchange in ";
    cout << "an arc\'s weight to trigger another iteration) - \n\t\tdefault w";
    cout << " is 1E-4 (or, -4log)\n-X w\t\tw is a perplexity convergence ratio between 0 and 1,\n\t\twith 1 being the strictest (default w=.999)\n-f w\t\tw is a count added to every arc before normalizing for train (-t)";
    cout << "ing,\n\t\tadded to the counts for all arcs, immediately";
    cout << " before normalization -\n\t\t(this implements so-called \"Dirichlet prior\" smoothing)";
    cout << "\n-U\t\tuse the initial weights of non-locked arcs as prior counts\n\t\t(in addition to -f)";
    cout << "\n-M n\t\tn is th";
    cout << "e maximum number of training iterations that will be\n\t\tperfor";
    cout << "med, regardless of whether the convergence criteria is\n\t\tmet ";
    cout << "- default n is 256\n-x\t\tlist only the input alphabet of the tr";
    cout << "ansducer to stdout\n-y\t\tlist only the output alphabet of the t";
    cout << "ransducer to stdout\n-c\t\tlist only statistics on the transduce";
    cout << "r to stdout\n-F filename\twrite the final transducer to a file (";
    cout << "in lieu of stdout)\n-v\t\tinvert the ";
    cout << "resulting transducer by swapping the input and\n\t\toutput symbo";
    cout << "ls \n-d\t\tdo not reduce (eliminate dead-end states)";
    cout << " created\n-C\t\tconsolidate arcs with same source, destination, ";
    cout << "input and\n\t\toutput, with a total weight equal to the sum (cla";
    cout << "mped to a\n\t\tmaximum weight of one)\n-p w\t\tprune (discard) a";
    cout << "ll arcs with weight less than w";
    cout << "\n-w w\t\tprune states and arcs only used in paths w times worse\n\t\tthan the best path (1 means keep only best path, 10 = keep paths up to 10 times weaker)";
    cout <<"\n-z n\t\tkeep at most n states (those used in highest-scoring paths)";
    cout << "\n-g n\t\tstochastically generate";
    cout << " n input/output pairs by following\n\t\trandom paths (first choosing an input symbol with uniform\n\t\tprobability, then using the weights to choose an output symbol\n\t\tand destination) from the in";
    cout << "itial state to the final state\n\t\toutput is in the same for";
    cout << "m accepted in -t and -S.\n\t\tTraining a transducer with conditional normalization on its own -g output should be a no-op.\n-G n\t\tstochastically generate n paths by randomly picking an arc\n\t\tleaving the current state, by joint normalization\n\t\tuntil the final state is reached.\n\t\tsame output format as -k best paths\n\n"
        "-@\t\tFor -G or -k, output in the same format as -g and -t.  training on this output with joint normalization should then be a noop.\n-R n\t\tUse n as the random seed for repeatable -g and -G results\n\t\tdefault seed = current time\n-L n\t\twhile generating input/output p";
    cout << "airs with -g or -G, give up if\n\t\tfinal state isn't reached after n steps (default n=1000)\n-T n\t\tduring composit";
    cout << "ion, index arcs in a hash table when the\n\t\tproduct of the num";
    cout << "ber of arcs of two states is greater than n \n\t\t(by default, n";
    cout << " = 32)\n-N n\t\tassign each arc in the result transducer a uniq";
    cout << "ue parameter-tie group number\n\t\tstarting at n and counting up.  If n is 0 (";
    cout << "the special group\n\t\tfor unchangeable arcs), all the arcs are ";
    cout << "assigned to group 0\n\t\tif n is negative, all group numbers are";
    cout << " removed";
    cout << "\n-A\t\tthe weights in the first transducer (depending o";
    cout << "n -l or -r, as\n\t\twith -b, -S, and -t) are assigned to the res";
    cout << "ult, by arc group\n\t\tnumber.  Arcs with group numbers for whic";
    cout << "h there is no\n\t\tcorresponding group in the first transducer a";
    cout << "re removed\n-m\t\tgive meaningful names to states created in com";
    cout << "position\n\t\trather than just numbers\n"
        "-a\t\t(may SIGNIFICANTLY speed up --train-cascade) during composition";
    cout << ", keep the identity of matching arcs from\n\t\tthe two transduce";
    cout << "rs separate, assigning the same arc group\n\t\tnumber to arcs in";
    cout << " the result as the arc in the transducer it\n\t\tcame from.  Thi";
    cout << "s will create more states, and possibly less\n\t\tarcs, than the";
    cout << " normal approach, but the transducer will have\n\t\tequivalent p";
    cout << "aths.\n-h\t\thelp on transducers, file formats\n";
    cout << "-V\t\tversion number\n-u\t\tDon't normalize outgoing arcs for each input during training;\n\t\ttry -tuM 1 to see forward-backward counts for arcs" ;
    cout << "\n-Y\t\tWrite transducer to GraphViz .dot file\n\t\t(see http://www.research.att.com/sw/tools/graphviz/)";
    cout << "\n-q\t\tSuppress computation status messages (quiet!)";
    cout << "\n-K\t\tAssume state names are integer indexes (when the final state is an integer)";
    cout << "\n-o g\t\tUse learning rate growth factor g (>= 1) (default=1)";
    cout << "\n-1\t\trandomly scale weights (of unlocked arcs) after composition uniformly by (0..1]";
    cout << "\n-! n\t\tperform n additional random restarts (initializations of arcs) for training, keeping the lowest perplexity";
    cout << "\n-?\t\tcache EM derivations in memory for faster iterations";
    cout << "\n-:\t\tcache em derivations including reverse structure (faster but uses even more memory)";

    cout << "\n-= 2.0\t\traise weights to 2.0th power, *after* normalization. e.g. -= 0 removes all weights\n";
    cout << "\n\n";
    cout << "some formatting switches for paths from -k or -G:\n\t-I\tshow input symbols ";
    cout << "only\n\t-O\tshow output symbols only\n\t-E\tif -I or -O is speci";
    cout << "fied, omit special symbols (beginning and\n\t\tending with an as";
    cout << "terisk (e.g. \"*e*\"))\n\t-Q\tif -I or -O is specified, omit out";
    cout << "ermost quotes of symbol names\n\t-W\tdo not show weights for pat";
    cout << "hs\n\t-%\tSkip arcs that are optimal, showing only sidetracks\n";
    cout << "\n\nWeight output format switches\n\t\t(by default, small/large weights are written as logarithms):";
    cout << "\n\t-Z\tWrite weights in logarithm form always, e.g. 'e^-10',\n\t\texcept for 0, which is written simply as '0'";
    cout << "\n\t-B\tWrite weights as their base 10 log (e.g. -1log == 0.1)";
    cout << "\n\t-2\tInstead of e^K, output Kln (deprecated)";
    cout << "\n\t-D\tWrite weights as reals always, e.g. '1.234e-200'";
    cout << "\n\nTransducer output format switches:";
    cout << "\n\t-H\tOne arc per line (by default one state and all its arcs per line)";
    cout << "\n\t-J\tDon't omit output=input or Weight=1";

#ifdef USE_OPENFST
    cout << "\n\n--minimize-compositions=N : det/min after each of the first N compositions\n"
        "\n"
        "--minimize-all-compositions : the same, but for N=infinity\n"
        "\n"
        "--minimize : minimize the final result before printing.  UNLESS THIS (or one of the two above) IS SET, many --minimize-X options have no effect.\n"
        "\n"
        "--minimize-determinize-only : just determinize, no minimize\n"
        "--minimize-sum : collapse paths by summing prob (default is to keep the best)\n"
        "--minimize-determinize : determinize before minimize - necessary if transducer\n"
        "isn't already deterministic.  if this fails, carmel will abort.  without this\n"
        "option, carmel will detect nondeterminstic transducers and skip minimization\n"
        "\n"
        "--minimize-no-connect : skip the removal of unconnected states after\n"
        "minimization (not recommended)\n"
        "\n"
        "--minimize-inverted : use this if your transducer is output deterministic, and\n"
        "not input deterministic.  inverts, minimizes, then inverts back\n"
        "\n"
        "--minimize-rmepsilon : use to get rid of *e*/*e* (both input and output epsilon)\n"
        "arcs prior to minimization.  necessary if you have any state with two outgoing\n"
        "epsilon arcs, but makes minimization fail if you have loops (leaving final state)\n"
        "\n"
        "--minimize-pairs : in case of a nonfunctional transducer (input nondeterministic\n"
        "with multiple possible output strings) perform a less ambitious FSA --minimize treating\n"
        "the input:output as a single symbol '(input,output)'.  this provides less minimization but always works\n"
        "\n"
        "--minimize-pairs-no-epsilon : for --minimize-pairs, treat *e*:*e* as a real symbol and not an epsilon\n"
        "if you don't use this, you may need to use --minimize-rmepsilon, which should give a smaller result anyway\n"
        ;

#endif
cout <<         "\n"
        "--restart-tolerance=w : like -X w, but applied to the first iteration of each random start.\n"
        "a random start is rejected unless its perplexity is within (log likelihood ratio) w of the best start so far.\n"
        "w=1.1 allows a start up to 10% worse than the best, .9 demands a 10% improvement.\n"
        "\n"
        "--final-restart-tolerance=w : vary --restart-tolerance from its initial value to this\n"
        "\n"
        "--final-restart=N : the 1st...Nth random restart move from --restart-tolerance to\n"
        "--final-restart-tolerance (exponentially) and then holds constant from restarts N,N+1,...\n"
    ;

    cout << "\n\n--final-sink : if needed, add a new final state with no outgoing arcs\n";
    cout << "\n--consolidate-max : for -C, use max instead of sum for duplicate arcs\n";
    cout << "\n--consolidate-unclamped : for -C sums, clamp result to max of 1\n";
    cout << "\n--project-left : replace arc x:y with x:*e*\n";
    cout << "\n--project-right : replace arc x:y with *e*:y\n";
    cout << "\n--project-identity-fsa : modifies either projection so result is an identity arc (left means keep the input symbol, right the output)\n";
    cout << "\n--random-set : like -1 but ignore previous weights and set a new weight on (0..1]\n";
    cout << "\n--train-cascade : train simultaneously a list of transducers composed together\n; for each transducer filename f, output f.trained with new weights.  as with -t, the first transducer file argument is actually a list of input/output pairs like in -S.  with -a, more states but fewer arcs just like composing with -a, but original groups in the cascade are preserved even without -a.\n";
    cout << "\n--matrix-fb : use a n*m*s matrix (n=input sentence length, m=output len, s=# states) for training, rather than a sparse derivations lattice (not recommended, but may be faster in some cases without caching i.e. -: or -?)";
    cout <<    "\n"
        "--disk-cache-derivations=/tmp/derivations.template.XXXXXX : use the provided filename (optional) to cache more derivations than would fit into memory.  XXXXXX is replaced with a unique-filename-making string.  the file will be deleted after training completes\n"
        "\n"
        "--disk-cache-bufsize=1M : unless 0, replace the default file read buffer with one of this many bytes (k=1000, K = 1024, M=1024K, etc)"
        "\n--cache-no-prune : don't prune unreachable states in derivation cache (not recommended)."
        "\n"
        ;
    cout << "\n"
        "--exponents=2,.1 : comma separated list of exponents, applied left to right to the input WFSTs (including stdin if -s).  if more inputs than exponents, use (noop) exponent of 1.  this differs from -=, which exponentiates the weights of the resulting (output) WFST.\n"
        ;
    cout << "\n"
        "--post-b=transducerfile : in conjunction with -b, a parallel sequence of inputs to be composed with the result (left or right composition depending on -l / -r.  compare to -S except 2 parallel files instead of alternating lines, and gives best paths like -b.  also may succeed for compositions that wouldn't fit in memory under -S\n";

    cout << "\n"
        "--sum : show (before and after --post-b) product of final transducer's sum-of-paths (acyclic-correct only), as prob and per-input-ppx.\n"

        ;

    cout << "\n"
        "--digamma=0,,0.5 : (train-cascade) comma separated components for each transducer in the cascade; if the component is the empty string, do the usual num/denom normalization; if given a number alpha (as opposed to the empty string), do exp(digamma(num+alpha))/exp(digamma(denom+alpha)).  the variational bayes approximation requires exp(digamma(denom+N*alpha)) where N is the size of the normgroup; this can be achieved by setting --digamma=0 and --priors=x.\n"
        "--normby=JCCN : (gibbs/train-cascade) normalize the nth transducer by the nth character; J=joint, C=conditional, N=none (every arc stays at original prob; in --crp for now, this means a probability of 1 is used for N normalized arcs)\n"
        "--priors=1,e^-2 : (gibbs/train-cascade) add priors[n] to the counts of every arc in the nth transducer before normalization\n"
        "\n"
        ;

    cout << "\nforest-em export/import:\n"
        "--load-fem-param=infile: restore params onto cascade from forest-em params file\n"
        "--write-loaded=suffix: write inputN.suffix after --load-fem-param and possible normalization with --normby (empty suffix means overwrite 'inputN', not 'inputN.')\n"
        "--number-from=N: (before write-loaded) assign consecutive group ids to each arc starting at N>0\n"
        "--fem-param=outfile: write forest-em params file for the input cascade\n"
        "--fem-norm=outfile : write a forest-em normgroups file for the input cascade\n"
        "--fem-forest=outfile : write a forest-em derivation forests file (implies --train-cascade; to avoid actual training, use -M -1 to perform no EM\n"
        "--fem-alpha=outfile : write a (parallel to --fem-param) list of per-parameter alpha (as in --crp --priors=.01,.0001).  locked arcs and --normby=N parameters get alpha -1 (which is also understood by forest-em --crp to lock)\n"
        "--no-compose : don't compose or train; just show stats (useful with fem-param fem-norm etc. but not fem-forest)\n"
        "\n";

    cout << "\ngibbs sampling / incremental em options:\n"
        "--crp : train a chinese restaurant process (--priors are the alphas) by gibbs sampling instead of EM.  implies --train-cascade, and derivation caching (-? -: or --disk-cache-derivations). (use -M n) to do n iterations; -a may be more efficient as usual\n"
        "--burnin=n : when summing gibbs counts, skip <burnin> iterations first (iteration 0 is a random derivation from initial weights).  typical settings are --burnin=2000 -M 10000\n"
        "--crp-restarts : number of additional runs (0 means just 1 run), using cache-prob at the final iteration select the best for .trained and --print-to output.  --init-em affects each start.  TESTME: print-every with path weights may screw up start weights\n"
        "--high-temp=n : (default 1) raise probs to 1/temp power before making each choice - deterministic annealing for --unsupervised\n"
        "--low-temp=n : (default 1) temperature at final iteration (linear interpolation from high->low)\n"
        "--final-counts : normally, counts are averaged over all the iterations after --burnin.  this option says to use only final iteration's (--burnin is ignored)\n"
        "--crp-exclude-prior : when writing .trained weights, use only the expected counts from samples, excluding the prior (p0) counts\n"
        "--crp-argmax-sum : instead of multiplying the sample probs together and choosing the best, sum (average) them\n"
        "--crp-argmax-final : for --crp-restarts, choose the sample/.trained weights with best final sample cache-prob.  otherwise, take the best (avg cache-prob post --burnin)\n"
        "--init-em=n : perform n iterations of EM to get weights for randomly choosing initial sample, but use initial weights (pre-em) for p0 base model; note that EM respects tied/locked arcs but --crp removes them\n"
        "--em-p0 : with init-em=n, use the trained weights as the base distribution as well (note: you could have done this in a previous carmel invocation, unlike --init-em alone)\n"
        "--print-from=m --print-to=n: for m..(n-1)th input transducer, print the final iteration's path on its own line.  default n=0.  a blank line follows each training example\n"
        "--print-every=n: with --print-to, print the 0th,nth,2nth,,... (every n) iterations as well as the final one.  these are prefaced and suffixed with comment lines starting with #\n"
        "--print-counts-from=m --print-counts-to=n : every --print-every, print the instantaneous and cumulative counts for parameters m...(n-1) (for debugging)\n"
        "--norm-order : print counts in gibbs param id order (different from fst file order for conditional); params for a normgroup will always be consecutive\n"
        "--print-counts-sparse=x : only print counts that are at least x above the prior count (also shows index if x!=0)\n"
        "--print-counts-rich : show cascade arc associated with each count\n"
        "--width : for --print-counts, truncate numbers to this many chars wide\n"
        "--print-norms-from=m --print-norms-to=n : likewise, show sum of normgroups' counts\n"
        "--uniform-p0 : use a uniform base probability model for --crp, even when the input WFST have weights\n"
      "--init-from-p0 : For the initial sample: normally previous blocks' cache is used for proposal prob.  With this option, each block is generated independently from the base distribution alone (resampling is unchanged).\n"
      "--dirichlet-p0 : use the input WFST weights, UNNORMALIZED, as the dirichlet prior pseudocounts - this way different normgroups can have different effective alphas.  Note: alpha argument still further scales the initial psuedocounts, so set alpha=1.\n"
        "--cache-prob : show the true probability according to cache model for each sample\n"
        "--sample-prob : show the sample prob given model, previous sample\n"
        "--no-prob : show no probability for --crp\n"
        "--prior-inference-stddev : if >0, after each post burn-in iteration, allow each normalization group's prior counts to be scaled by some random ratio with stddev=this centered around 1.  the default --priors is now 1 instead of 0. proposals that lead to lower cache prob for the sample tend to be rejected.  Goldwater&Griffiths used 0.1\n"
        "--prior-inference-global : disregarding supplied hyper-normalization groups, scale all prior counts in the same direction.  BHMM1 in Goldwater&Griffiths, but moves priors for ALL transducers (that don't have --prior-groupby=0) in the same direction.  for the same direction per transducer, use --prior-groupby=111...\n"
        "--prior-inference-restart-fresh : at each --crp-restart, reset the priors to their initial values\n"
        "--prior-inference-start : on iterations [start,end) do hyperparam inference; default is to do inference starting from --burnin, but this overrides that\n"
        "--prior-inference-end : default is to continue inference until the final sample, but this overrides that (e.g. you may wish inference to conclude at burnin)\n"
        "--prior-groupby=0211 : (gibbs) Griffiths & Goldwater style prior-inference; nth character means, for the nth cascade transducer: 0: no inference.  1: adjust all normgroups' priors in the same direction (BHMM1), 2: adjust independently for each normalization group. 1 is the default.\n"
        "--prior-inference-show : show for each prior group the cumulative scale applied to its prior counts\n"
        "--expectation: use full forward/backward fractional counts instead of a single count=1 random sample\n"
        "--random-start: for expectation, scale the initial per-example counts by random [0,1).  without this, every run would have the same outcome.  this is implicitly enabled for restarts, of course.\n"
        "--include-self: don't remove the counts from the current block in computing the proposal probabilities (this plus --expectation = incremental EM)\n"
        "\n";


    cout << "\n--help : more detailed help\n";
    /* // user doesn't need to know about this stuff
    cout << "\n--train-cascade-compress : perform a (probably frivolous) reduction of unused arcs' parameter lists\n";
    cout << "\n--train-cascade-compress-always : even when the composition needed no pruning, compress the table (certainly frivolous)\n";
    cout << "\n--debug-cascade=N : N is a bitfield: DEBUG_CHAINS=1,DEBUG_CASCADE=2,DEBUG_COMPOSED=4,DEBUG_COMPRESS=8,DEBUG_COMPRESS_VERBOSE=16\n";
    cout << "\n--compose-cascade : see the result of composing as a cascade without doing tuning (note: for debugging; group numbers on output will be impenetrable even with --debug-cascade=4\n";
    */
    cout << "\n\nConfused?  Think you\'ve found a bug?  If all else fails, ";
    cout << "e-mail graehl@isi.edu or knight@isi.edu\n\n";

}
