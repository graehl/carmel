#ifndef GRAEHL_SHARED__GIBBS_HPP
#define GRAEHL_SHARED__GIBBS_HPP

#include <graehl/shared/gibbs_opts.hpp>
#include <iomanip>
#include <graehl/shared/stream_util.hpp>
#include <graehl/shared/time_space_report.hpp>
#include <graehl/shared/periodic.hpp>
#include <graehl/shared/maybe_update_bound.hpp>
#include <graehl/shared/assoc_container.hpp>
#include <graehl/shared/delta_sum.hpp>
#include <graehl/shared/print_width.hpp>

#include <graehl/shared/debugprint.hpp>


#define DEBUG_GIBBS
#ifdef DEBUG_GIBBS
#define DGIBBS(a) a;
#else
#define DGIBBS(a)
#endif
#define OUTGIBBS(a) DGIBBS(std::cerr<<a)

/* Gibbs underlying implementation inherits from gibbs_base, and calls run_starts(*this)

   impl must call for each param one of:
       unsigned define_param(unsigned norm, double prior)
       unsigned define_param(unsigned norm, double prob, double alpha)
       where define_param(n,p,a) = define_param(n,gopt.uniformp0?alpha:alpha*prob)

   or (allowing out of order param id defn)
       void define_param_id(unsigned id,unsigned norm,double prob,double alpha)

       prior, when normalized, serves at the base distribution.

   the current (during resampling) prob of a param is gibbs_base::proposal_prob(id) or gibbs_base::proposal_prob(gps_t)

   init_run(r): for r=[0,gopt.restarts]
   init_iteration(i)
   resample_block(blocki): for blocki=[0,n_pairs): choose new random sample[blocki] using p^power (this->power, don't forget to use it :)
   print_sample(sample):
   print_param(out,parami): like out<<gps[i] but customized

   void print_counts(bool final=false,char const* name="") {
     print_counts_default(*this,final,name);
   }
   void print_periodic()
    {
        print_all(*this);
    }

    void run_gibbs()
    {
        assert(gibbs);
        gibbs_base::init(n_nodes,total_forests); //FIXME: input # of "symbols" by cmd line since avg derivtree size != #symbols
        gibbs_base::run_starts(*this);
        gibbs_base::print_all(*this);
    }
    void init_run(unsigned r)
    {
    }
    void init_iteration(unsigned i)
    {
    }
    block_t *blockp;
    void resample_block(unsigned block)
    {
        block_t &b=sample[block];  // this is cleared for us already
        blockp=&b;
    }
    void print_sample(blocks_t const& sample)
    {
        out<<"\n";
    }
    void print_param(std::ostream &out,unsigned parami) const
    {
        out<<"\n";
    }

 */


namespace graehl {


//template <class F>
struct gibbs_param
{
    void exclude_prior()
    {
        if (has_norm())
            sumcount.addbase(-prior);
    }
    void final_counts(double tmax1)
    {
        if (has_norm()) {
            sumcount.extend(tmax1);
            count()=sumcount.s;
        }
    }
    double save_count() const
    {
        return count();
    }
    void restore_count(double saved)
    {
        count()=saved;
    }

    template <class A>
    void init_cache(unsigned i,A &pcount,A &psum) const
    {
        if (has_norm())
            psum[norm]+=(pcount[i]=prior);
    }

    template <class A>
    double cache_prob(unsigned paramid,A &ccount,A &csum) const
    {
        return has_norm() ? ccount[paramid]++/csum[norm]++ : prior;
    }

    // same as proposal_prob but returns p=0 for 0 count instead of div by 0
    template <class A>
    double final_prob(A &normsum) const
    {
        if (has_norm()) {
            double c=count();
            return c>0?c/normsum[norm]:0;
        } else
            return prior;
    }

    template <class A>
    double proposal_prob(A &normsum) const
    {
        return has_norm() ? count()/normsum[norm] : prior;
    }

