/* TODO(em): use alpha array (per param prior/fixed flag).  but not appearing in normgroup is same as locking for em.

  TODO(gibbs):
   don't allocate counts array, best_weights array - redundant w/ gibbs_base
   deallocate rule_weights array once to_gibbs()?  alternate output for --outparams-file
   implement viterbi checkpoints and/or viterbi final redecode?
   add parens to sample printout?
*/

#ifndef GRAEHL_TT__FOREST_EM_HPP
#define GRAEHL_TT__FOREST_EM_HPP

#define TICK_PERIOD 1000

#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <algorithm>
#include <iostream>

#include <graehl/shared/myassert.h>
#include <boost/config.hpp>
#include <graehl/shared/tree.hpp>
#include <graehl/shared/config.h>
#include <graehl/shared/unimplemented.hpp>
#include <graehl/shared/weight.h>
#include <graehl/forest-em/forest.hpp>
#include <graehl/forest-em/forest-em-params.hpp>

#include <graehl/shared/em.hpp>
#include <graehl/shared/dynarray.h>
#include <graehl/shared/normalize.hpp>
#include <graehl/shared/debugprint.hpp>
#include <graehl/shared/funcs.hpp>
#include <graehl/shared/myassert.h>
#include <graehl/shared/filelines.hpp>
#include <graehl/shared/backtrace.hpp>
#include <graehl/shared/input_error.hpp>

#include <graehl/shared/memmap.hpp>
#include <graehl/shared/swapbatch.hpp>
#include <graehl/shared/gibbs.hpp>

#include <map>

namespace graehl {

template <class Float>
struct FForests : public gibbs_base {
    typedef FForests<Float> Forests;
    typedef FForest<Float> Forest;
    typedef typename Forest::inside_t inside_t;
    typedef typename Forest::prob_t prob_t;
    typedef typename Forest::count_t count_t;
    typedef NormalizeGroups<count_t,prob_t> Norms;
    typedef typename Norms::index_type NormIndex;
    typedef typename Norms::Groups::iterator NormPtr;
    typedef typename Norms::Group NormGroup;
    typedef typename NormGroup::iterator NormGroupIter;

    typedef SwapBatch<Forest> ForestBatches;
    std::string tempfile_prefix;
    typedef std::size_t size_t;
    size_t max_nodes;
    periodic_wrapper<tick_writer> ticker;
// # params:
    dynamic_array<prob_t> rule_weights; //optional initial param values
    auto_array<prob_t> best_weights;
    auto_array<count_t> counts;
// # forest nodes:
    auto_array<inside_t> inside, outside;
    auto_array<ForestNode *> viterbi_or_backpointers;
    count_t prior_count;
    size_t n_nodes;
    unsigned restart;
    unsigned iteration;
    unsigned watch_period;
    unsigned viterbi_period,per_forest_counts_period;
    unsigned watch_depth;
    bool watching_group;
    NormPtr watch_group;
    unsigned watch_rule;
    bool save_best_enable;
    bool checkpoint_parameters;
    std::string file_prefix;
    bool count_report_enable;
    count_t count_report_threshold;
    prob_t prob_report_threshold;
    bool viterbi_enable,per_forest_counts_enable;
    std::ostream &logstream;
    ForestBatches forests;
    FileLines rule_names;
    Norms norm_groups;
    size_t  max_norm_ruleid,max_forest_ruleid;
    std::ofstream viterbi_o,per_forest_counts_o;
    std::ostream *viterbi_out,*per_forest_counts_out,*per_forest_inside_out;
    bool viterbi_go,per_forest_counts_go,per_forest_inside_go;
    double total_logprob;
    unsigned forest_no;
    typename Forest::prepare_inside_outside *forest_prep;
    unsigned total_forests; // necessary?  this is just forests.size()
    typedef std::map<unsigned,count_t> per_forest_counts_t;
    per_forest_counts_t per_forest_counts;

