#ifndef GRAEHL_SHARED__GIBBS_HPP
#define GRAEHL_SHARED__GIBBS_HPP

#include <graehl/shared/delta_sum.hpp>
#include <graehl/shared/time_series.hpp>

namespace graehl {

struct gibbs_opts
{
    unsigned restarts;
    unsigned init_em;
    unsigned burnin;
    bool em_p0;
    bool cache_prob;
    bool ppx;
    bool p0init;
    unsigned print_every; // print the current sample every N iterations
    unsigned print_from;
    unsigned print_to;
    unsigned print_counts_from;
    unsigned print_counts_to;
    bool cumulative_counts;
    bool argmax_final;
    bool argmax_sum;
    bool exclude_prior;
    // random choices have probs raised to 1/temperature(iteration) before coin flip
    typedef clamped_time_series<double> temps;
    temps temperature(double iters) const {
        return temps(high_temp,low_temp,iters,temps::linear);
    }
    double high_temp,low_temp;
    gibbs_opts() { set_defaults(); }
    void set_defaults()
    {
        burnin=0;
        restarts=0;
        init_em=0;
        em_p0=false;
        cache_prob=false;
        print_counts_from=print_counts_to=0;
        p0init=true;
        ppx=true;
        print_every=0;
        print_from=print_to=0;
        high_temp=low_temp=1;
        cumulative_counts=true;
        argmax_final=false;
        argmax_sum=false;
        exclude_prior=false;
    }
    void validate()
    {
        if (restarts>0)
            cache_prob=true;
//            if (!cumulative_counts) argmax_final=true;
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
