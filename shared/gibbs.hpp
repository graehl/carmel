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
#include <graehl/shared/unimplemented.hpp>
#include <graehl/shared/debugprint.hpp>
#include <boost/math/distributions/normal.hpp>

//#define DEBUG_GIBBS

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

  typedef fixed_array<double> scale_t;
  typedef dynamic_array<unsigned> i_of_norm_t;
  template <class A>
  void scale_prior(i_of_norm_t const& normi,scale_t const& scale,A &normsum,bool invert=false)
  {
    if (has_norm()) {
      unsigned i=normi[norm];
      if (i>0) {
        double f=scale[i];
        if (invert) f=1./f;
        double s=f*prior,d=s-prior;
        sumcount.addbase(d);
        normsum[norm]+=d;
        prior=s;
      }
    }
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
    init_rscale();
    use_cache_prob=!gopt.expectation && (gopt.cache_prob||gopt.prior_inference_stddev>0);
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
  // this is the result of block resampling; a sparse parallel vector of paramid,optional wt
  struct block_delta
  {
    Weight prob;
    typedef unsigned id_t;
    typedef double wt_t;
    typedef dynamic_array<id_t> ids_t; // indexes into gps
    typedef dynamic_array<wt_t> wts_t; // if nonempty, applies a non-1 weight to each gibbs_param in block_t
    ids_t id;
    wts_t wt;

    // only used for --expectation (online-em)
    void randomize()
    {
      force_weights();
      for (wts_t::iterator i=wt.begin(),e=wt.end();i!=e;++i)
        *i *= random01();
    }

    void clear()
    {
      id.clear();
      wt.clear();
    }
    void swap(block_delta &o)
    {
      id.swap(o.id);
      wt.swap(o.wt);
    }

    unsigned size() const { return id.size(); }
    bool have_weights() const
    {
      if (wt.size()==id.size())
        return true;
      assert(wt.empty());
      return false;
    }
    void force_weights()
    {
      if (have_weights()) return;
      wt.reinit(size(),1);
    }

    void push_back(id_t i)
    {
      assert(!have_weights());
      id.push_back(i);
    }
    void push_back(id_t i,wt_t w)
    {
      assert(have_weights());
      id.push_back(i);
      wt.push_back(w);
    }
 private:
    //TODO: test all below before using
    typedef std::pair<id_t,wt_t> pair_t;
    typedef dynamic_array<pair_t > pairs_t;
    void to_pairs(pairs_t &c)
    {
      assert(have_weights());
      unsigned i=0,N=id.size();
      c.reserve(N);
      for (;i<N;++i)
        c.push_back(pair_t(id[i],wt[i]));
    }
    // consecutive (id,wt) (id,wt2) -> id,(wt+wt2)
    void combine_from_pairs(pairs_t const& c)
    {
      clear();
      unsigned N=c.size();
      if (N==0) return;
      id_t last=c[0].first;
      wt_t sum=c[0].second;
      for(unsigned i=1;i<N;++i) {
        pair_t const& p=c[i];
        id_t ci=p.first;
        if (last==ci)
          sum+=p.second;
        else {
          id.push_back(last);
          wt.push_back(sum);
          last=ci;
          sum=p.second;
        }
      }
      id.push_back(last);
      wt.push_back(sum);
    }
    struct order_first
    {
      bool operator()(pair_t const& a,pair_t const& b) const { return a.first<b.first; }
    };

    //TODO: test
    void compress()
    {
      if (have_weights()) {
        pairs_t p;
        to_pairs(p);
        std::sort(p.begin(),p.end(),order_first());
        combine_from_pairs(p);
      }
    }


  };

  typedef fixed_array<block_delta> blocks_t;
  typedef block_delta::ids_t block_t; // will have meaning (cache prob print sample etc) only for sampling, not forward/backward

  typedef dynamic_array<gp_t> gps_t;
  gps_t gps;
  unsigned nnorm;
  normsum_t normsum;
  blocks_t sample;
  double temperature;
  double power; // for deterministic annealing (temperature) = 1/temperature if temp positive.
  typedef gibbs_param::i_of_norm_t i_of_norm_t;
  typedef gibbs_param::scale_t scale_t;
  struct metanorm : public i_of_norm_t // metanorm[normgrp] = id where same id = scaled by same random factor (group 0 is never scaled but still occurs in scale_t array with ignored value)
  {
    dynamic_array<double> cumulative;

    void init_cumulative()
    {
      cumulative.reinit(nexti-1,1);
    }

    unsigned nexti;
    metanorm(unsigned nexti=1) : nexti(nexti) {  }
    void add(unsigned norm)
    {
      (*this)(norm)=nexti;
    }
    void add_fixed(unsigned norm)
    {
      (*this)(norm)=0;
    }
    void finish_scalegroup()
    {
      ++nexti;
    }

    void scale_priors(gps_t &gps,normsum_t &normsum,scale_t & s,bool invert=false)
    {
      for (unsigned i=1;i<nexti;++i) {
        double &r=cumulative[i-1];
        double k=s[i];
        if (invert)
          r/=k;
        else
          r*=k;
      }
      s[0]=1;
      for (gps_t::iterator i=gps.begin(),e=gps.end();i!=e;++i)
        i->scale_prior(*this,s,normsum,invert);
    }

    void set_fixed()
    {
      nexti=0;
      for (unsigned i=0,N=size();i!=N;++i)
        (*this)[i]=0;

    }

    // set_local and set_global only work after normgroups are all add()ed.  otherwise on lookup you'll have refs off the end of array
    void set_local()
    {
      nexti=size()+1;
      for (unsigned i=0,N=size();i!=N;++i)
        (*this)[i]=i+1;
    }
    void set_global()
    {
      nexti=2;
      for (unsigned i=0,N=size();i!=N;++i)
        (*this)[i]=1;
    }
  };
  metanorm prior_scale;

private:
  normsum_t ccount,csum,pcount,psum; // for computing true cache model probs
  unsigned iter,Ni; // i=0 is random init sample.  i=1...Ni are the (gibbs resampled) samples
  double time; // at i=gopt.burnin, t=0.  at tmax = nI-gopt.burnin, we have the final sample.
  gibbs_opts::temps temp;
  scale_t scales;
  typedef boost::math::normal_distribution<double> gaussian_t;
  gaussian_t rscale;
  double rquantfor0;
  double rquantremain;
  Weight rpdfmean;
  bool use_cache_prob;
  void init_rscale()
  {
    double sdev=gopt.prior_inference_stddev;
    if (sdev<=0) return;
    double mean=1;
    rscale=gaussian_t(mean,sdev);
    try {
      rquantfor0=boost::math::cdf(rscale,0);
    } catch(std::domain_error &e) {
      rquantfor0=0;
    }
    rquantremain=1-rquantfor0;
//        rpdfmean=boost::math::pdf(rscale,mean);
  }
  double random_scale_ratio() const  // greater than 0
  {
    double r=boost::math::quantile(rscale,rquantfor0+random01()*rquantremain);
    assert(r>0);
    return r;
  }

  // return M-H assymetric term Q(scale from old)/Q(old from scale)
  Weight choose_prior_scales()
  {
    double sdev=gopt.prior_inference_stddev;
    assert(sdev>0);
    gaussian_t rscale(1,sdev);
    unsigned N=prior_scale.nexti;
    Weight q2_1=1,q1_2=1;
    scales.reinit(N);
    for (unsigned i=1;i<N;++i) {
      scales[i]=random_scale_ratio();
      q2_1*=boost::math::pdf(rscale,scales[i]); // p new | old
      q1_2*=boost::math::pdf(rscale,1/scales[i]); // p old | new
    }
    return q1_2/q2_1;
  }

  void scale_priors(bool inverse)
  {
    prior_scale.scale_priors(gps,normsum,scales,inverse);
  }

public:

  void propose_new_priors()
  {
    if (gopt.prior_inference_stddev<=0) return;
    if (gopt.expectation)
      unimplemented("prior inference not yet supported for expectation - i.e. it only works with cache prob and blocked gibbs sampling.  in principle using likelihood instead of cache prob should be possible.");
    Weight a2=choose_prior_scales();
    normsum_t pcount1(pcount),psum1(psum);
    //todo: don't modify state for current priors in checking quality of proposed new priors (cache_prob should take argument for which set of arrays to use)
    Weight p1=cache_prob(false); // //todo: use already computed cache prob // false should agree w/ true but isn't.  why?
    scale_priors(false);
    Weight p2=cache_prob(true);
    Weight a1=p2/p1; // current sample is this many times more likely under the new prior pseudocounts
    Weight a=a1*a2; // correct for asymm. prior scale proposal dist. (accept more often if it's easier to get back to old)
    bool accept=random01()<a.getReal();
    log<<(accept?"accepted":"rejected")<<" new priors ";
    if (gopt.prior_inference_show) {
      log<<prior_scale.cumulative<<" "; //TODO: show avg per-parameter prior, not scale factor?
    }
//        DGIBBS(log<<"scaled by "<<scales<<" ");
    log<<"with ";
//DGIBBS()
    log<<"p1="<<p1.as_base(2)<<" p2="<<p2.as_base(2);
    log<<" a1=p2/p1="<<a1.getReal()<<" a2=q(1|2)/q(2|1)="<<a2.getReal()<<" p_accept="<<a.getReal()<<". ";
    if (!accept) {
      pcount1.swap(pcount);
      psum1.swap(psum);
      scale_priors(true); // undo new prior
    }
  }

  bool burning() const
  {
    return iter>=gopt.burnin;
  }
  bool inferring() const
  {
    unsigned start=gopt.prior_inference_start ? gopt.prior_inference_start : gopt.burnin;
    return gopt.prior_inference_stddev>0 && start<=iter && (!gopt.prior_inference_end || iter<gopt.prior_inference_end);
  }

  double final_t() const
  {
    return Ni-gopt.burnin;
  }
  unsigned size() const { return gps.size(); }

  // called after parameters all defined.
  void finish_params()
  {
    prior_scale.resize(nnorm);
    if (gopt.prior_inference_global)
      prior_scale.set_global();
    if (gopt.prior_inference_local)
      prior_scale.set_local();
  }

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
    normsum.reinit_nodestroy(nnorm);
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

  void save_priors(saved_counts_t &c)
  {
    c.reinit_nodestroy(size());
    for (unsigned i=0,N=size();i<N;++i)
      c[i]=gps[i].prior;
  }

  void restore_priors(saved_counts_t const& c)
  {
    for (unsigned i=0,N=size();i<N;++i)
      gps[i].prior=c[i];
    recompute_cache_priors();
    prior_scale.init_cumulative();
  }

  // cache probability changes each time a param is used (it becomes more likely in the future)
  void init_cache()
  {
    if (!use_cache_prob) return;
    unsigned N=gps.size();
    pcount.init(N);
    ccount.init(N);
    csum.init(nnorm);
    psum.init(nnorm);
    recompute_cache_priors();
    reset_cache();
  }
  void recompute_cache_priors() //todo: recompute into ccount/csum instead for checking new priors?
  {
    if (!use_cache_prob) return;
    for (unsigned i=0;i<nnorm;++i)
      psum[i]=0;
    for (unsigned i=0,N=gps.size();i<N;++i)
      gps[i].init_cache(i,pcount,psum);
  }

  void reset_cache()
  {
    if (!use_cache_prob) return;
    ccount.uninit_copy_from(pcount);
    csum.uninit_copy_from(psum);
  }
  void free_cache()
  {
    if (!use_cache_prob) return;
    ccount.dealloc();
    csum.dealloc();
  }
  Weight cache_prob(bool recompute)
  {
    if (recompute) // if priors have changed
      recompute_cache_priors();
    reset_cache(); // todo: copy from proposed changed priors to ccount directly, then to pcount if accept
    Weight w=1;
    for (unsigned i=0;i<n_blocks;++i)
      w*=cache_prob(sample[i]);
    return w;
  }

  double cache_prob(unsigned paramid)
  {
    return gps[paramid].cache_prob(paramid,ccount,csum);
  }
  Weight cache_prob(block_delta const& p)
  {
    if (gopt.expectation)
      return p.prob;
    else
      return cache_prob(p.id);
  }

  Weight cache_prob(block_t const& p)
  {
    Weight prob=1;
    for (block_t::const_iterator i=p.begin(),e=p.end();i!=e;++i)
      prob*=cache_prob(*i);
    assert(prob<=1);
    return prob;
  }
  //proposal HMM within an iteration.  also avged prob once finalized
  Weight proposal_prob(block_t const& p)
  {
    Weight prob=1;
    for (block_t::const_iterator i=p.begin(),e=p.end();i!=e;++i)
      prob*=proposal_prob(*i);
    assert(prob<=1);
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
  void addc(gibbs_param &p,double scale)
  {
    assert(time>=p.sumcount.tmax);
    p.addc(scale,time,normsum); //t=0 until burnin done
  }
  void addc(unsigned param,double scale)
  {
    addc(gps[param],scale);
  }
  void addc(block_t &b,double scale)
  {
    assert(time>=0);
    for (block_t::const_iterator i=b.begin(),e=b.end();i!=e;++i)
      addc(*i,scale);
  }
  void addc(block_delta &bd,double scale)
  {
    block_t & b=bd.id;
    if (gopt.expectation)
      for (unsigned i=0,N=b.size();i<N;++i)
        addc(b[i],bd.wt[i]*scale);
    else
      addc(b,scale);
  }

  void clear_blocks()
  {
    for (unsigned i=0;i<n_blocks;++i)
      sample[i].clear();
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
    clear_blocks();
    iteration(imp,gopt.random_start||(runi&&gopt.expectation)); // initial sample; randomize deltas when doing expectation to prevent deterministic hillclimb
    //FIXME: isn't really random!  get the same sample after every iteration
    for (iter=1;iter<=Ni;++iter) {
      time=(double)iter-(double)gopt.burnin; //very funny: unsigned arithmetic -> double (unsigned maximum) if you're sloppy
      if (time<0) time=0;
      iteration(imp,false);
    }
    log<<"\nGibbs stats: "<<stats<<"\n";
    if (gopt.prior_inference_show)
      log<<"Final prior-scale="<<prior_scale.cumulative<<"\n";

    return stats;
  }

  double block_weight(unsigned block)
  {
    return 1;
  }

  template <class G>
  void iteration(G &imp,bool randomize)
  {
    temperature=temp(iter);
    power=(temperature>0)?1./temperature:1;
    itername(log);
    if (use_cache_prob) reset_cache();
    Weight p=1;
    imp.init_iteration(iter);
    for (unsigned b=0;b<n_blocks;++b) {
      if (gopt.tick_every)
        num_progress(log,b+1,gopt.tick_every,70,".",""); //FIXME: use proportional progress so total #blocks = 2 lines of status or so
      else
        num_progress_scale(log,b+1,n_blocks,70,2,".","\n ");
      block_delta &block=sample[b];
      double wt=imp.block_weight(b);
      if (!gopt.include_self)
        addc(block,-wt);
      block_delta include_self_save;
      if (gopt.include_self)
        include_self_save.swap(block);
      else
        block.clear();
      imp.resample_block(b);
      block_delta &bd=sample[b];
      if (gopt.expectation) {
        if (randomize) {
          bd.randomize();
          bd.prob=0; // TODO: get prob after randomizing
        }
      } else {
        bd.prob=prob(block.id); // for gopt.cheap_prob, do this before adding probs back to get prob underestimate; do it after to get overestimate (cache model is immune because it tracks own history)
      }
      p*=bd.prob;
      if (gopt.include_self)
        addc(include_self_save,-wt);
      addc(block,wt); //todo: can efficiently compute cache prob as we do this
    }
    if (iter>0 && inferring())
      propose_new_priors();
    record_iteration(p);
    maybe_print_periodic(imp);
  }
  unsigned beststart;
public:
  template <class G>
  gibbs_stats run_starts(G &imp)
  {
    init_cache();
    saved_counts_t priors;
    saved_counts_t best_counts;
    blocks_t best_sample(n_blocks);
    gibbs_stats best;
    unsigned re=gopt.restarts;
    bool restart_priors=re>0 && gopt.prior_inference_restart_fresh;
    prior_scale.init_cumulative();
    if (restart_priors)
      save_priors(priors);
    for (unsigned r=0;r<=re;++r) {
      graehl::time_space_report(log,"Gibbs sampling run: ");
      if (re>0) log<<"(random restart "<<r<<" of "<<re<<"): ";
      if (r>0&&restart_priors)
        restore_priors(priors);
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
    free_cache();
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

  void record_iteration(Weight p,bool random_scale_expectation=false)
  {
    char const* probname=0;
    if (gopt.expectation)
      probname="sum-all-derivations";
    else if (gopt.cache_prob)
      probname="cache-model";
    else if (gopt.cheap_prob)
      probname="cheap(proposal)";
    if (probname) {
      log << ' '<<probname<<' ';
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
  void print_count(unsigned i,G &imp,bool final=false,bool show_metanorm=true)
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
        if (show_metanorm) {
          unsigned metanorm=p.has_norm()?prior_scale[p.norm]:0;
          out<<'\t';
          if (metanorm>0)
            out<<metanorm;
          else
            out<<"FIXED";
        }
      }
      if (gopt.rich_counts) {
        out<<'\t';
        imp.print_param(out,i);
      }
//                    out<<'\t'<<d.s;
      out<<'\n';
    }
  }

  void print_counts_header(bool final,char const* name="",bool show_metanorm=true)
  {
    if (!gopt.printing_counts()) return;
    out<<"\n#";
    out<<"id\t";
    out<<"group\tcount\tprob";
    double ta=time+1;
    if (!final) {
      out<<"\tavg@"<<ta<<"\tlast@t\tprior";
      if (show_metanorm)
        out<<"\tgroupby";
    }
    if (gopt.rich_counts)
      out<<"\tparam name";
    if (!final)
      out<<"\titer="<<iter;
    out<<"\t"<<name;
    out<<'\n';
  }

  // override order here
  template <class G>
  void print_counts_body(G &imp,bool final,unsigned from,unsigned to,bool show_metanorm=true)
  {
    for (unsigned i=from;i<to;++i)
      print_count(i,imp,final,show_metanorm);
  }

  template <class G>
  void print_counts(G &imp,bool final=false,char const* name="",bool show_metanorm=true)
  {
    if (!gopt.printing_counts()) return;
    print_counts_header(final,name,show_metanorm);
    unsigned to=std::min(gopt.print_counts_to,gps.size());
    if (gopt.norm_order)
      print_counts_body(imp,final,gopt.print_counts_from,to,show_metanorm);
    else
      imp.print_counts_body(imp,final,gopt.print_counts_from,to,show_metanorm);
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