    size_t nodes_used() const {
        return n_nodes;
    }
    void print_info(std::ostream &out=std::cerr) const {
        out << " (temporary files will be stored at " << tempfile_prefix << "*)\n";
    }
    void print_stats(std::ostream &out=std::cerr) const {
        BACKTRACE;
        out << n_nodes << " forest nodes total (" << n_nodes*sizeof(ForestNode)<<" bytes), max #nodes " << max_nodes << ", average " << n_nodes * (1. / total_forests) << "\n";
        forests.print_stats(out);
        out << "\n ";
        norm_groups.print_stats(out);
        out << "\n largest rule index was " << max_forest_ruleid << ".\n";
    }
    void set_watch(size_t watch_rule=0) {
        BACKTRACE;
//        DBPC2("set_watch",watch_rule);
        if (watch_rule) {
            watch_group = norm_groups.find_group_holding(watch_rule);
            if (!watch_group)
                throw std::runtime_error(std::string("Couldn't find rule ").append(boost::lexical_cast<std::string>(watch_rule)).append(" in any normalization groups.\n"));
            watching_group = true;
            DBP(watch_group);
        } else
            watching_group = false;
    }

    void read_norm_groups(std::istream &in) {
        BACKTRACE;
        try {
            in >> norm_groups;
        } catch (std::exception &e) {
            throw_input_error(in,e.what(),"normalization group",norm_groups.num_groups()+1);
//            return false;
        }

        max_norm_ruleid=norm_groups.max_index();
        if (rule_weights.size() && rule_weights.size() <= max_norm_ruleid) {
            std::string error=std::string("Initial rule weights file not big enough - normalization used rule  (").append(boost::lexical_cast<std::string>(max_norm_ruleid)).append(" expected)");
                logstream << error << std::endl;
                throw std::runtime_error(error);
        }
    }
    bool read_forests(std::istream &in) {
        BACKTRACE;
        size_t &fmaxrule=Forest::max_ruleid; // static global return value
        SetLocal<size_t> g1(fmaxrule,0);
        size_accum<size_t> total_size;
        try {
            forests.read_all_enumerate(in,make_both_functors_byref(total_size,ticker));
        } catch (std::exception &e) {
            throw_input_error(in,e.what(),"forest",forests.size()+1);
//            return false;
        }
        max_forest_ruleid = fmaxrule;
        max_nodes=total_size.maximum();
        total_forests=forests.size();
        n_nodes=total_size;
        return true;
    }
    std::string dump_suffix() const
    {
        using boost::lexical_cast;
        using std::string;
        return string(".restart.") + lexical_cast<string>(restart+1)+".iteration."+lexical_cast<string>(iteration+1);
    }
    void dump_params() {
        BACKTRACE;
        using boost::lexical_cast;
        using std::string;
        string suffix=dump_suffix();
        string weightfile=file_prefix+".params"+suffix;
        string countfile=file_prefix+".counts"+suffix;
//        logstream << "\nDumping parameters to " << weightfile << " and counts to " << countfile << std::endl;
        logstream<<'\n';
        {
            std::ofstream weightf(weightfile.c_str());
            write_params(weightf,weightfile);
        }
        {
            std::ofstream countf(countfile.c_str());
            write_counts(countf,countfile);
        }
    }
    void write_params(std::ostream &out,std::string const& fname) {
        BACKTRACE;
        logstream << "Writing trained parameters to " << fname<<"\n";
        print_range(out,rule_weights.begin()+1,rule_weights.end(),true,false);
        out << std::endl;
    }
    void write_counts(std::ostream &out,std::string const& fname) {
        BACKTRACE;
        logstream << "Writing trained counts to " << fname <<"\n";
        print_range(out,counts.begin()+1,counts.end(),true,false);
        out << std::endl;
    }

    // used by write_params_byid
    std::string prob_field,count_field;
    void operator()(std::ostream &out,unsigned ruleid) const
    {
        if (rule_weights.exists(ruleid) && prob_field.length())
            out << ' ' << prob_field << '='  << rule_weights[ruleid];
        if (counts.exists(ruleid) && count_field.length())
            out << ' ' << count_field << '=' << counts[ruleid];
    }

    // doesn't write count_field if you didn't prepare_em (since counts.size()==0)
    // prints to out: "id=N prob_field_=rule_weights[N] count_field=counts[N]"
    // when "id=N" is seen from in, otherwise copies in -> out
    void write_params_byid(std::istream &in,std::ostream &out,const std::string prob_field_="emprob",const std::string count_field_="emcount") {
        BACKTRACE;
        prob_field=prob_field_;
        count_field=count_field_;
        insert_byid(boost::cref(*this),in,out);
    }