    double prior; //FIXME: not needed except to make computing --cache-prob easier.  also holds prob when NONORM
    //FIXME: alternative strategy: store locked arc prob in count(), and ensure that NONORM=-1 (reserved normsum id) has sum locked to 1.  may be slightly faster when computing probs for sampling
    double count() const
    {
        return sumcount.x;
    }
    double& count()
    {
        return sumcount.x;
    }
    bool has_norm() const // if NONORM, then prior = actual prob
    {
        return norm!=NONORM;
    }
//TODO: actually handle no-normgroup (locked) special value.  if you want to force p=1 just put it in a singleton normgroup
    static const unsigned NONORM=(unsigned)-1;
    unsigned norm; // index shared by all params to be normalized w/ this one
    delta_sum sumcount; //FIXME: move this to optional parallel array for many-parameter non-cumulative gibbs.
    template <class Normsum>
    void restore_p0(Normsum &ns) //ns must have been init to all 0s before the restore_p0 calls on each param
    {
        if (has_norm()) {
            ns[norm]+=prior;
            sumcount.clear(prior);
            assert(sumcount.tmax==0);
        }
    }
    template <class Normsum>
    void add_norm(Normsum &ns) const
    {
        if (has_norm())
            ns[norm]+=count();
    }
    template <class Normsums>
    void addc(double d,double t,Normsums &ns) // first call(s) should be w/ t=0
    {
        if (has_norm()) {
            ns[norm]+=d;
            sumcount.add_delta(d,t);
        }
    }
    gibbs_param() : prior(),norm(NONORM) {  }
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
    void init(unsigned n_sym_=1, unsigned n_blocks_=1)
    {
        n_sym=n_sym_;
        if (gopt.n_sym)
            n_sym=gopt.n_sym;
        n_blocks=n_blocks_;
        sample.reinit(n_blocks);
        gps.clear();
        nnorm=0;
    }

    gibbs_base(gibbs_opts const &gopt_
               , unsigned n_sym=1
               , unsigned n_blocks=1
               , std::ostream &out=std::cout
               , std::ostream &log=std::cerr)
        : gopt(gopt_)
        , out(out)
        , log(log)
        , nnorm(0)
    {
        gopt.validate();
        temp=gopt.temp;
        init(n_sym,n_blocks);
    }

    // need to call init before use
    gibbs_base(gibbs_opts const &gopt_
               ,std::ostream &out=std::cout
               ,std::ostream &log=std::cerr)
        : gopt(gopt_)
        , n_sym(1)
        , n_blocks(1)
        , out(out)
        , log(log)
        , nnorm(0)
        , sample(0)
    {
        gopt.validate();
        temp=gopt.temp;
    }

    gibbs_stats stats;
    gibbs_opts gopt;
    unsigned n_sym,n_blocks;
    std::ostream &out;
    std::ostream &log;
    typedef gibbs_param gp_t;
    typedef fixed_array<double> normsum_t;
    typedef normsum_t saved_counts_t;
    typedef dynamic_array<unsigned> block_t; // indexes into gps
    typedef fixed_array<block_t> blocks_t;
    typedef dynamic_array<gp_t> gps_t;
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

    // add params, then restore_p0() will be called on run().  first param assigned id=0.
    unsigned define_param(unsigned norm,double prior)
    {
        maybe_increase_max(nnorm,norm+1);
        unsigned r=gps.size();
        gps.push_back(norm,prior);
        return r;
    }
    double prior(double prob,double alpha,double normsz) const
    {
        return gopt.uniformp0?alpha:alpha*prob*normsz;
    }

    unsigned define_param(unsigned norm,double prob,double alpha,double normsz)
    {
        return define_param(norm,prior(prob,alpha,normsz));
    }
    unsigned define_param(double prob) //fixed prob
    {
        return define_param(gibbs_param::NONORM,prob);
    }

    void define_param_id(unsigned id,double prob) //fixed prob
    {
        define_param_id(id,gibbs_param::NONORM,prob);
    }

