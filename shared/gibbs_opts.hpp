#ifndef GRAEHL_SHARED__GIBBS_OPTS_HPP
#define GRAEHL_SHARED__GIBBS_OPTS_HPP

//#include <boost/program_options.hpp>
#include <graehl/shared/time_series.hpp>
#include <graehl/shared/weight.h>
#include <graehl/shared/stream_util.hpp>

// avoid library linking dep for carmel (note: carmel should use boost options too eventually)
#ifdef FOREST_EM_VERSION
#include <graehl/shared/fileargs.hpp>
#endif


namespace graehl {

//TODO: parse unsigned so -1 is not error but rather max_unsigned?
struct gibbs_opts
{
    static char const* desc() { return "Gibbs (chinese restaurant process) options"; }
    template <class OD>
    void add_options(OD &opt, bool carmel_opts=false, bool forest_opts=false)
    {
        opt.add_options()
            ("crp", defaulted_value(&iter),
             "# of iterations of Bayesian 'chinese restaurant process' parameter estimation (Gibbs sampling) instead of EM")
            ("include-self",defaulted_value(&include_self)->zero_tokens(),
             "don't remove the counts from the current block in computing the proposal probabilities (this plus --expectation = incremental EM)")
            ("crp-exclude-prior", defaulted_value(&exclude_prior)->zero_tokens(),
             "When writing .trained weights, use only the expected counts from samples, excluding the prior (p0) counts")
            ("cache-prob", defaulted_value(&cache_prob)->zero_tokens(),
             "Show the true probability according to cache model for each sample")
            ("cheap-prob", defaulted_value(&cheap_prob)->zero_tokens(),
             "Show a cheaper-to-compute version of --cache-prob which doesn't update probs mid-block")
            ("no-prob", defaulted_value(&no_prob)->zero_tokens(),
             "Give no probability output")
            ("high-temp", defaulted_value(&high_temp),
             "Raise probs to 1/temp power before making each choice - deterministic annealing for --unsupervised")
            ("low-temp", defaulted_value(&low_temp),
             "See high-temp. temperature is high-temp @i=0, low-temp @i=finaltemperature at final iteration (linear interpolation from high->low)")
            ("burnin", defaulted_value(&burnin),
             "When summing gibbs counts, skip <burnin> iterations first (iteration 0 is a random derivation from initial weights)")
            ("final-counts", defaulted_value(&final_counts)->zero_tokens(),
             "Normally, counts are averaged over all the iterations after --burnin.  this option says to use only final iteration's (--burnin is ignored; effectively set burnin=# crp iter)")
            ("crp-restarts", defaulted_value(&restarts),
             "Number of additional runs (0 means just 1 run), using cache-prob at the final iteration select the best for .trained and --print-to output.  --init-em affects each start.  TESTME: print-every with path weights may screw up start weights")
            ("crp-argmax-final",defaulted_value(&argmax_final)->zero_tokens(),
             "For --crp-restarts, choose the sample/.trained weights with best final sample cache-prob.  otherwise, use best entropy over all post --burnin samples")
            ("crp-argmax-sum",defaulted_value(&argmax_sum)->zero_tokens(),
             "Instead of multiplying the sample probs together and choosing the best, sum (average) them")
            ("print-counts-from",defaulted_value(&print_counts_from),
             "Every --print-every, print the instantaneous and cumulative counts for parameters from...(to-1) (for debugging)")
            ("print-counts-to",defaulted_value(&print_counts_to),
             "See print-counts-from.  4294967295 means until end")
            ("print-counts-sparse",defaulted_value(&print_counts_sparse),
             "(if nonzero) skip printing counts with avg(count)<prior+sparse, and show parameter index")
            ("print-counts-rich",defaulted_value(&rich_counts)->zero_tokens(),
             "print a rich identifier for each parameter's counts")
            ("width",defaulted_value(&width),
             "limit counts/probs to this many printed characters")
            ("print-norms-from",defaulted_value(&print_norms_from),
             "Every --print-every, print the normalization groups' instantaneous (proposal HMM) sum-counts, for from...(to-1)")
            ("print-norms-to",defaulted_value(&print_norms_to),
             "See print-norms-to.  4294967295 means until end")
            ("print-every",defaulted_value(&print_every),
             "print the 0th,nth,2nth,,... (every n) iterations as well as the final one.  these are prefaced and suffixed with comment lines starting with #")
            ("progress-every",defaulted_value(&tick_every),
             "show a progress tick (.) every N blocks")
            ("prior-inference-stddev",defaulted_value(&prior_inference_stddev),
             "if >0, after each post burn-in iteration, allow each normalization group's prior counts to be scaled by some random ratio with stddev=this centered around 1; proposals that lead to lower cache prob for the sample tend to be rejected.  Goldwater&Griffiths used 0.1")
            ("prior-inference-global",defaulted_value(&prior_inference_global),"disregarding supplied hyper-normalization groups, scale all prior counts in the same direction.  BHMM1 in Goldwater&Griffiths")
            ("prior-inference-local",defaulted_value(&prior_inference_local),"disregarding supplied hyper-normalization groups, seperately scale prior counts for each multinomial (normalization group)")
            ("prior-inference-restart-fresh",defaulted_value(&prior_inference_restart_fresh),"on each random restart, reset the priors to their initial value (otherwise, let them drift across restarts); note: smaller priors generally memorize training data better.")
            ("prior-inference-show",defaulted_value(&prior_inference_show),"show for each prior group the cumulative scale applied to its prior counts")
            ("prior-inference-start",defaulted_value(&prior_inference_start),"(if nonzero) on iterations [start,end) do hyperparam inference; default is to do inference starting from --burnin, but this overrides that")
            ("prior-inference-end",defaulted_value(&prior_inference_end),"see above")
            ;
        if (forest_opts)
            opt.add_options()
                ("const-alpha",defaulted_value(&alpha),
                 "prior applied to initial param values: alpha*p0*N (where N is # of items in normgroup, so uniform has p0*N=1)")
                ("n-symbols",defaulted_value(&n_sym),
                 "N for per-point perplexity (total number of symbols the derivations explain).  there's no way to deduce this automatically since a single rule may produce multiple symbols")
#ifdef FOREST_EM_VERSION
                ("alpha",defaulted_value(&alpha_file),
                 "per-parameter alpha file parallel to -I (overrides const-alpha); negative alpha means locked (use init prob but don't update/normalize)")
                ("outsample-file",defaulted_value(&sample_file),
                 "print actual sample (tree w/o parens) to this file")
                ("print-file",defaulted_value(&print_file),
                 "print-counts and print-norms to this file (default stdout)")
#endif
                ;

        if (carmel_opts)
            opt.add_options()
                ("print-from",defaulted_value(&print_from),
                 "For [print-from]..([print-to]-1)th input transducer, print the final iteration's path on its own line.  a blank line follows each training example")
                ("print-to",defaulted_value(&print_to),
                 "See print-from. 4294967295 means until end")
                ("init-em",defaulted_value(&init_em),
                 "Perform n iterations of EM to get weights for randomly choosing initial sample, but use initial weights (pre-em) for p0 base model; note that EM respects tied/locked arcs but --crp removes them")
                ("em-p0",defaulted_value(&em_p0)->zero_tokens(),
                 "With init-em=n, use the trained weights as the base distribution as well (note: you could have done this in a previous carmel invocation, unlike --init-em alone)")
                ("uniform-p0",defaulted_value(&uniformp0)->zero_tokens(),
                 "Use a uniform base probability model for --crp, even when the input WFST have weights.  --em-p0 overrides this.")
              ("init-from-p0",defaulted_value(&init_from_p0)->zero_tokens(),
               "For the initial sample: normally previous blocks' cache is used for proposal prob.  With this option, each block is generated independently from the base distribution alone (resampling is unchanged).")
              ("dirichlet-p0",defaulted_value(&dirichlet_p0)->zero_tokens(),
               "Use the input WFST weights, UNNORMALIZED, as the dirichlet prior pseudocounts - this way different normgroups can have different effective alphas.  Note: alpha argument still further scales the initial psuedocounts, so set alpha=1.")
                ("norm-order",defaulted_value(&norm_order)->zero_tokens(),
                 "Print arc counts in normgroup (consecutive gibbs param id) order rather than WFST file order")
                ("expectation",defaulted_value(&expectation)->zero_tokens(),
                 "use full forward/backward fractional counts instead of a single count=1 random sample")
                ("random-start",defaulted_value(&random_start)->zero_tokens(),
                 "for expectation, scale the initial per-example counts by random [0,1).  without this, every run would have the same outcome.  this is implicitly enabled for restarts, of course.")
                ;
    }
    //forest-em and carmel supported:
    unsigned iter;
    bool norm_order;
    bool exclude_prior;
    bool cache_prob;
    bool cheap_prob;
    bool no_prob;
    double high_temp,low_temp;
    unsigned burnin;
    bool final_counts;
    unsigned print_every; // print the current sample every N iterations
    unsigned print_counts_from,print_counts_to; // which param ids' counts to print
    bool rich_counts; // show extra identification for each parameter, not just its id
    unsigned width; // # chars for counts/probs etc.
    double print_counts_sparse;
    unsigned print_norms_from,print_norms_to; // which normgroup ids' sums to print