    void prealloc_params(unsigned n)
    {
        rule_weights.reserve(n+1); // 1 for 1-based indexing
    }

    void read_params(std::istream &in) {
        BACKTRACE;
        try {
        //if (!read_range(in,rule_weights.begin()+1,rule_weights.end())) {
            rule_weights.clear();
            rule_weights.push_back(); // dummy to make 1-indexed :(*  should define new 1-indexed array class instead
            if (rule_weights.append_from(in) != GENIOGOOD)
                throw std::runtime_error("couldn't read rule weights");
            try {
                rule_weights.compact();
            } catch (std::bad_alloc &e) {
                logstream << "Warning: not enough memory to compact rule weights array - proceeding for now.\n";
            }
//            rule_weights.compact_giving(rule_weights);
//            in >> rule_weights; // bug: need dummy weight for rule #0
        } catch(std::exception &e) {
            std::ostringstream err;

            err << "Couldn't read vector of initial weights: " << e.what() << "\n - expected vector of weights e.g. (1 .5 0) with the first weight being for parameter #1.\n";
            show_error_context(in,err);
            throw std::runtime_error(err.str());
        }
    }

    bool gibbs;

    FForests(gibbs_opts const& gopt,size_t _n_nodes,size_t _max_norm,unsigned tick_period=TICK_PERIOD,std::string tempfile_prefix_="",std::ostream &_log=std::cerr,InDiskfile _rule_names=InDiskfile(),unsigned log_level_=1) :
        gibbs_base(gopt,*gopt.print_file,_log),
      tempfile_prefix(tempfile_prefix_+random_alpha_string(8)),
      watching_group(false),
      viterbi_enable(false),
      per_forest_counts_enable(false),
      logstream(_log),
        forests(tempfile_prefix + ".forests.swap.",_n_nodes*sizeof(ForestNode),false),
      rule_names(_rule_names),
      norm_groups(tempfile_prefix + ".normgroups.swap.",_max_norm*sizeof(NormIndex)),
      max_norm_ruleid(0)
      ,max_forest_ruleid(0),log_level(log_level_)
      , zero_zerocounts(false)
    {
        n_nodes=max_nodes=total_forests=0; // set in read_forests.
        per_forest_counts_go=per_forest_inside_go=viterbi_go=false;
        gibbs=gopt.iter>0;
        per_forest_inside_go=false;
        BACKTRACE;
        if (tick_period)
            ticker.init(&logstream);
        else
            ticker.init(NULL);
        ticker.set_period(tick_period);
    }
    ~FForests() {
        BACKTRACE;
    }


    //TODO: separate out parts that are redundant when using gibbs e.g. counts are already in gibbs params; norm groups tracked in gibbs_base  maybe easier to update redundant things to/from gibbs so same printout can apply, but that wastes memory.  make gibbs_base more abstract so it can defer to implementation for normgroup types etc?  or make forest-em EM impl use gibbs data structure instead.
    //TODO: have usual EM checkpoint type options applicable in gibbs as well (via init_run and init_iteration ?)
    //TODO: move ForestEmParams::perform_forest_em logic here instead?
    void prepare(ForestEmParams const& p) {
        prepare(p,p.prior_counts,p.checkpoint_prefix,p.checkpoint_parameters,p.watch_rule,p.watch_period,p.watch_depth,
                (bool)p.random_restarts,p.count_report_enable
                ,p.count_report_threshold,p.prob_report_threshold,p.viterbi_enable,p.viterbi_per
                ,p.initial_1_params,p.add_k_smoothing,p.per_forest_counts_enable
                ,p.per_forest_counts_per,p.zero_zerocounts);
    }

    size_t rulespace;

    void init_rule_weights(bool initparams_one,bool random_set)
    {
        if (rule_weights.size()) { // initial params were already read
            if (rulespace > rule_weights.size())
                throw std::runtime_error("Initial params file wasn't large enough for forests/norms.");
            if (rulespace < rule_weights.size())
                logstream << "Warning: more initial rule weights were provided (" << rule_weights.size() << ") than used in norms or forests: " << rulespace << std::endl;
            watch_report();
        } else {
//            DBP(initparams_one);
            if (initparams_one) {
                rule_weights.reinit_nodestroy(rulespace,1);
            } else {
                rule_weights.reinit_nodestroy(rulespace);
                norm_groups.init_uniform(rule_weights.begin());
            }
            if (random_set) {
                randomize();
                restart=0;
            }
        }
    }