    void define_param_id(unsigned id,unsigned norm,double prior)
    {
        at_expand(gps,id)=gibbs_param(norm,prior);
        maybe_increase_max(nnorm,norm+1);
    }
    void define_param_id(unsigned id,unsigned norm,double prob,double alpha,double normsz)
    {
        define_param_id(id,norm,prior(prob,alpha,normsz));
    }

    void restore_p0()
    {
        normsum.reinit(nnorm);
        for (gps_t::iterator i=gps.begin(),e=gps.end();i!=e;++i)
            i->restore_p0(normsum);
    }

    // finalize avged counts over burned in iters; now proposal_prob = avged over all samples
    void finalize_cumulative_counts()
    {
        if (gopt.final_counts && !gopt.exclude_prior)
            return;
        double tmax1=final_t()+1; // if we don't add 1, then the last iteration's value isn't included in average
        if (gopt.exclude_prior)
            for (gps_t::iterator i=gps.begin(),e=gps.end();i!=e;++i)
                i->exclude_prior();
        if (!gopt.final_counts)
            for (gps_t::iterator i=gps.begin(),e=gps.end();i!=e;++i)
                i->final_counts(tmax1);
        compute_norms();
    }
    void compute_norms()
    {
        normsum.reinit_nodestroy(nnorm);
        for (gps_t::iterator i=gps.begin(),e=gps.end();i!=e;++i)
            i->add_norm(normsum);
    }

    // save/restore params //note: locked (NONORM) arcs wt in prior aren't touched
    void save_counts(saved_counts_t &c)
    {
        c.reinit_nodestroy(size());
        for (unsigned i=0,N=size();i<N;++i)
            c[i]=gps[i].save_count();
    }
    void restore_counts(saved_counts_t const& c)
    {
        for (unsigned i=0,N=size();i<N;++i)
            gps[i].restore_count(c[i]);
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
        for (unsigned i=0;i<N;++i)
            gps[i].init_cache(i,pcount,psum);
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
    double cache_prob(unsigned paramid)
    {
        return gps[paramid].cache_prob(paramid,ccount,csum);
    }
    Weight cache_prob(block_t const& p)
    {
        Weight prob=one_weight();
        for (block_t::const_iterator i=p.begin(),e=p.end();i!=e;++i)
            prob*=cache_prob(*i);
        assert(prob<=one_weight());
        return prob;
    }
    //proposal HMM within an iteration.  also avged prob once finalized
    Weight proposal_prob(block_t const& p)
    {
        Weight prob=one_weight();
        for (block_t::const_iterator i=p.begin(),e=p.end();i!=e;++i)
            prob*=proposal_prob(*i);
        assert(prob<=one_weight());
        return prob;
    }
    Weight operator()(unsigned i) const { return proposal_prob(i); }
    double final_prob(unsigned paramid) const
    {
        return final_prob(gps[paramid]);
    }
    double proposal_prob(unsigned paramid) const
    {
        return proposal_prob(gps[paramid]);
    }
    double final_prob(gibbs_param const& p) const // like proposal_prob but safe for hole parameters skipped when defining by id
    {
        return p.final_prob(normsum);
    }
    double proposal_prob(gibbs_param const&p) const
    {
        return p.proposal_prob(normsum);
    }
    void addc(gibbs_param &p,double delta)
    {
        assert(time>=p.sumcount.tmax);
        p.addc(delta,time,normsum); //t=0 until burnin done
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
        stats.clear(n_sym,n_blocks);
        Ni=gopt.iter;
        restore_p0(); // sets counts to prior, and normsums so prob is right
        imp.init_run(runi);
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
            if (time<0) time=0;
            iteration(imp,true);
        }
        log<<"\nGibbs stats: "<<stats<<"\n";
        return stats;
    }

