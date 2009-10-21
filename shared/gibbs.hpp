#ifndef GRAEHL_SHARED__GIBBS_HPP
#define GRAEHL_SHARED__GIBBS_HPP

#include <graehl/shared/time_space_report.hpp>
#include <graehl/shared/periodic.hpp>
#include <graehl/shared/maybe_update_bound.hpp>
#include <graehl/shared/delta_sum.hpp>
#include <graehl/shared/time_series.hpp>
#include <graehl/shared/stream_util.hpp>
#include <graehl/shared/weight.h>
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
            ("sample-prob", bool_switch(&ppx),
             "Show the sample prob given model, previous sample")
            ("high-temp", defaulted_value(&high_temp),
             "Raise probs to 1/temp power before making each choice - deterministic annealing for --unsupervised")
            ("low-temp", defaulted_value(&low_temp),
             "See high-temp. temperature is high-temp @i=0, low-temp @i=finaltemperature at final iteration (linear interpolation from high->low)")
            ("burnin", defaulted_value(&burnin),
             "When summing gibbs counts, skip <burnin> iterations first (iteration 0 is a random derivation from initial weights)")
            ("final-counts", bool_switch(&final_counts),
             "Normally, counts are averaged over all the iterations after --burnin.  this option says to use only final iteration's (--burnin is ignored; effectively set burnin=# crp iter)")
            ("print-counts-from",defaulted_value(&print_counts_from),
             "Every --print-every, print the instantaneous and cumulative counts for parameters from...(to-1) (for debugging)")
            ("print-counts-to",defaulted_value(&print_counts_to),
             "See print-counts-from")
            ("print-normsum-from",defaulted_value(&print_normsum_from),
             "Every --print-every, print the normalization groups' instantaneous (proposal HMM) sum-counts, for from...(to-1)")
            ("print-normsum-to",defaulted_value(&print_normsum_to),
             "See print-normsum-to")
            ("print-every",defaulted_value(&print_every),
             "print the 0th,nth,2nth,,... (every n) iterations as well as the final one.  these are prefaced and suffixed with comment lines starting with #")
            ("crp-restarts", defaulted_value(&restarts),
             "Number of additional runs (0 means just 1 run), using cache-prob at the final iteration select the best for .trained and --print-to output.  --init-em affects each start.  TESTME: print-every with path weights may screw up start weights")
            ("crp-argmax-final",bool_switch(&argmax_final),
             "For --crp-restarts, choose the sample/.trained weights with best final sample cache-prob.  otherwise, use best entropy over all post --burnin samples")
            ("crp-argmax-sum",bool_switch(&argmax_sum),
             "Instead of multiplying the sample probs together and choosing the best, sum (average) them")
            ;
        if (carmel_opts)
            opt.add_options()
                ("print-from",defaulted_value(&print_from),
                 "For [print-from]..([print-to]-1)th input transducer, print the final iteration's path on its own line.  a blank line follows each training example")
                ("print-to",defaulted_value(&print_to),
                 "See print-from")
                ("init-em",defaulted_value(&init_em),
                 "Perform n iterations of EM to get weights for randomly choosing initial sample, but use initial weights (pre-em) for p0 base model; note that EM respects tied/locked arcs but --crp removes them")
                ("em-p0",bool_switch(&em_p0),
                 "With init-em=n, use the trained weights as the base distribution as well (note: you could have done this in a previous carmel invocation, unlike --init-em alone)")
                ("uniform-p0",bool_switch(&uniformp0),
                 "Use a uniform base probability model for --crp, even when the input WFST have weights.  --em-p0 overrides this.")
                ;
    }
    //forest-em and carmel supported:
    unsigned iter;
    bool exclude_prior;
    bool cache_prob;
    bool ppx;
    double high_temp,low_temp;
    unsigned burnin;
    bool final_counts;
    unsigned print_every; // print the current sample every N iterations
    unsigned print_counts_from,print_counts_to; // which param ids' counts to print
    unsigned print_normsum_from,print_normsum_to; // which normgroup ids' sums to print

    unsigned restarts; // 0 = 1 run (no restarts)

     // criteria to max over restarts:
    bool argmax_final;
    bool argmax_sum;

    //carmel only:
    unsigned init_em;
    bool em_p0;
    bool uniformp0;
    unsigned print_from,print_to; // which blocks to print

    bool printing_derivs() const
    {
        return print_to>print_from;
    }

    bool printing_counts() const
    {
        return print_counts_to>print_counts_from;
    }

    bool printing_norms() const
    {
        return print_normsum_to>print_normsum_from;
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
        iter=0;
        burnin=0;
        restarts=0;
        init_em=0;
        em_p0=false;
        cache_prob=false;
        print_counts_from=print_counts_to=0;
        print_normsum_from=print_normsum_to=0;
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
    double count() const
    {
        return sumcount.x;
    }
    double& count()
    {
        return sumcount.x;
    }
    unsigned norm; // index shared by all params to be normalized w/ this one
    delta_sum sumcount; //FIXME: move this to optional parallel array for many-parameter non-cumulative gibbs.
    template <class Normsum>
    void restore_p0(Normsum &ns)
    {
        sumcount.clear(prior);
        add_norm(ns);
    }
    template <class Normsum>
    void add_norm(Normsum &ns) const
    {
        ns[norm]+=count();
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


struct gibbs_stats
{
    double N; // from burnin...last iter, or just last if --final-counts
    Weight sumprob; //FIXME: more precision, or (scale,sum) pair
    Weight allprob,finalprob; // (prod over all N, and final) sample cache probs
    typedef gibbs_stats self_type;
    gibbs_stats() {clear();}
    void clear()
    {
        allprob.setOne();
        finalprob.setOne();
        sumprob=0;
        N=0;
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
    void print(O&o) const
    {
        o << "#samples="<<N<<" final sample ppx="<<finalprob.ppxper().as_base(2)<<" burned-in avg="<<allprob.ppxper(N).as_base(2);
    }
    bool better(gibbs_stats const& o,gibbs_opts const& gopt) const
    {
        return gopt.argmax_final ? finalprob>o.finalprob : (gopt.argmax_sum ? sumprob>o.sumprob : allprob>o.allprob);
    }
};

struct gibbs_base
{
    gibbs_base(gibbs_opts const &gopt
               , unsigned n_sym=1
               , unsigned n_blocks=1
               , std::ostream &out=std::cout
               , std::ostream &log=std::cerr)
        : gopt(gopt)
        , n_sym(n_sym)
        , n_blocks(n_blocks)
        , out(out)
        , log(log)
        , nnorm(0)
        , sample(n_blocks)
    {
        this->gopt.validate();
        temp=gopt.temp;
    }
    gibbs_stats stats;
    gibbs_opts gopt;
    std::ostream &out;
    std::ostream &log;
    typedef gibbs_param gp_t;
    typedef fixed_array<double> normsum_t;
    typedef normsum_t saved_counts_t;
    typedef dynamic_array<unsigned> block_t; // indexes into gps
    typedef fixed_array<block_t> blocks_t;
    typedef dynamic_array<gp_t> gps_t;
    unsigned n_sym,n_blocks;
    gps_t gps;
    unsigned nnorm;
    normsum_t normsum;
    blocks_t sample;
    double power; // for deterministic annealing (temperature)
 private:
    normsum_t ccount,csum,pcount,psum; // for computing true cache model probs
    unsigned i,Ni; // i=0 is random init sample.  i=1...Ni are the gibbs samples
    double burnt0;
    double t; // at i=burnt0, t=0.  at tmax = nI-burnt0, we have the final sample.
    gibbs_opts::temps temp;
    bool subtract_old;
    bool accum_delta;
 public:
    unsigned size() const { return gps.size(); }

    // add params, then restore_p0() will be called on run()
    unsigned define_param(unsigned norm, double prior=0)
    {
        maybe_increase_max(nnorm,norm);
        unsigned r=gps.size();
        gps.push_back(norm,prior);
        return r;
    }
    void restore_p0()
    {
        normsum.reinit(nnorm);
        for (gps_t::iterator i=gps.begin(),e=gps.end();i!=e;++i)
            i->restore_p0(normsum);
    }

    // finalize avged counts over burned in iters
    void finalize_cumulative_counts()
    {
        if (gopt.final_counts && !gopt.exclude_prior)
            return;
        double tmax1=Ni+1-burnt0; //FIXME: check this gives the final iteration a single vote, not 0 or 2.
        for (gps_t::iterator i=gps.begin(),e=gps.end();i!=e;++i) {
            gibbs_param &g=*i;
            delta_sum &d=g.sumcount;
            if (gopt.exclude_prior)
                d.addbase(-g.prior);
            if (!gopt.final_counts) {
                d.extend(tmax1);
                d.x=d.s;
            }
        }
        compute_norms();
    }
    void compute_norms()
    {
        normsum.reinit(nnorm);
        for (gps_t::iterator i=gps.begin(),e=gps.end();i!=e;++i)
            i->add_norm(normsum);
    }

    // save/restore params
    void save_counts(saved_counts_t &c)
    {
        c.reinit(size());
        for (unsigned i=0,N=size();i<N;++i)
            c[i]=gps[i].count();
    }
    void restore_counts(saved_counts_t const& c)
    {
        for (unsigned i=0,N=size();i<N;++i)
            gps[i].count()=c[i];
    }
    void restore_probs(saved_counts_t const& c)
    {
        restore_counts(c);
        compute_norms();
    }

    // cache probability changes each time a param is used (it becomes more likely in the future)
    void init_cache()
    {
        unsigned N=gps.size();
        pcount.init(N);
        psum.init(nnorm);
        ccount.init(N);
        csum.init(nnorm);
        for (unsigned i=0;i<N;++i) {
            gibbs_param const& gp=gps[i];
            psum[gp.norm]+=(pcount[i]=gp.prior);
        }
        reset_cache();
    }
    void reset_cache()
    {
        ccount.uninit_copy_from(pcount);
        csum.uninit_copy_from(psum);
    }
    void free_cache()
    {
        ccount.dealloc();
        csum.dealloc();
    }
    Weight cache_prob(block_t const& p)
    {
        Weight prob=one_weight();
        for (block_t::const_iterator i=p.begin(),e=p.end();i!=e;++i)
            prob*=cache_prob(*i);
        return prob;
    }
    double cache_prob(unsigned paramid)
    {
        unsigned norm=gps[paramid].norm;
        return ccount[paramid]++/csum[norm]++;
    }

    //proposal HMM within an iteration:
    Weight proposal_prob(block_t const& p)
    {
        Weight prob=one_weight();
        for (block_t::const_iterator i=p.begin(),e=p.end();i!=e;++i)
            prob*=proposal_prob(*i);
        return prob;
    }
    double proposal_prob(unsigned paramid) const
    {
        return proposal_prob(gps[paramid]);
    }
    double proposal_prob(gibbs_param const&p) const
    {
        return p.count()/normsum[p.norm];
    }
    void addc(gibbs_param &p,double delta)
    {
        p.addsum(delta,t,normsum); //t=0 and accum_delta=false until burnin
    }
    void addc(unsigned param,double delta)
    {
        addc(gps[i],delta);
    }
    void addc(block_t &b,double delta)
    {
        for (block_t::const_iterator i=b.begin(),e=b.end();i!=e;++i)
            addc(*i,delta);
    }

 private:
    //actual impl:
    template <class G>
    gibbs_stats run(unsigned runi,G &imp)
    {
        stats.clear();
        Ni=gopt.iter;
        imp.init_run(runi);
        restore_p0();
        power=1.;
        accum_delta=false;
        i=0;
        if (gopt.print_every!=0) print_counts("prior counts");
        burnt0=gopt.final_counts ? Ni : gopt.burnin;
        subtract_old=false;
        iteration(imp); // random initial sample
        subtract_old=true;
        for (i=1;i<=Ni;++i) {
            power=1./temp(i);
            t=i-burnt0;
            if (t>=0) accum_delta=true;
            else if (t<0) t==0;
            iteration(imp);
        }
        log<<"\nGibbs stats: "<<stats<<"\n";
        return stats;
    }

    template <class G>
    void iteration(G &imp)
    {
        itername(log);
        if (gopt.cache_prob) reset_cache();
        Weight p=1;
        for (unsigned b=0;b<n_blocks;++b) {
            num_progress(log,b);
            block_t &block=sample[b];
            addc(block,-1);
            block.clear();
            imp.resample_block(i);
            addc(block,1);
            p*=prob(block);
        }
        record_iteration(p);
        maybe_print_periodic(imp);
    }
 public:
    template <class G>
    gibbs_stats run_starts(G &imp)
    {
        if (gopt.cache_prob) init_cache();
        saved_counts_t best_counts;
        blocks_t best_sample(n_blocks);
        gibbs_stats best;
        unsigned re=gopt.restarts;
        for (unsigned r=0;r<=re;++r) {
            graehl::time_space_report(Config::log(),"Gibbs restart:");
            if (re>0) log<<"\nRandom start "<<r<<"(of "<<re<<")\n";
            gibbs_stats const& s=run(r,imp);
            if (r==0 || s.better(best,gopt)) {
                log << "\nNew best: "<<s<<"\n";
                best=s;
                finalize_cumulative_counts();
                save_counts(best_counts);
                best_sample.swap(sample);
            }
        }
        if (re>0) {
            restore_counts(best_counts);
            best_sample.swap(sample);
        }
        if (gopt.cache_prob) free_cache();
        return best;
    }

    //logging:
    Weight prob(block_t const& b)
    {
        return gopt.cache_prob?cache_prob(b):proposal_prob(b);
    }
    void record_iteration(Weight p)
    {
        log<<" "<<(gopt.cache_prob?"cache-model":"sample")<<" ";
        p.print_ppx(log,n_sym,n_blocks);
        log<<'\n';
        stats.record(t,p);
    }
    std::ostream &itername(std::ostream &o,char const* suffix=" ") const
    {
        o<<"Gibbs i="<<i;
        if (!temp.is_constant())
            o<<" temperature="<<1./power<<" power="<<power<<suffix;
        return o;
    }
    static inline bool divides(unsigned div,unsigned i)
    {
        return div!=0 && i%div==0;
    }
    template <class G>
    void maybe_print_periodic(G &imp)
    {
        if (divides(gopt.print_every,i)) {
            itername(out<<"# ","\n");
            imp.print_sample(sample);
            print_norms();
            print_counts();
        }
    }
    void print_norms(char const* name="normalization group sums")
    {
        if (gopt.printing_norms())
            return;
        unsigned from=gopt.print_normsum_from;
        unsigned to=std::min(gopt.print_normsum_to,normsum.size());
        if (to>from) {
            out <<"#\nnormgrp\tsum\t<<name"<<" i="<<i<<"\n";
        }

    }
    void print_counts(char const* name="counts")
    {
        if (!gopt.printing_counts())
            return;
        unsigned from=gopt.print_counts_from;
        unsigned to=std::min(gopt.print_counts_to,gps.size());
        if (to>from) {
            out<<"#\nnormgrp\tcounts\t"<<name<<" i="<<i<<"\n";
            print_range(out,gps.begin()+from,gps.begin()+to,true,true);
        }
    }
};

/* GE inherits from gibbs_base.

   init_run(r): for r=[0,gopt.restarts]
   resample_block(i): for i=[0,n_pairs): choose new random sample[i] using p^power
   print_sample(sample):

 */



/*
template <class GE>
struct gibbs_crp : public gibbs_base
{
    gibbs_opts & opt;
    GE &imp;
    gibbs_crp(gibbs_opts & opt,GE &imp) : opt(opt),imp(imp),log(imp.log()) {  }

};
*/



}

#endif