    void load_rule_names()
    {
        if (rule_names.exists()) {
            unsigned maxid=rulespace-1;
            DBP2(rule_names.size(),maxid);
            if (rule_names.size() < maxid) {
                using namespace std;
                string error=string("Not enough lines in rule names file (").append(boost::lexical_cast<string>(maxid)).append(" expected, got "+boost::lexical_cast<string>(rule_names.size())+")");
                logstream << error << std::endl;
                throw runtime_error(error);
            }
        }
    }

    //FIXME: use ForestEmParams structure throughout rather than silly individual members
    void prepare(ForestEmParams const& p,count_t prior_counts, const std::string &file_prefix_,bool checkpoint_parameters_,size_t _watch_rule=0,unsigned _watch_period=10,unsigned _watch_depth=20,bool restarts=false,bool count_report_enable_=false,count_t count_report_threshold_=1e-2,prob_t prob_report_threshold_=1e-5,bool viterbi_enable_=false,unsigned viterbi_forest_per=0,bool initparams_one=false,count_t add_k_smoothing_=0,bool per_forest_counts_enable_=false,unsigned per_forest_counts_per_=0,bool zero_zerocounts_=false)
    {
        BACKTRACE;
        count_report_enable=count_report_enable_;
        rulespace=std::max(max_forest_ruleid,max_norm_ruleid)+1;
        file_prefix=file_prefix_;
        norm_groups.add_k_smoothing=add_k_smoothing_;
        viterbi_period=viterbi_forest_per;
        viterbi_enable=viterbi_enable_;
        per_forest_counts_period=per_forest_counts_per_;
        per_forest_counts_enable=per_forest_counts_enable_;
        count_report_threshold=count_report_threshold_;
        prob_report_threshold=prob_report_threshold_;
        zero_zerocounts=zero_zerocounts_;

        watch_depth = _watch_depth;
        watch_rule = _watch_rule;
        watch_period = _watch_period;
        checkpoint_parameters=checkpoint_parameters_;
        prior_count=prior_counts;
        set_watch(watch_rule);
        firsttime=true;

        inside.alloc(max_nodes);
        outside.alloc(max_nodes);
        viterbi_or_backpointers.alloc(max_nodes);
        init_rule_weights(initparams_one,p.random_set);
        counts.alloc(rulespace);
        save_best_enable=restarts;
        if (save_best_enable)
            best_weights.alloc(rulespace);
        load_rule_names();
        {
            DBP_ADD_VERBOSE(2);
            DBP(norm_groups);
        }
        {
            DBP_ADD_VERBOSE(1);
            DBP(rule_weights);
        }
        restart=0;
        iteration=0;
        counts_accum=Forest::prepare_accumulate(counts.begin(),&overflows);
    }
    void converge_em() {
        BACKTRACE;
        watch_report();
    }
    //for accumulating per-forest-counts - visit_inside_norm_outside:
    void operator()(unsigned rule,inside_t inside,inside_t norm_outside) {
        count_t count=inside * norm_outside;
        counts[rule] += count;
        //       if (per_forest_counts_out)
//            accumulate(&per_forest_counts,rule,count);
    }


     // assigns random parameter values - not called unless random_restarts > 0
    void randomize() {
        BACKTRACE;
        norm_groups.init_random(rule_weights.begin());
        watch_report();
        ++restart;
        iteration=0;
    }
     // returns (weighted) average log prob over all examples given current parameters, and collects counts (performs count initialization itself).  first_time flag intended to allow you to drop examples that have 0 probability (training can't continue if they're kept)
private:

    //FIXME: encapsulate writing something to a CHECKPOINT.suffix file every N forests ... if I add a 3rd
    void viterbi_begin() {
        if (viterbi_enable) {
            viterbi_go=true;
            viterbi_o.open((file_prefix + ".viterbi" +  dump_suffix()).c_str());
            viterbi_out=&viterbi_o;
        } else {
            viterbi_go=false;
        }
    }
    void viterbi_end() {
        if (viterbi_go)
            viterbi_o.close();
    }
    void per_forest_counts_begin() {
        if (per_forest_counts_enable) {
            per_forest_counts_go=true;
            per_forest_counts_o.open((file_prefix + ".per_forest_counts" +  dump_suffix()).c_str());
            per_forest_counts_out=&per_forest_counts_o;
        } else {
            per_forest_counts_go=false;
        }
    }
    void per_forest_counts_end() {
        if (per_forest_counts_go)
            per_forest_counts_o.close();
    }
    struct watch_guard {
        Forests *f;
        watch_guard(Forests *f_) : f(f_) {
            if (f->on_watch_iteration()) {
                f->viterbi_begin();
                f->per_forest_counts_begin();
            }
        }
        ~watch_guard() {
            if (f->on_watch_iteration()) {
                f->viterbi_end();
                f->per_forest_counts_end();
            }
        }
    };
    typename Forest::accumulate_counts counts_accum;
    void begin_visit() {
//        DBPC2("estimate",rule_weights);DBP_SCOPE;
        if (collect_counts) {
            count_t weighted_prior=prior_count*total_forests;
            enumerate(counts,value_setter(weighted_prior));
            total_logprob=0;
            counts_accum.reset_stats();
        }
        forest_no=0;
        n_zeroprob=0;
        forest_prep=new typename Forest::prepare_inside_outside(counts.begin(),rule_weights.begin(),inside.begin(),outside.begin(),viterbi_or_backpointers.begin()); // only need to do this once?  well, this way we're safe even if interleaved estimate() to diff Forests
//        norm_groups.normalize(rule_weights.begin());
    }
    void end_visit() {
        delete forest_prep;
        if (collect_counts) {
            counts_accum.finish_counts();
            DBPC3("Done collecting counts:",forest_no,total_logprob/forest_no);
            DBP_ADD_VERBOSE(4);
            DBP(counts);
            counts_accum.print(logstream);
        }
    }

public:
    bool collect_counts;
    bool first_time;
// assumes you already prepare_em
    void prep_final_per_forest_counts(std::ostream &out)
    {
        per_forest_counts_go=true;
        per_forest_counts_out=&out;
        logstream << "Running final per-forest counts collection.\n";
    }

// assumes you already prepare_em
    void prep_final_per_forest_inside(std::ostream &out)
    {
        per_forest_inside_go=true;
        per_forest_inside_out=&out;
        logstream << "Running final per-forest inside score printing.\n";
    }

    // assumes you already prepare_em
    void prep_final_viterbi(std::ostream &out)
    {
        viterbi_go=true;
        viterbi_out=&out;
        logstream << "Running final viterbi forests decoding.\n";
    }

    // call after prep_final_per_forest_counts and/or prep_final_viterbi
    void final_iteration()
    {
        SetLocal<bool> g2(collect_counts,false);
        SetLocal<unsigned> g1(viterbi_period,1);
        SetLocal<unsigned> g3(per_forest_counts_period,1);
        logstream << "Repeating final iteration ...";
        estimate_visit();
        logstream << "\n";

    }
    typedef typename Forest::count_overflows count_overflows;
    count_overflows overflows;
    //for estimate/estimate_visit:
    void operator()(Forest &f) {
        BACKTRACE;
        ticker();
        ++forest_no;
        inside_t sumptrees = f.compute_inside();
        if (collect_counts) {
            f.collect_counts(counts_accum);
            DBP(array<inside_t>(outside.begin(),outside.begin()+f.size()));
            DBP_ADD_VERBOSE(3);
            DBP(counts);
        }
        double logprob=sumptrees.getLn();
        if ( inside[0].isZero() ) {
            if (first_time) {
                logstream << "Warning: 0 probability for forest #" << forest_no << std::endl;
            }
            ++n_zeroprob;
        } else {
            total_logprob += logprob;
        }
        DBPC5("Done with forest",forest_no,f,total_logprob,logprob);
        DBP_SCOPE;
        DBP(array<inside_t>(inside.begin(),inside.begin()+f.size()));

        if (per_forest_counts_go && forest_no % per_forest_counts_period == 0) {
            per_forest_counts.clear();
            if (!collect_counts) // on final iteration, may not ahve coolected counts above
                f.compute_norm_outside();
            f.visit_inside_norm_outside(boost::ref(*this));
            word_spacer sep;
            *per_forest_counts_out << '(';
            for (typename per_forest_counts_t::const_iterator i=per_forest_counts.begin(),e=per_forest_counts.end();i!=e;++i)
                *per_forest_counts_out << sep << i->first << ':' << i->second;
            *per_forest_counts_out << ")\n";
        }
        if (viterbi_go && forest_no % viterbi_period == 0) {
            f.compute_viterbi();
            f.write_viterbi(*viterbi_out,sumptrees);
            *viterbi_out << '\n';
        }
        if (per_forest_inside_go) {
            *per_forest_inside_out << sumptrees << '\n';
        }
    }