    bool prior_inference_show;
    double prior_inference_stddev;
    bool prior_inference_global;
    bool prior_inference_local;
    bool prior_inference_restart_fresh;
    unsigned prior_inference_start,prior_inference_end;

    unsigned restarts; // 0 = 1 run (no restarts)
    unsigned tick_every;

     // criteria to max over restarts:
    bool argmax_final;
    bool argmax_sum;

    bool include_self; // don't remove counts from current block before creating proposal. expectation+include_self = incremental EM


    //carmel only:
    bool expectation; // instead of sampling, ask the gibbs impl. to compute full forward/backward fractional counts
    bool random_start;

    unsigned init_em;
    bool em_p0;
    bool uniformp0;
  bool dirichlet_p0;
  bool init_from_p0;
    unsigned print_from,print_to; // which blocks to print

    //forest-em only:
    double alpha; //TODO: per-normgroup (or per-param) alphas
    unsigned n_sym;

#ifdef FOREST_EM_VERSION
    ostream_arg sample_file,print_file;
    istream_arg alpha_file;
#endif
    bool printing_sample() const
    {
        return
#ifdef FOREST_EM_VERSION
            sample_file
#else
            print_to>print_from;
#endif
        ;
    }

    bool printing_counts() const
    {
        return print_counts_to>print_counts_from;
    }

