#ifndef GRAEHL_SHARED__GIBBS_HPP
#define GRAEHL_SHARED__GIBBS_HPP

#include <graehl/shared/delta_sum.hpp>
#include <graehl/shared/time_series.hpp>
#include <boost/program_options.hpp>

namespace graehl {

struct gibbs_opts
{
    static char const* desc() { return "Gibbs (chinese restaurant process) options"; }
    template <class OD>
    void add_options(OD &opt, bool carmel_opts=false)
    {
        using boost::program_options::bool_switch;
        opt.add_options()
            ("crp", defaulted_value(&iter),
             "# of iterations of Bayesian 'chinese restaurant process' parameter estimation (Gibbs sampling) instead of EM")
            ("crp-exclude-prior", bool_switch(&exclude_prior),
             "When writing .trained weights, use only the expected counts from samples, excluding the prior (p0) counts")
            ("cache-prob", bool_switch(&cache_prob),
             "Show the true probability according to cache model for each sample")
            ("sample-prob", bool_switch(&cache_prob),
             "Show the sample prob given model, previous sample")
            ("high-temp", defaulted_value(&high_temp),
             "Raise probs to 1/temp power before making each choice - deterministic annealing for --unsupervised")
            ("low-temp", defaulted_value(&low_temp),
             "See high-temp. temperature is high-temp @i=0, low-temp @i=finaltemperature at final iteration (linear interpolation from high->low)")
            ("burnin", defaulted_value(&burnin),
             "When summing gibbs counts, skip <burnin> iterations first (iteration 0 is a random derivation from initial weights)")
            ("final-counts", bool_switch(&final_counts),
             "Normally, counts are averaged over all the iterations after --burnin.  this option says to use only final iteration's (--burnin is ignored; effectively set burnin=# crp iter)")
            ;
        if (carmel_opts)
            opt.add_options()
                ("crp-restarts", defaulted_value(&restarts),
                 "Number of additional runs (0 means just 1 run), using cache-prob at the final iteration select the best for .trained and --print-to output.  --init-em affects each start.  TESTME: print-every with path weights may screw up start weights")
                ("crp-argmax-final",bool_switch(&argmax_final),
                 "For --crp-restarts, choose the sample/.trained weights with best final sample cache-prob.  otherwise, use best entropy over all post --burnin samples")
                ("crp-argmax-sum",bool_switch(&argmax_sum),
                 "Instead of multiplying the sample probs together and choosing the best, sum (average) them")
                ("print-from",defaulted_value(&print_from),
                 "For [print-from]..([print-to]-1)th input transducer, print the final iteration's path on its own line.  a blank line follows each training example")
                ("print-to",defaulted_value(&print_to),
                 "See print-from")
                ("print-every",defaulted_value(&print_every),
                 "With --print-to, print the 0th,nth,2nth,,... (every n) iterations as well as the final one.  these are prefaced and suffixed with comment lines starting with #")
                ("print-counts-from",defaulted_value(&print_counts_from),
                 "Every --print-every, print the instantaneous and cumulative counts for parameters m...(n-1) (for debugging)")
                ("print-counts-to",defaulted_value(&print_counts_to),
                 "See print-counts-from")
                ("init-em",defaulted_value(&init_em),
                 "Perform n iterations of EM to get weights for randomly choosing initial sample, but use initial weights (pre-em) for p0 base model; note that EM respects tied/locked arcs but --crp removes them")
                ("em-p0",bool_switch(&em_p0),
                 "With init-em=n, use the trained weights as the base distribution as well (note: you could have done this in a previous carmel invocation, unlike --init-em alone)")
                ("uniform-p0",bool_switch(&uniformp0),
                 "Use a uniform base probability model for --crp, even when the input WFST have weights")
                ;
    }
    unsigned iter;
    unsigned restarts;
    unsigned init_em;
    unsigned burnin;
    bool em_p0;
    bool cache_prob;
    bool ppx;
    bool uniformp0;
    unsigned print_every; // print the current sample every N iterations
    unsigned print_from;
    unsigned print_to;
    unsigned print_counts_from;
    unsigned print_counts_to;
    bool final_counts;
    bool argmax_final;
    bool argmax_sum;
    bool exclude_prior;
    // random choices have probs raised to 1/temperature(iteration) before coin flip
    typedef clamped_time_series<double> temps;
    temps temp;
    temps temperature() const {
        return temps(high_temp,low_temp,iter,temps::linear);
    }
    double high_temp,low_temp;
    gibbs_opts() { set_defaults(); }
    void set_defaults()
    {
        iter=0;
        burnin=0;
        restarts=0;
        init_em=0;
        em_p0=false;
        cache_prob=false;
        print_counts_from=print_counts_to=0;
        uniformp0=false;
        ppx=true;
        print_every=0;
        print_from=print_to=0;
        high_temp=low_temp=1;
        final_counts=false;
        argmax_final=false;
        argmax_sum=false;
        exclude_prior=false;
    }
    void validate()
    {
if (final_counts) burnin=iter-1;

        if (restarts>0)
            cache_prob=true;
//            if (!cumulative_counts) argmax_final=true;
        temp=temperature();
    }
};

//template <class F>
struct gibbs_param
{
    double prior; //FIXME: not needed except to make computing --cache-prob easier
    inline double count() const
    {
        return sumcount.x;
    }
    unsigned norm; // index shared by all params to be normalized w/ this one
    delta_sum sumcount; //FIXME: move this to optional parallel array for many-parameter non-cumulative gibbs.
    template <class Normsum>
    void restore_p0(Normsum &ns)
    {
        ns[norm]+=prior;
        sumcount.clear(prior);
    }
    template <class Normsums>
    void addsum(double d,double t,Normsums &ns) // first call(s) should be w/ t=0
    {
        ns[norm]+=d;
        sumcount.add_delta(d,t);
    }
    gibbs_param(unsigned norm, double prior) : prior(prior),norm(norm) {}
    typedef gibbs_param self_type;
    TO_OSTREAM_PRINT
    template <class O>
    void print(O &o) const
    {
        o<<norm<<'\t'<<sumcount;
    }
};

}

#endif