    unsigned size()
    {
        return total_forests;
    }

    double estimate(bool first_time_) {
        BACKTRACE;
        first_time=first_time_;
        collect_counts=true;
        watch_guard guard(this); // prepares viterbi and per_forest_counts if on a watch iteration
        estimate_visit();
        std::size_t N=forest_no-n_zeroprob;
        logstream << "\nN="<<N<< ' ';
        if (n_zeroprob)
            logstream << '('<< n_zeroprob << " 0 prob removed) ";
        return total_logprob/(forest_no-n_zeroprob);
    }
    void estimate_visit()
    {
        begin_visit();
        forests.enumerate(boost::ref(*this));
        end_visit();
    }

    // renormalizes parameters; learning_rate may be ignored, but is intended to magnify the delta from the previous parameter set to the normalized new parameter set.  should return largest absolute change to any parameter.  should also save the un-magnified (raw normalized counts) version for undo_maximize (if you only use learning_rate==1, then you don't need to do anything but normalize)
    bool firsttime;
    void watch_report() {
        BACKTRACE;
        DBPC2("watch_report",watch_group);
        if (!watching_group)
            return;
        prob_t *rulebase=rule_weights.begin();
        NormGroup &wg=*watch_group;

        NormGroupIter b(wg.begin()), end(wg.end()), mid=b;

        indirect_gt<NormIndex,prob_t *> gtcmp(rulebase);
        unsigned group_size=wg.size();
        unsigned real_watch_depth=watch_depth;
        if (watch_depth > group_size) {
            mid = end;
            real_watch_depth = group_size;
        } else
            mid += watch_depth;
        if (is_sorted(b,mid,gtcmp) && !firsttime) {
            logstream << " (no change in rank order of top " << real_watch_depth << " rules)";
        } else {
            if (mid != end) {
                std::partial_sort(b,mid,end,gtcmp);
            } else {
                std::sort(b,end,gtcmp);
            }
            logstream << "\nNew top " << real_watch_depth << " rules for normalization group:";
            for (;b!=mid;++b) {
                unsigned ruleid=Norms::get_index(*b);
                prob_t ruleweight=rulebase[ruleid];
                logstream << boost::format("\n%1% %|15t|%2% (id = %3%)") % ruleweight % rule_names[ruleid-1] % ruleid;
            }
            logstream << std::endl;
        }
    }
    void normalize() {
        BACKTRACE;
        norm_groups.normalize(rule_weights.begin());
    }
    bool on_watch_iteration() const
    {
        return iteration <= watch_period || (watch_period && iteration % watch_period  == 0);
    }
    unsigned log_level;
    ParamDelta maximize(double learning_rate) {
        BACKTRACE;

        DBPC2("maximize-pre",counts);DBP_SCOPE;
        std::ostream *logs=NULL;
        if (firsttime) {
            firsttime=false;
            if (log_level > 1) logs=&logstream;
        }
        int zcounts=zero_zerocounts? norm_groups.ZERO_ZEROCOUNTS : norm_groups.UNIFORM_ZEROCOUNTS;
        norm_groups.normalize(counts.begin(),rule_weights.begin(),zcounts,logs);
        DBPC2("maximize-post",rule_weights);
        if (on_watch_iteration()) {
            watch_report();
            if (checkpoint_parameters)
                dump_params();
            if (count_report_enable) {
                unsigned n_prob=0,n_count=0;
                for (typename auto_array<count_t>::const_iterator i=counts.begin()+1,e=counts.end();i!=e;++i) {
                    if (*i >= count_report_threshold)
                        ++n_count;
                    if (*i >= prob_report_threshold)
                        ++n_prob;
                }
                logstream<<" (out of " << counts.size()-1 << " parameters, " << n_count << " had count > " << count_report_threshold << ", and " << n_prob << " had prob > " << prob_report_threshold << ")" ;
            }
        }
        ++iteration;
        return ParamDelta(norm_groups.maxdiff.getReal(),norm_groups.maxdiff_index);
    }
    // for overrelaxed EM: if probability gets worse, reset learning rate to 1 and use the last improved weights
    void undo_maximize() {}