    bool printing_norms() const
    {
        return print_norms_to>print_norms_from;
    }


    // random choices have probs raised to 1/temperature(iteration) before coin flip
    typedef clamped_time_series<double> temps;
    temps temp;
    temps temperature() const {
        return temps(high_temp,low_temp,iter,temps::linear);
    }
    gibbs_opts() { set_defaults(); }
    void set_defaults()
    {
        expectation=false;
        random_start=false;

        include_self=false;
        prior_inference_start=prior_inference_end=0;
        prior_inference_local=prior_inference_global=false;
        prior_inference_restart_fresh=false;
        prior_inference_stddev=0;
        n_sym=0;
#ifdef FOREST_EM_VERSION
        sample_file=ostream_arg();
        print_file=stdout_arg();
#endif
        rich_counts=false;
        alpha=.1;
        tick_every=0;
        width=7;
        iter=0;
        burnin=0;
        restarts=0;
        init_em=0;
        em_p0=false;
        print_counts_sparse=0;
        print_counts_from=print_counts_to=0;
        print_norms_from=print_norms_to=0;
        uniformp0=dirichlet_p0=init_from_p0=false;
        cheap_prob=false;
        no_prob=false;
        cache_prob=true;
        print_every=0;
        print_from=print_to=0;
        high_temp=low_temp=1;
        final_counts=false;
        argmax_final=false;
        argmax_sum=false;
        exclude_prior=false;
        norm_order=false;
    }
    void validate()
    {
        if (width<4) width=20;
        if (no_prob) {
            cache_prob=cheap_prob=false;
        }
//        if (tick_every<1) tick_every=100;
        if (final_counts) burnin=iter;
        if (burnin>iter)
            burnin=iter;
        if (restarts>0)
            cache_prob=true;
//            if (!cumulative_counts) argmax_final=true;
        temp=temperature();
    }
};



struct gibbs_stats
{
    double N; // from burnin...last iter, or just last if --final-counts
    double n_sym,n_blocks;

    Weight sumprob; //FIXME: more precision, or (scale,sum) pair
    Weight allprob,finalprob; // (prod over all N, and final) sample cache probs
    typedef gibbs_stats self_type;
    gibbs_stats() {clear();}
    void clear(double ns=1,double nb=1)
    {
        allprob.setOne();
        finalprob.setOne();
        sumprob=0;
        N=0;
        n_blocks=nb;
        n_sym=ns;
    }
    void record(double t,Weight prob)
    {
        if (t>=0) {
            N+=1;
            sumprob+=prob;
            allprob*=prob;
            finalprob=prob;
        }
    }
    TO_OSTREAM_PRINT
    template <class O>
    void print_ppx(O &o,Weight p) const
    {
        p.print_ppx(o,n_sym,n_blocks,"per-point-ppx","per-block-ppx","prob");
    }

    template <class O>
    void print(O&o) const
    {
        o << "final sample ";
        print_ppx(o,finalprob);
        o << "\n  burned-in avg (over "<<N<<" samples) ";
        print_ppx(o,allprob.root(N));
    }
    bool better(gibbs_stats const& o,gibbs_opts const& gopt) const
    {
        return gopt.argmax_final ? finalprob>o.finalprob : (gopt.argmax_sum ? sumprob>o.sumprob : allprob>o.allprob);
    }
};

}

#endif