    double block_weight(unsigned block)
    {
        return 1;
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
            if (gopt.tick_every)
                num_progress(log,b+1,gopt.tick_every,70,".",""); //FIXME: use proportional progress so total #blocks = 2 lines of status or so
            else
                num_progress_scale(log,b+1,n_blocks,70,2,".","\n ");
            block_t &block=sample[b];
            blockp=&block;
            double wt=imp.block_weight(b);
            if (subtract_old)
                addc(block,-wt);
            block.clear();
            imp.resample_block(b);
            p*=prob(block); // for gopt.cheap_prob, do this before adding probs back to get prob underestimate; do it after to get overestimate (cache model is immune because it tracks own history)
            addc(block,wt);
        }
        record_iteration(p);
        maybe_print_periodic(imp);
    }
    unsigned beststart;
 public:
    block_t *blockp; // redundant/courtesy: resample_block gets block index as argument already
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
            restore_probs(best_counts); //TESTME: used to erroneously be restore_counts! was this accidentally doing something good in start-selection?
        if (gopt.cache_prob) free_cache();
        return best;
    }

    //logging:
    Weight prob(block_t const& b)
    {
        return gopt.cache_prob?cache_prob(b):proposal_prob(b);
    }
    void log_ppx(Weight p)
    {
//        p.print_ppx(log,n_sym,n_blocks,"per-point-ppx","per-block-ppx","prob");
        stats.print_ppx(log,p);
    }

    void record_iteration(Weight p)
    {
        if (gopt.cache_prob) {
            log << " cache-model ";
            log_ppx(p);
        }
        if (gopt.cheap_prob) {
            log << " cheap-prob ";
            log_ppx(p);
        }
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
            itername(out<<"# ");
            out<<"t="<<time<<"\n";
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
            out <<"\n# group\t"<<name<<" i="<<iter<<" t="<<time<<"\n";
            print_range_i(out,normsum,from,to,true,true);
        }

    }
    void print_field(double d)
    {
        out<<'\t';
        print_width(out,d,gopt.width);
    }
    template <class G>
    void print_count(unsigned i,G &imp,bool final=false)
    {
        double ta=time+1;
        gibbs_param const& p=gps[i];
        delta_sum const& d=p.sumcount;
        double lastat=d.tmax;
        double avg=final?d.x/ta:d.avg(ta);  //d.x is an instantaneous (per-iter) count in non-final, but holds the extended .s sum after the final iteration and on restoring best start
        if (gopt.print_counts_sparse==0 || avg>=p.prior+gopt.print_counts_sparse) {
            out<<i<<'\t';
            if (p.has_norm())
                out<<p.norm;
            else
                out<<"LOCKED";
            if (final)
                print_field(avg);
            else
                print_field(d.x); // inst. count
            print_field(final_prob(i));
            if (!final) {
                print_field(avg);
                print_field(lastat);
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

    void print_counts_header(bool final,char const* name="")
    {
        if (!gopt.printing_counts()) return;
        out<<"\n#";
        out<<"id\t";
        out<<"group\tcount\tprob";
        double ta=time+1;
        if (!final)
            out<<"\tavg@"<<ta<<"\tlast@t\tprior";
        if (gopt.rich_counts)
            out<<"\tparam name";
        if (!final)
            out<<"\titer="<<iter;
        out<<"\t"<<name;
        out<<'\n';
    }

    // override order here
    template <class G>
    void print_counts_body(G &imp,bool final,unsigned from,unsigned to)
    {
        for (unsigned i=from;i<to;++i)
            print_count(i,imp,final);
    }

    template <class G>
    void print_counts(G &imp,bool final=false,char const* name="")
    {
        if (!gopt.printing_counts()) return;
        print_counts_header(final,name);
        unsigned to=std::min(gopt.print_counts_to,gps.size());
        if (gopt.norm_order)
            print_counts_body(imp,final,gopt.print_counts_from,to);
        else
            imp.print_counts_body(imp,final,gopt.print_counts_from,to);
        out<<"\n";
    }
    template <class G>
    void print_all(G &imp,bool final=true)
    {
        if (final)
            out<<"\n# final best gibbs run (start #"<<beststart<<" t="<<time<<"):\n";
        if (gopt.printing_sample())
            imp.print_sample(sample);
        print_norms();
        print_counts(imp,final);
    }

};


}

#endif
