#ifndef GRAEHL_SHARED__GIBBS_HPP
#define GRAEHL_SHARED__GIBBS_HPP

#include <graehl/shared/gibbs_opts.hpp>
#include <iomanip>
#include <graehl/shared/stream_util.hpp>
#include <graehl/shared/time_space_report.hpp>
#include <graehl/shared/periodic.hpp>
#include <graehl/shared/maybe_update_bound.hpp>
#include <graehl/shared/delta_sum.hpp>
#include <graehl/shared/print_width.hpp>


#define DEBUG_GIBBS
#ifdef DEBUG_GIBBS
#define DGIBBS(a) a;
#else
#define DGIBBS(a)
#endif
#define OUTGIBBS(a) DGIBBS(std::cerr<<a)


namespace graehl {


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
    void restore_p0(Normsum &ns) //ns must have been init to all 0s before the restore_p0 calls on each param
    {
        ns[norm]+=prior;
        sumcount.clear(prior);
        assert(sumcount.tmax==0);
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
        o<<norm<<'\t'<<sumcount<<" prior="<<prior;
    }
};

struct gibbs_base
{
    gibbs_base(gibbs_opts const &gopt_
               , unsigned n_sym=1
               , unsigned n_blocks=1
               , std::ostream &out=std::cout
               , std::ostream &log=std::cerr)
        : gopt(gopt_)
        , n_sym(n_sym)
        , n_blocks(n_blocks)
        , out(out)
        , log(log)
        , nnorm(0)
        , sample(n_blocks)
    {
        gopt.validate();
        temp=gopt.temp;
//        OUTGIBBS("temp="<<temp<<"\n");
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
    double temperature;
    double power; // for deterministic annealing (temperature) = 1/temperature if temp positive.
 private:
    normsum_t ccount,csum,pcount,psum; // for computing true cache model probs
    unsigned iter,Ni; // i=0 is random init sample.  i=1...Ni are the (gibbs resampled) samples
    double time; // at i=gopt.burnin, t=0.  at tmax = nI-gopt.burnin, we have the final sample.
    gibbs_opts::temps temp;
    bool accum_delta;
 public:
    bool burning() const
    {
        return iter>=gopt.burnin;
    }
    double final_t() const
    {
        return Ni-gopt.burnin;
    }
    unsigned size() const { return gps.size(); }

    // add params, then restore_p0() will be called on run()
    unsigned define_param(unsigned norm, double prior=0)
    {
        maybe_increase_max(nnorm,norm+1);
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
        double tmax1=final_t()+1; // if we don't add 1, then the last iteration's value isn't included in average
        double EPSILON=0; //1e-30; // avoid divide by 0 when normalizing
        for (gps_t::iterator i=gps.begin(),e=gps.end();i!=e;++i) {
            gibbs_param &g=*i;
            delta_sum &d=g.sumcount;
            if (gopt.exclude_prior)
                d.addbase(EPSILON-g.prior);
            if (!gopt.final_counts) {
                d.extend(tmax1);
                d.x=d.s;
            }
        }
        compute_norms();
    }
    void compute_norms()
    {
        normsum.reinit_nodestroy(nnorm);
        for (gps_t::iterator i=gps.begin(),e=gps.end();i!=e;++i)
            i->add_norm(normsum);
    }

    // save/restore params
    void save_counts(saved_counts_t &c)
    {
        c.reinit_nodestroy(size());
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
        assert(prob<=one_weight());
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
        assert(time>=p.sumcount.tmax);
        p.addsum(delta,time,normsum); //t=0 and accum_delta=false until burnin
    }
    void addc(unsigned param,double delta)
    {
        addc(gps[param],delta);
    }
    void addc(block_t &b,double delta)
    {
        assert(time>=0);
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
        restore_p0();
        imp.init_run(runi);
        accum_delta=false;
        iter=0;
        time=0;
        if (gopt.print_every!=0 && gopt.print_counts_sparse==0) {
            out<<"# ";
            print_counts(imp,true,"(prior counts)");
        }
        iteration(imp,false); // random initial sample
        //FIXME: isn't really random!  get the same sample after every iteration
        for (iter=1;iter<=Ni;++iter) {
            time=(double)iter-(double)gopt.burnin; //very funny: unsigned arithmetic -> double (unsigned maximum) if you're sloppy
            if (time>=0) accum_delta=true;
            else time=0;
            iteration(imp,true);
        }
        log<<"\nGibbs stats: "<<stats<<"\n";
        return stats;
    }

    template <class G>
    void iteration(G &imp,bool subtract_old=true)
    {
        temperature=temp(iter);
        power=(temperature>0)?1./temperature:1;
        itername(log);
        if (gopt.cache_prob) reset_cache();
        Weight p=1;
        imp.init_iteration(iter);
        for (unsigned b=0;b<n_blocks;++b) {
            num_progress(log,b,gopt.tick_every);
            block_t &block=sample[b];
            if (subtract_old)
                addc(block,-1);
            block.clear();
            imp.resample_block(b);
            addc(block,1);
            p*=prob(block);
        }
        record_iteration(p);
        maybe_print_periodic(imp);
    }
    unsigned beststart;
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
            graehl::time_space_report(log,"Gibbs sampling run: ");
            if (re>0) log<<"(random restart "<<r<<" of "<<re<<"): ";
            log<<"\n";
            gibbs_stats const& s=run(r,imp);
            if (r==0 || s.better(best,gopt)) {
                beststart=r;
                log << "\nNew best: "<<s<<"\n";
                best=s;
                finalize_cumulative_counts();
                save_counts(best_counts);
                best_sample.swap(sample);
            }
        }
        best_sample.swap(sample);
        if (re>0)
            restore_counts(best_counts);
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
        p.print_ppx(log,n_sym,n_blocks,"per-point-ppx","per-block-ppx","prob");
        log<<'\n';
        if (burning())
            stats.record(time,p);
    }
    std::ostream &itername(std::ostream &o,char const* suffix=" ") const
    {
        o<<"Gibbs i="<<iter;
        //o<<" time="<<time;
        if (!temp.is_constant()) {
            o<<" temperature="<<temperature;
            o<<" power="<<power;
        }
        o<<suffix;
        return o;
    }
    static inline bool divides(unsigned div,unsigned i)
    {
        return div!=0 && i%div==0;
    }
    template <class G>
    void maybe_print_periodic(G &imp)
    {
        if (divides(gopt.print_every,iter)) {
            itername(out<<"# ","\n");
            print_all(imp,false);
        }
    }
    void print_norms(char const* name="normalization group sums")
    {
        if (!gopt.printing_norms())
            return;
        unsigned from=gopt.print_norms_from;
        unsigned to=std::min(gopt.print_norms_to,normsum.size());
        if (to>from) {
            out <<"\n# group\t"<<name<<" i="<<iter<<"\n";
            print_range_i(out,normsum,from,to,true,true);
        }

    }
    void print_field(double d)
    {
        out<<'\t';
        print_width(out,d,gopt.width);
    }
    template <class G>
    void print_counts(G &imp,bool final=false,char const* name="")
    {
        unsigned width=gopt.width;
        if (width<4) width=20;
/*        local_precision<std::ostream> prec(out,width);
        local_stream_format<std::ostream> save(out);
        out.precision(width);
        out<<std::setw(width);*/
        if (!gopt.printing_counts())
            return;
        unsigned from=gopt.print_counts_from;
        unsigned to=std::min(gopt.print_counts_to,gps.size());
        if (to>from) {
            out<<"\n#";
            bool sparse=gopt.print_counts_sparse!=0;
            if (sparse)
                out<<"id\t";
            out<<"group\tcounts\tprob";
            double ta=time+1;
            if (!final)
                out<<"\tavg@"<<ta<<"\tlast@t\tprior";
            if (gopt.rich_counts)
                out<<"\tparam name";
            if (!final)
                out<<"\titer="<<iter;
            out<<"\t"<<name;
            out<<'\n';
            for (unsigned i=from;i<to;++i) {
                gibbs_param const& p=gps[i];
                delta_sum const& d=p.sumcount;
                double avg=final?d.x/ta:d.avg(ta);
                if (!sparse || avg>=p.prior+gopt.print_counts_sparse) {
                    if (sparse)
                        out<<i<<'\t';
                    out<<p.norm;
                    print_field(final?avg:d.x); //NOTE: for final, only the counts were restored upon restarts, so don't use anything other than d.x
                    print_field(proposal_prob(i));
                    if (!final) {
                        print_field(avg);
                        print_field(d.tmax);
                        print_field(p.prior);
                    }
                    if (gopt.rich_counts) {
                        out<<'\t';
                        imp.print_param(out,i);
                    }
//                    out<<'\t'<<d.s;
                    out<<'\n';
                }
            }
        }
        out<<'\n';
    }
    template <class G>
    void print_all(G &imp,bool final=true)
    {
        if (final)
            out<<"# final best gibbs run (start #<<"<<beststart<<"):\n";
        if (gopt.printing_sample())
            imp.print_sample(sample);
        print_norms();
        print_counts(imp,final);
    }

};

/* GE inherits from gibbs_base.

   init_run(r): for r=[0,gopt.restarts]
   init_iteration(i)
   resample_block(blocki): for blocki=[0,n_pairs): choose new random sample[blocki] using p^power
   print_sample(sample):
   print_param(out,parami): like out<<gps[i] but customized
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