     // if you're doing random restarts, transfer the current parameters to safekeeping
    void save_best() {
        BACKTRACE;
        if (save_best_enable)
            rule_weights.copyto(best_weights.begin());
    }
    void restore_best() {
        BACKTRACE;
        if (save_best_enable) {
            best_weights.copyto(rule_weights.begin());
            watch_report();
            logstream << std::endl;
        }
    }
     // swaps estimated counts for current parameters
    void swap_counts() {
        BACKTRACE;
        counts.swap(rule_weights);
    }
    size_t n_zeroprob;
    bool zero_zerocounts;

    auto_array<Float> alphas; // -1 = locked
    void read_alphas()
    {
        if (gopt.alpha_file) {
            alphas.read(gopt.alpha_file.stream());
        }
    }

    Float alpha(unsigned i)
    {
        return i<alphas.size() ? alphas[i] : gopt.alpha;
    }

    void define_gibbs(bool norm=true)
    {
        if (norm) normalize();
        unsigned nid=rulespace;
        norm_groups.visit_norm_param(*this,nid);
    }
    /* for define_gibbs:*/
    // normid starts at 1, but p may be sparse
    void operator()(unsigned normid,typename Norms::index_type p,double normsz)
    {
        Float a=alpha(p);
        if (a<0) (*this)(p);
        else
            define_param_id(p,normid,rule_weights[p],a,normsz);
    }
    void operator()(typename Norms::index_type p)
    {
        define_param_id(p,rule_weights[p]);
    }

    void run_gibbs()
    {
        assert(gibbs);
        to_gibbs();
        gibbs_base::run_starts(*this);
        gibbs_base::print_all(*this);
        from_gibbs();
    }

    void to_gibbs()
    {
        gibbs_base::init(n_nodes,total_forests); //FIXME: input # of "symbols" by cmd line since avg derivtree size != #symbols
        for (unsigned i=0,nn=norm_groups.num_groups();i!=nn;++i) {
            prior_scale.add(i);
            prior_scale.finish_scalegroup();
        } // default: each normgroup in its own singleton inference group
        read_alphas();
        define_gibbs(true);
        alphas.clear();
        finish_params();
    }
    void from_gibbs()
    {
        //TODO: save unnormed counts also?
        for (unsigned i=0;i<rulespace;++i) {
            rule_weights[i]=gibbs_base::final_prob(i);
        }
    }
    // for gibbs_base:
    void init_run(unsigned r)
    {
    }
    void init_iteration(unsigned i)
    {
    }
    block_t *blockp;
    void resample_block(unsigned block)
    {
        Forest &f=forests[block];
        f.compute_inside(inside.begin(),*this);
        if (gopt.expectation) {
            unimplemented("--expectation in forest-em not yet implemented");
        } else {
            blockp=&sample[block].id;
            f.choose_random(inside.begin(),*this,power);
        }

    }
    Weight operator()(unsigned i) const { return proposal_prob(i); } // for compute_inside
    void record(unsigned rule) // for choose_random
    {
        blockp->push_back(rule);
    }

    // TODO: record parens or arity of rules so can show tree
    void print_deriv(std::ostream &o,block_t const& p)
    {
        graehl::word_spacer sp;
        for (block_t::const_iterator i=p.begin(),e=p.end();i!=e;++i) {
            o << sp << *i;
        }
    }

    void print_sample(blocks_t const& sample)
    {
        if (!gopt.sample_file) return;
        print_sample(gopt.sample_file.stream(),sample);
    }
    void print_sample(std::ostream &o,blocks_t const& sample)
    {
        if (gopt.expectation) unimplemented("can't print sample for --expectation (yet)");
        for (blocks_t::const_iterator i=sample.begin(),e=sample.end();i!=e;++i) {
            print_deriv(o,i->id);
            o << "\n";
        }
//        out<<"\n";
    }
    void print_param(std::ostream &out,unsigned parami) const
    {
//        out<<"\n";
    }
    void print_periodic()
    {
        print_all(*this);
    }

};

}




#endif
