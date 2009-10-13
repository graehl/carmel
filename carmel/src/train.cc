#include <boost/utility.hpp>
#include <graehl/shared/config.h>
#include <graehl/carmel/src/train.h>
#include <graehl/carmel/src/fst.h>
#include <graehl/shared/weight.h>
#include <graehl/carmel/src/derivations.h>
#include <graehl/carmel/src/cascade.h>
#include <graehl/shared/serialize_batch.hpp>
#include <graehl/shared/time_space_report.hpp>

#define DEBUG_GIBBS
//#define DEBUGTRAIN

namespace graehl {

derivations::statistics derivations::global_stats;

void check_fb_agree(Weight fin,Weight fin2)
{
#ifdef DEBUGTRAIN
    Config::debug()<<"Forward prob = " << fin << std::endl;
    Config::debug()<<"Backward prob = " << fin2 << std::endl;
#endif
#ifdef ALLOWED_FORWARD_OVER_BACKWARD_EPSILON
    double ratio = (fin > fin2 ? fin/fin2 : fin2/fin).getReal();
    double e = ratio - 1;
    if ( e > ALLOWED_FORWARD_OVER_BACKWARD_EPSILON )
        Config::warn() << "Warning: forward prob vs backward prob relative difference of " << e << " exceeded " << ALLOWED_FORWARD_OVER_BACKWARD_EPSILON << " (with infinite precision, it should be 0).\n";
#endif
}

struct WeightAccum {
    Weight sum;
    int n_nonzero;
    int n;
    void operator ()(Weight const&w) {
        w.NaNCheck();
        sum += w;
        ++n;
        if (w.isPositive())
            ++n_nonzero;
    }
    WeightAccum() {
        reset();
    }
    void reset() {
        n=n_nonzero=0;
        sum.setZero();
    }
    friend std::ostream & operator <<(std::ostream &o,WeightAccum const& a)
    {
        return o << "("<<a.sum<<","<<a.n<<","<<a.n_nonzero<<")";
    }
};

template <class a>
void print_stats(arcs_table<a> const& t,char const* header)
{

    Config::debug() << header;
    WeightAccum a_w;
    WeightAccum a_c;
    for (typename arcs_table<a>::const_iterator i=t.begin(),e=t.end();i!=e;++i) {
        a_w(i->weight());
        a_c(i->counts);
    }
    Config::debug() << "(sum,n,nonzero): weights="<<a_w<<" counts="<<a_c<<"\n";
}

struct matrix_io_index : boost::noncopyable
{
    typedef dynamic_array<DWPair> for_io;
    typedef HashTable<IOPair,for_io> for_state;
    typedef fixed_array<for_state> states_t;

    arcs_table<arc_counts> &t;
    states_t forward,backward;

    matrix_io_index(arcs_table<arc_counts> &t) : t(t), forward(t.n_states), backward(t.n_states)
    {
    }

    void populate(bool include_backward)
    {
        for (unsigned i=0,N=t.size();i!=N;++i) {
            arc_counts const& ac=t[i];
            IOPair io(ac.in(),ac.out());
            unsigned s=ac.src,d=ac.dest();
            forward[s][io].push_back(DWPair(d,i));
            if (include_backward)
                backward[d][io].push_back(DWPair(s,i));
        }
    }
};


// similar to transposition but not quite: instead of replacing w.ij with w.ji, replace w.ij with w.(I-i)(J-j) ... matrix has the same dimensions.  it's a 180 degree rotation, not a reflection about the identity line
void matrix_reverse_io(Weight ***w,int max_i, int max_o) {
    int i;
    for ( i = 0 ; i <= max_i/2 ; ++i ) {
        Weight **temp = w[i];
        w[i] = w[max_i - i];
        w[max_i - i] = temp;
    }
    for ( i = 0 ; i <= max_i ; ++i )
        for ( int o = 0 ; o <= max_o/2 ; ++o ) {
            Weight *temp = w[i][o];
            w[i][o] = w[i][max_o - o];
            w[i][max_o - o] = temp;
        }
}

#ifdef DEBUG_GIBBS
#define DGIBBS(a) a;
#else
#define DGIBBS(a)
#endif
#define OUTGIBBS(a) DGIBBS(std::cerr<<a)

template <class arc_counts>
struct cached_derivs
{
    WFST &x;
    serialize_batch<derivations> derivs;
    typedef arcs_table<arc_counts> arcs_t;
    arcs_t arcs;
    unsigned size()
    {
        return derivs.size();
    }
    cached_derivs(WFST &x,training_corpus &corpus,WFST::deriv_cache_opts const& copt)
        : x(x),derivs(copt.use_disk(),copt.disk_cache_filename,true,copt.disk_cache_bufsize),arcs(x)
    {
        if (copt.cache()) {
            graehl::time_space_report(Config::log(),"Computed cached derivations:");
            compute_derivations(corpus.examples,copt.cache_backward());
        }
    }
    template <class Examples>
    void compute_derivations(Examples const &ex,bool cache_backward)
    {
        wfst_io_index io(x);
        unsigned n=1;
        derivs.clear();
        for (typename Examples::const_iterator i=ex.begin(),end=ex.end();
             i!=end ; ++i,++n) {
            derivations &d=derivs.start_new();
            if (!d.init_and_compute(x,io,arcs,i->i,i->o,i->weight,n,cache_backward)) {
                warn_no_derivations(x,*i,n);
                derivs.drop_new();
            } else {
#ifdef DEBUGDERIVATIONS
                Config::debug() << "Derivations in transducer for input/output #"<<n<<" (final="<<d.final()<<"):\n";
                i->print(Config::debug(),x,"\n");
                printGraph(d.graph(),Config::debug());
#endif
                derivs.keep_new();
            }
        }
        derivs.mark_end();
        Config::log() << derivations::global_stats;
    }
    template <class F>
    void foreach_deriv(F &f)
    {
        unsigned n=0;
        for (derivs.rewind();derivs.advance();) {
            ++n;
            training_progress(n);
            f(n,derivs.current());
        }

    }
};

struct gibbs
{
    WFST &composed;
    cascade_parameters &cascade;
    training_corpus &corpus;
    WFST::NormalizeMethods const& methods;
    WFST::train_opts topt;
    WFST::gibbs_opts gopt;
    typedef WFST::gibbs_params gps_t;
    gps_t gps;
    typedef fixed_array<double> normsum_t;
    unsigned nnorm;
    normsum_t normsum;
    struct init_trivial  // hack to make single transducer cascade work; stuff done before derivs are built
    {
        init_trivial(WFST &composed,cascade_parameters &cascade)
        {
        }
    };
    init_trivial init;
    cached_derivs<gibbs_counts> derivs;
    typedef cascade_parameters::chain_t param_list;
    typedef FSTArc *carc_t;
    typedef dynamic_array<param_list> acpath;
    typedef fixed_array<acpath> sample_t;
    sample_t sample;
    carc_t carc(GraphArc const& a) const
    {
        return derivs.arcs.ac(a).arc;
    }
    param_list ac(GraphArc const& a) const
    {
        return cascade[carc(a)];
    }
    void addc(acpath const& p,double delta)
    {
        for (acpath::const_iterator i=p.begin(),e=p.end();i!=e;++i)
            addc(*i,delta);
    }
    void addc(param_list p,double delta)
    {
        for (;p;p=p->next)
            addc(param(p->data),delta);
    }
    void addc(gibbs_param &p,double delta)
    {
        p.addsum(delta,t,normsum,accum_delta);
    }
    Weight cache_prob(acpath const& p)
    {
        Weight prob=one_weight();
        for (acpath::const_iterator i=p.begin(),e=p.end();i!=e;++i)
            prob*=cache_prob(*i);
        return prob;
    }
    Weight cache_prob(param_list p)
    {
        Weight prob=one_weight();
        for (;p;p=p->next) {
            FSTArc *a=p->data;
            prob*=cache_prob(a->groupId);
        }
        return prob;
    }
    double cache_prob(unsigned paramid)
    {
        unsigned norm=gps[paramid].norm;
        return ccount[paramid]++/csum[norm]++;
    }
    gibbs_param &param(FSTArc const& f) const
    { return gps[f.groupId]; }
    gibbs_param &param(FSTArc const* pf) const
    { return param(*pf); }
    double p_param(gibbs_param const&p) const
    {
        return p.count/normsum[p.norm];
    }
    std::ostream &itername(std::ostream &o,char const* suffix=" ") const
    {
        o<<"Gibbs i="<<i;
        if (!temp.is_constant())
            o<<" temperature="<<1./power<<" power="<<power<<suffix;
        return o;
    }
    void iteration()
    {
        std::ostream &log=Config::log();
        itername(log);
        prob.setOne();
        if (gopt.cache_prob) {
            cprob.setOne();
            unsigned N=gps.size();
            ccount.init(N);
            csum.init(nnorm);
            for (unsigned i=0;i<N;++i) {
                gibbs_param const& gp=gps[i];
                csum[gp.norm]+=(ccount[i]=gp.prior);
            }
        }
        derivs.foreach_deriv(*this);
        if (gopt.cache_prob) {
            ccount.dealloc();
            csum.dealloc();
            log<<" cache-model ";
            cprob.print_ppx(log,corpus.n_output,corpus.n_pairs);
        }
        if (gopt.ppx) {
            log<<" sample ";
            prob.print_ppx(log,corpus.n_output,corpus.n_pairs);
        }
        log<<'\n';
        maybe_print_paths_periodic();
#ifdef DEBUG_GIBBS
//        print_sample(0,cascade.size());
//        save_weights(false); // unnormalized total counts
//        std::cerr<<'\n'<<*cascade.cascade[0]<<'\n';
#endif
    }
    struct p_init
    {
        gibbs const&c;
        p_init(gibbs const&c) :c(c) {  }
        double operator()(GraphArc const& a) const
        {
            return a.weight;
        }
        param_list ac(GraphArc const& a) const
        {
            return c.ac(a);
        }
    };    //cached_derivs.foreach_deriv
    void operator()(unsigned n_1based,derivations &d)
    {
        training_progress(n_1based);
        acpath &p=sample[n_1based-1];
        if (subtract_old)
            addc(p,-1);
//        OUTGIBBS(','<<n_1based<<':')
        if (init_prob) {
            d.random_path(p,p_init(*this),power);
        } else {
            d.random_path(p,*this,power);
        }
        if (gopt.cache_prob)
            cprob*=cache_prob(p);
        if (gopt.ppx)
            prob*=p_path(p);
//        OUTGIBBS(p.size());
        addc(p,1);
    }
    // for derivations::random_path; compute global backward probs on Weight array
    double operator()(GraphArc const& a) const
    {
        return p_param_list(ac(a));
    }
    double p_param_list(param_list p) const
    {
        double w=1.;
        for (;p;p=p->next)
            w*=p_param(param(p->data));
        return w;
    }
    Weight p_path(acpath const& p) const
    {
        Weight w=one_weight();
        for (acpath::const_iterator i=p.begin(),e=p.end();i!=e;++i)
            w*=p_param_list(*i);
        return w;
    }
    //TODO: allow locked arcs?  see fst.h *_gibbs derivations global_normalize
    gibbs(WFST &composed,cascade_parameters &cascade, training_corpus &corpus
          ,WFST::NormalizeMethods & methods
          ,WFST::train_opts const& topt
          ,WFST::gibbs_opts const& gopt
          ,WFST::saved_weights_t *saved=0
        ) :
        composed(composed), cascade(cascade), corpus(corpus), methods(methods), topt(topt), gopt(gopt), init(composed,cascade), derivs(composed,corpus,topt.cache), sample(derivs.size()), temp(gopt.temperature(topt.max_iter))
    {
        if (saved)
            cascade.restore_weights(composed,*saved); // but derivs got a chance to grab em weights for init random sample!
        nnorm=cascade.set_gibbs_params(composed,methods,gps,gopt.p0init); // #1 - overwrite chains' (input xdcr arcs) group
//        if (cascade.trivial) OUTGIBBS(composed); // #1 verified
        normsum.init(nnorm);
        cascade.set_composed(composed);
        cascade.set_trivial_gibbs_chains(); // #2 - build the singleton chains based on groups from #1
//        DGIBBS(cascade.print_chains(std::cerr,false)); // #2 verified
    }
    void run(bool use_init_prob=false)
    {
        Ni=topt.max_iter;
        burnt0=gopt.cumulative_counts ? gopt.sched.burnin : Ni;
        addnorms(); // for base model
        power=1.;
        accum_delta=false;
        i=0;
        if (gopt.print_every!=0) print_counts("prior counts");
        init_prob=use_init_prob;
        subtract_old=false;
        iteration(); // random initial sample
        init_prob=false;
        subtract_old=true;
        for (i=1;i<=Ni;++i) {
            power=1./temp(i);
            t=i-burnt0;
            if (t==0) {
                for (gps_t::iterator i=gps.begin(),e=gps.end();i!=e;++i)
                    i->initsum();
                accum_delta=true;
            } else if (t<0) t==0;
            iteration();
        }
    }
    // not idempotent until final iteration, because param.sumcount.x is obliterated
    void normalize_accum_delta()
    {
        normsum_t sn(nnorm);
        double tmax=Ni;
        for (gps_t::iterator i=gps.begin(),e=gps.end();i!=e;++i)
            sn[i->norm]+=i->sumcount.extend(tmax);
        for (gps_t::iterator i=gps.begin(),e=gps.end();i!=e;++i) {
            gibbs_param &p=*i;
            p.sumcount.x=p.sumcount.s/sn[p.norm];
        }
    }
    void save_weights(bool accum=true)
    {
        if (accum&&accum_delta)
            normalize_accum_delta();
        cascade.update_gibbs(*this);
    }
    // for cascade.update_gibbs
    void operator()(unsigned src,FSTArc & f) const
    {
        gibbs_param &p=param(f);
        f.weight=accum_delta ? p.sumcount.x : p_param(p);
    }
    typedef dynamic_array<FSTArc*> cascade_path;
    typedef fixed_array<cascade_path> cascade_paths;
    typedef cascade_parameters::cascade_t casc;
    // single path in composition -> separate paths for each in cascade[i], for i in [a,b) put result in r[i]
    void paths(acpath const& p,unsigned a,unsigned b,cascade_paths &r)
    {
        assert(b<=r.size());
        for (unsigned i=a;i<b;++i)
            r[i].clear();
        for (acpath::const_iterator i=p.begin(),e=p.end();i!=e;++i) {
//            OUTGIBBS(*i); //FIXME: trivial => all the same but first (!)
            for (param_list p=*i;p;p=p->next) {
                FSTArc *a=p->data;
                unsigned ci=param(a).cascadei;
                assert(ci<r.size());
                r[ci].push_back(a);
            }
        }
    }
    void print_sample(unsigned a,unsigned b,casc const& c)
    {
        if (!(b>a && a<c.size())) {
            if (b!=0)
                Config::warn() << "--print-from,-to gibbs ["<<a<<","<<b<<") is out of range for "<<c.size()<<" input transducers.\n";
            return;
        }
        if (b>c.size()) b=c.size();
        if (a==b) return;
        for (sample_t::const_iterator i=sample.begin(),e=sample.end();i!=e;++i)
            print_path(*i,a,b,c);
    }
    void print_path(acpath const& p,unsigned a,unsigned b,casc const &c)
    {
        cascade_paths ps(c.size());
        paths(p,a,b,ps);
        for (unsigned i=a;i<b;++i) {
            gopt.printer(*c[i],ps[i]);
        }
        gopt.printer.out()<<'\n';
    }
    void print_sample(unsigned print_from,unsigned print_to)
    {
        print_sample(print_from,print_to,cascade.cascade);
    }
    void print_sample()
    {
        print_sample(gopt.print_from,gopt.print_to);
    }
    static inline bool divides(unsigned div,unsigned i)
    {
        return div!=0 && i%div==0;
    }
    void maybe_print_paths_periodic()
    {
        if (divides(gopt.print_every,i))
            print_paths_periodic();
    }
    std::ostream &out()
    {
        return gopt.printer.out();
    }
    void print_counts(char const* name="counts")
    {
        unsigned from=gopt.print_counts_from;
        unsigned to=std::min(gopt.print_counts_to,gps.size());
        if (to>from && from<gps.size()) {
            out()<<"# count\tnormgrp\tinput#\tcumulative\t"<<name<<" i="<<i<<"\n";
            print_range(out(),gps.begin()+from,gps.begin()+to,true,true);
        }
    }
    void print_paths_periodic()
    {
        itername(gopt.printer.out()<<"# ","\n");
        if (gopt.printer.needs_weight())
            save_weights(false);
        print_sample();
        gopt.printer.out()<<"#\n";
        print_counts();
    }

 private:
    bool init_prob; // initial sample uses composed weights, not necessarily p0
    normsum_t ccount,csum; // for computing true cache model probs
    Weight cprob;
    Weight prob;
    unsigned i,Ni;
    double power;
    double t;
    double burnt0;
    WFST::gibbs_opts::temps temp;
    bool subtract_old;
    bool accum_delta;

    void addnorms()
    {
        for (gps_t::const_iterator i=gps.begin(),e=gps.end();i!=e;++i) {
            gibbs_param const& p=*i;
            normsum[p.norm]+=p.count;
        }
    }
};


void WFST::train_gibbs(cascade_parameters &cascade, training_corpus &corpus, NormalizeMethods & methods, train_opts const& topt
                       , gibbs_opts const& gopt, double min_prior)
{
    std::ostream &log=Config::log();
    for (NormalizeMethods::iterator i=methods.begin(),e=methods.end();i!=e;++i) {
        if (i->add_count<=0) {
            Config::warn() << "Gibbs sampling requires positive --priors for base model / initial sample.  Setting to "<<min_prior<<"\n";
            i->add_count=min_prior;
        }
    }

    bool em=gopt.init_em>0;
    bool restore=em && !gopt.em_p0;
    saved_weights_t saved;
    if (restore)
        cascade.save_weights(*this,saved);
    if (em) {
        NormalizeMethods m2=methods;
        for (NormalizeMethods::iterator i=m2.begin(),e=m2.end();i!=e;++i)
            i->add_count=0;
        train_opts t2=topt;
        unsigned N=t2.max_iter=gopt.init_em;

        // EM:
#if 0
        forward_backward fb(*this,cascade,false,0,true,t2,corpus);
        Weight p,cp;
        for (unsigned i=0;i<N;++i) {
            time_report taken(Config::log(),"Time for iteration: ");
            cascade.update(*this);
            p=fb.estimate(cp);
            Weight maxdarc=fb.maximize(m2);
            log<<"i="<<i<<" init-em ";
            p.print_ppx(log,corpus.n_output,corpus.n_pairs);
            log<<'\n';
        }
        cascade.use_counts_final(*this,m2);
#else
        train(cascade,corpus,m2,false,0,0,1,t2,true);
#endif
    }
    //restore old weights; can't do gibbs init before em because groupId gets overwritten; em needs that id from composition
    gibbs g(*this,cascade,corpus,methods,topt,gopt,restore?&saved:0);
    saved.clear();
    g.run(em);
    g.save_weights();
    g.print_sample();
    cascade.clear_groups();
    cascade.update(*this);
}


namespace for_arcs {

// for cascade, use before update->estimate so cascade can recover counts later (estimate doesn't use em_weight)
struct save_counts
{
    void operator()(arc_counts &a) const
    {
        a.em_weight = a.weight();
    }
};

// use after estimate if you got a new global best
struct save_best_counts
{
    void operator()(arc_counts &a) const
    {
        a.best_weight=a.em_weight;
    }
};


// scratch gets previous weight
struct prep_new_weights
{
    Weight scale_prior;
    prep_new_weights(Weight scale_prior) : scale_prior(scale_prior) {}
    void operator()(arc_counts &a) const
    {
        if ( !WFST::isLocked((a.arc)->groupId) ) { // if the group is tied, then the group number is zero, then the old weight does not change. Otherwise update as follows
            //note: it's not possible for a cascade composed arc to have a locked groupid, so don't bother not testing
            a.scratch = a.weight();   // old weight - Yaser: this is needed only to calculate change in weight later on ..
            //FIXME: in cascade, do we want a change per component transducer weight change convergence criteria?
            a.weight() = a.counts + a.prior_counts*scale_prior; // new (unnormalized weight)

            NANCHECK(a.counts);
            NANCHECK(a.prior_counts);
            NANCHECK(a.weight());
            NANCHECK(a.scratch);
        }
    }
};

// overrelax weight() and store raw EM weight in em_weight.  after pre_norm_counts, scratch has old .  also after WFST::normalize.  POST: need normalization again.
struct overrelax
{
    FLOAT_TYPE delta_scale;
    overrelax(FLOAT_TYPE delta_scale) : delta_scale(delta_scale) {}
    void operator()(arc_counts &a) const
    {
        a.em_weight = a.weight();
        NANCHECK(a.em_weight);
        if (delta_scale > 1.)
            if ( !WFST::isLocked(a.groupId()) )
                if ( a.scratch.isPositive() ) {
                    a.weight() = a.scratch * ((a.em_weight / a.scratch).pow(delta_scale));
                    NANCHECK(a.scratch);
                    NANCHECK(a.weight());
                }
    }
};

struct max_change
{
    Weight maxChange;
    void operator()(arc_counts &a)
    {
        if (!WFST::isLocked(a.groupId())) {
            Weight change = absdiff(a.weight(),a.scratch);
            if ( change > maxChange )
                maxChange = change;
        }
    }
    Weight get() const
    {
        return maxChange;
    }
};

struct save_best
{
    void operator()(arc_counts &a) const
    {
        a.best_weight=a.weight();
    }
};

struct swap_em_scaled
{
    void operator()(arc_counts &a) const
    {
        std::swap(a.em_weight,a.weight());
    }
};

struct keep_em_weight
{
    void operator()(arc_counts &a) const
    {
        a.weight()=a.em_weight;
    }
};

struct use_best_weight
{
    void operator()(arc_counts &a) const
    {
        a.weight()=a.best_weight;
    }
};


struct clear_count
{
    void operator()(arc_counts &a) const
    {
        a.counts.setZero();
    }
};

struct clear_scratch
{
    void operator()(arc_counts &a) const
    {
        a.scratch.setZero();
    }
};

struct add_weighted_scratch
{
    Weight w;
    add_weighted_scratch(Weight w):w(w){}
    void operator()(arc_counts &a) const
    {
        if (!a.scratch.isZero())
            a.counts += w*a.scratch;
#ifdef DEBUG
        NANCHECK(a.counts);
        NANCHECK(a.scratch);
#endif
    }
};

}//ns



struct forward_backward : public cached_derivs<arc_counts>
{
    typedef cached_derivs<arc_counts> cache_t;
    cascade_parameters &cascade;
    unsigned n_in,n_out,n_st;
    typedef arcs_table<arc_counts> arcs_t;
    training_corpus *trn;

    arcs_t arcs;

    /// stuff for old matrix (non derivation-structure-caching) based forward/backward
    bool use_matrix;
    bool remove_bad_training;
    matrix_io_index mio;
    Weight ***f,***b;
    List<int> e_forward_topo,e_backward_topo; // epsilon edges that don't make cycles are handled by propogating forward/backward in these orders (state = int because of graph.h)

    inline void matrix_compute(IOSymSeq const& s,bool backward=false)
    {
        if (backward) {
            matrix_compute(s.i.n,s.i.rLet,s.o.n,s.o.rLet,x.final,b,mio.backward,e_backward_topo);
            // since the backward paths were obtained on the reversed input/output, reverse them back
            matrix_reverse_io(b,s.i.n,s.o.n);
        } else
            matrix_compute(s.i.n,s.i.let,s.o.n,s.o.let,0,f,mio.forward,e_forward_topo);
    }

    void matrix_compute(int nIn,int *inLet,int nOut,int *outLet,int start,Weight ***w,matrix_io_index::states_t &io,List<int> const& eTopo);

    inline void matrix_forward_prop(Weight ***m,matrix_io_index::for_io const* fio,unsigned s,unsigned i,unsigned o,unsigned d_i,unsigned d_o)
    {
        if (!fio) return;
        for (matrix_io_index::for_io::const_iterator dw=fio->begin(),e=fio->end();dw!=e;++dw) {
            arc_counts &a=arcs[dw->id];
            unsigned d=dw->dest;
            assert(a.dest()==d||a.src==d); // first: forward, second: reverse
            Weight &to=m[i+d_i][o+d_o][d];
            Weight const& from=m[i][o][s];
            Weight const& w=a.weight();
#ifdef DEBUGFB
            Config::debug() << "w["<<i+d_i<<"]["<<o+d_o<<"]["<<d<<"] += " <<  "w["<<i<<"]["<<o<<"]["<<s<<"] * weight("<< *dw<<") ="<< to <<" + " << from <<" * "<< w <<" = "<< to <<" + " << from*w <<" = "<< to+(from*w)<<"\n";
#endif
            to += from * w;
        }
    }

    // accumulate counts for this example into scratch (so they can be weighted later all at once.  saves a few mults to weighting as you go?)
    inline void matrix_count(matrix_io_index::for_io const* fio,unsigned s,unsigned i,unsigned o,unsigned d_i,unsigned d_o)
    {
        if (!fio) return;
        for (matrix_io_index::for_io::const_iterator dw=fio->begin(),e=fio->end();dw!=e;++dw) {
            arc_counts &a=arcs[dw->id];
            assert(a.dest()==dw->dest);
            a.scratch += f[i][o][s] *a.weight() * b[i+d_i][o+d_o][dw->dest];
        }
    }

    //     newPerplexity = train_estimate();
    //	lastChange = train_maximize(method);
    //    Weight train_estimate(Weight &unweighted_corpus_prob,bool remove_bad_training=true); // accumulates counts, returns perplexity of training set = 2^(- avg log likelihood) = 1/(Nth root of product of model probabilities of N-weight training examples)  - optionally deletes training examples that have no accepting path
    //    Weight train_maximize(NormalizeMethods const& methods,FLOAT_TYPE delta_scale=1); // normalize then exaggerate (then normalize again), returning maximum change

// return corpus prob; print with Weight::print_ppx
// unweighted_corpus_prob: ignore per-example weight, product over corpus of p(example)
    Weight estimate(Weight &unweighted_corpus_prob);

 private:
    // these take an initialize unweighted_corpus_prob and counts, and accumulate over the training corpus
    Weight estimate_cached(Weight &unweighted_corpus_prob_accum);
    Weight estimate_matrix(Weight &unweighted_corpus_prob_accum);

    // uses derivs.weight to scale counts for that example.  returns unweighted prob, however
    Weight estimate_cached(derivations & derivs);
 public:

    // return max change
    Weight maximize(WFST::NormalizeMethods const& methods,FLOAT_TYPE delta_scale=1.);

    void matrix_fb(IOSymSeq &s);

    void e_topo_populate(bool include_backward)
    {
        assert(use_matrix);
        e_forward_topo.clear();
        e_backward_topo.clear();
        {
            Graph eGraph = x.makeEGraph();
            TopoSort t(eGraph,&e_forward_topo);
            t.order_crucial();
            int b = t.get_n_back_edges();
            if ( b > 0 )
                Config::warn() << "Warning: empty-label subgraph has " << b << " cycles!  Training may not propogate counts properly" << std::endl;
            if (include_backward) {
                Graph revEGraph = reverseGraph(eGraph);
                TopoSort t(revEGraph,&e_backward_topo);
                t.order_crucial();
                freeGraph(revEGraph);
            }
            freeGraph(eGraph);
        }
    }

    bool cache_backward;
//    serialize_batch<derivations> cached_derivs;

    forward_backward(WFST &x,cascade_parameters &cascade,bool per_arc_prior,Weight global_prior,bool include_backward,WFST::train_opts const& opts,training_corpus & corpus)
        : cache_t(x,corpus,opts.cache)
        , cascade(cascade),arcs(x,per_arc_prior,global_prior),mio(arcs)
    {
        cascade.set_composed(x);
        trn=NULL;
        f=b=NULL;
        remove_bad_training=true;
        if (opts.cache.cache()) {
            use_matrix=false;
            cache_backward=opts.cache.cache_backward();
            Config::log()<<"Caching derivations in "<<derivs.stored_in()<<std::endl;
        } else {
            use_matrix=true;
            mio.populate(include_backward);
            e_topo_populate(include_backward);
        }
        n_st=x.numStates();
        trn=&corpus;
        if (use_matrix) {
            n_in=corpus.maxIn+1; // because position 0->1 is first symbol, there are n+1 boundary markers
            n_out=corpus.maxOut+1;
            f = NEW Weight **[n_in];
            if (include_backward)
                b = NEW Weight **[n_in];
            else
                b=NULL;
            for ( unsigned i = 0 ; i < n_in ; ++i ) {
                f[i] = NEW Weight *[n_out];
                if (b)
                    b[i] = NEW Weight *[n_out];
                for ( unsigned o = 0 ; o < n_out ; ++o ) {
                    f[i][o] = NEW Weight [n_st];
                    if (b)
                        b[i][o] = NEW Weight [n_st];
                }
            }
        }
    }

    void matrix_dump(unsigned m_i,unsigned m_o)
    {
        assert (use_matrix && f && b);
        Config::debug() << "\nForwardProb/BackwardProb:\n";
        for (int i = 0 ;i<=m_i ; ++i){
            for (int o = 0 ; o <=m_o ; ++o){
                Config::debug() << i << ':' << o << " (" ;
                for (int s = 0 ; s < n_st ; ++s){
                    Config::debug() << f[i][o][s] <<'/'<<b[i][o][s];
                    if (s < n_st-1)
                        Config::debug() << ' ' ;
                }
                Config::debug() <<')'<<std::endl;
                if(o < m_o)
                    Config::debug() <<' ' ;
            }
            Config::debug() <<std::endl;
        }
    }

    training_corpus &corpus() const
    {
        return *trn;
    }

    // call after done using f,b matrix for a corpus
    void cleanup()
    {
        if (use_matrix && f) {
            for ( unsigned i = 0 ; i < n_in ; ++i ) {
                for ( unsigned o = 0 ; o < n_out ; ++o ) {
                    delete[] f[i][o];
                    if (b)
                        delete[] b[i][o];
                }
                delete[] f[i];
                if (b)
                    delete[] b[i];
            }
            delete[] f;
            if (b)
                delete[] b;
        }
        f=b=NULL;
    }

    ~forward_backward()
    {
        cleanup();
    }
};


Weight WFST::train(
                   training_corpus & corpus,NormalizeMethods const& methods,bool weight_is_prior_count,
                   Weight smoothFloor,Weight converge_arc_delta, Weight converge_perplexity_ratio,
                   train_opts const& opts
    )
{
    cascade_parameters cascade;
    return train(cascade,corpus,methods,weight_is_prior_count,smoothFloor,converge_arc_delta,converge_perplexity_ratio,opts);
}


Weight WFST::train(cascade_parameters &cascade,
                   training_corpus & corpus,NormalizeMethods const& methods,bool weight_is_prior_count,
                   Weight smoothFloor,Weight converge_arc_delta, Weight converge_perplexity_ratio,
                   train_opts const& opts
                   , bool restore_old_weights
                   )
{
    cascade.normalize(*this,methods);
    unsigned ran_restarts=opts.ran_restarts;
    double learning_rate_growth_factor=opts.learning_rate_growth_factor;
    random_restart_acceptor ra=opts.ra;

    forward_backward fb(*this,cascade,weight_is_prior_count,smoothFloor,true,opts,corpus);

    Weight bestPerplexity;
    bestPerplexity.setInfinity();
    bool very_first_time=true;
    bool using_cascade=!cascade.trivial;
    if (using_cascade) {
        if (learning_rate_growth_factor!=1.) {
            Config::warn() << "Overrelaxed EM not supported for --train-cascade.  Disabling (growth factor=1)."<<std::endl;
            learning_rate_growth_factor=1;
        }

    }

    bool have_good_weights=false;

    for(unsigned restart_no=0;;++restart_no) { // random restarts
        unsigned train_iter = 0;
        Weight lastChange;
        Weight lastPerplexity;
        lastPerplexity.setInfinity();
        FLOAT_TYPE learning_rate=1;
//        bool first_time=true;
        bool last_was_reset=false;
        for ( ; ; ) {
            const bool first_time=train_iter==0;
            ++train_iter;
            time_report taken(Config::log(),"Time for iteration: ");
#ifdef DEBUGTRAIN
            Config::debug() << "Starting iteration: " << train_iter << '\n';
#endif

#ifdef DEBUG
#define DWSTAT(a) print_stats(arcs,a)
            arcs_table<arc_counts> const &arcs=fb.arcs;
#else
#define DWSTAT
#endif

//            DWSTAT("Before estimate");
            Weight corpus_p;
            bool cascade_counts=using_cascade && !first_time;

            if (cascade_counts) {
                fb.arcs.visit(for_arcs::save_counts()); // so you can later save_best_counts if you like the ppx
            }

            cascade.update(*this);

            if ( train_iter > opts.max_iter && have_good_weights) {
                Config::log()  << "Maximum number of iterations (" << opts.max_iter << ") reached before convergence criteria was met - greatest arc weight change was " << lastChange << "\n";
                break;
            }

            Weight p = fb.estimate(corpus_p); //lastPerplexity.isInfinity() // only delete no-path training the first time, in case we screw up with our learning rate
            Weight newPerplexity=p.ppxper(corpus.totalEmpiricalWeight);
            DWSTAT("After estimate");
            Config::log() << "i=" << train_iter << " (rate=" << learning_rate << "): ";
//            Config::log() << " per-output-symbol-perplexity="<<corpus_p.ppxper(corpus.n_output).as_base(2)<<" per-example-perplexity="<<newPerplexity.as_base(2);
            corpus_p.print_ppx(Config::log(),corpus.n_output,corpus.n_pairs); //FIXME: newPerplexity is training-example-weighted
            if ( newPerplexity < bestPerplexity && (!using_cascade || cascade_counts)) { // because of how I'm saving only composed counts, we can't actually get back to our initial starting point (iter 1)
                Config::log() << " (new best)";
                bestPerplexity=newPerplexity;
                have_good_weights=true;
                if (using_cascade)
                    fb.arcs.visit(for_arcs::save_best_counts());
                else
                    fb.arcs.visit(for_arcs::save_best()); // drawn from pre-normalization counts in case of non-trivial cascade, post-norm weights otherwise
            }

            Weight pp_ratio_scaled;
            if ( first_time ) {
                Config::log() << std::endl;
                if (!ra.accept(newPerplexity,bestPerplexity,restart_no,&Config::log())) {
                    Config::log() << "Random start was insufficiently promising; trying another."<<std::endl;
                    break; // to next random restart
                }
                pp_ratio_scaled.setZero();
            } else {
                pp_ratio_scaled = newPerplexity.relative_perplexity_ratio(lastPerplexity);
//Weight pp_ratio=newPerplexity/lastPerplexity;
                //pp_ratio_scaled=pp_ratio.root(std::fabs(newPerplexity.getLogImp())); // EM delta=(L'-L)/abs(L')
                Config::log() << " (relative-perplexity-ratio=" << pp_ratio_scaled << "), max{d(weight)}=" << lastChange;
#ifdef DEBUG_ADAPTIVE_EM
                Config::log()  << " last-perplexity="<<lastPerplexity<<' ';
                if ( learning_rate > 1) {
                    fb.arcs.visit(for_arcs::swap_em_scaled());

                    Weight d;
                    Weight em_pp=fb.estimate(d);

                    Config::log() << "unscaled-EM-perplexity=" << em_pp;

                    fb.arcs.visit(for_arcs::swap_em_scaled());

                    if (em_pp > lastPerplexity)
                        Config::warn() << " - last EM worsened perplexity, from " << lastPerplexity << " to " << em_pp << ", which is theoretically impossible." << std::endl;
                }
#endif
                Config::log() << std::endl;

            }
            if (!last_was_reset) {
                if (  pp_ratio_scaled >= converge_perplexity_ratio ) {
                    if ( learning_rate > 1 ) {
                        Config::log() << "Failed to improve (relaxation rate too high); starting again at learning rate 1" << std::endl;
                        learning_rate=1;
                        fb.arcs.visit(for_arcs::keep_em_weight());

                        last_was_reset=true;
                        continue;
                    }
                    Config::log() << "Converged - per-example perplexity ratio exceeds " << converge_perplexity_ratio << " after " << train_iter << " iterations.\n";
                    if (!have_good_weights)
                        Config::log() << "Because of the --train-cascade implementation, we need another iteration even though we've converged.\n";
                    else
                        break;
                } else {
                    if (learning_rate < MAX_LEARNING_RATE_EXP)
                        learning_rate *= learning_rate_growth_factor;
                }
            } else //FIXME: should I do: // if (!using_cascade || !first_time) or is the ppx reported for cascade accurate?  i think it is accurate; it's just that we need to have saved counts after an estimate, so we can't save a global best at i=1
                    last_was_reset=false;

//            DWSTAT("Before maximize");
            lastChange = fb.maximize(methods,learning_rate);
//            DWSTAT("After maximize");
            if (lastChange <= converge_arc_delta && have_good_weights) {
                Config::log() << "Converged - maximum weight change less than " << converge_arc_delta << " after " << train_iter << " iterations.\n";
                break;
            }

            lastPerplexity=newPerplexity;
        }
        if (ran_restarts > 0) {
            --ran_restarts;
            cascade.random_restart(*this,methods);
            Config::log() << "\nRandom restart - " << ran_restarts << " remaining.\n";
        } else {
            break;
        }
    }
    Config::log() << "Setting weights to model with lowest per-example-perplexity ( = prod[modelprob(example)]^(-1/num_examples) = 2^(-log_2(p_model(corpus))/N) = "<<bestPerplexity.as_base(2);

    Config::log() << std::endl;
    fb.arcs.visit(for_arcs::use_best_weight());
    cascade.use_counts_final(*this,methods);


    //        for ( List<IOSymSeq>::val_iterator seq=trn->examples.val_begin(),end = trn->examples.val_end() ; seq !=end ; ++seq ) seq->kill();

    return bestPerplexity;
}

// possible speedups:
// * exclude states that are definitely not on any path start->finish matching i:o
// ** quick check: exclude if none of the outgoing transitions are (input|*e*):(output|*e*) for some input, output in sequence
/*
// ** complete check: integrate search and summation of paths - search state: (s,i,o)
constraint: can't visit (dequeue) (s,i,o) until already for all t (t,i-1,o-1),(t,i,o-1),(t,i-1,o) and all u (u,i,o) s.t. u->*e*->s
easy to meet first 3 using for i: for o: loop; add state to agenda for next (i|i+1,o|o+1)
when you have an *e* leaving something on the agenda, you need to add the dest to the head of the agenda, and, you must preorder the agenda so that if a->*e*->b, a cannot occur after b in the agenda.  you could sort the agenda by reverse dfs finishing times (dfs on the whole e graph though)

*/
// ** in order for excluding states to be worthwhile in O(s*i*o) terms, have to take as input a 0-initialized w matrix and clear each non-0 entry after it is no longer in play.  ouch - that means all the lists (of nonzero values) need to be kept around until after people are done playing with the w

void forward_backward::matrix_compute(int nIn,int *inLet,int nOut,int *outLet,int start,Weight ***w,matrix_io_index::states_t &io,List<int> const& eTopo)
{

    int i, o, s;
    for ( i = 0 ; i <= nIn ; ++i )
        for ( o = 0 ; o <= nOut ; ++o )
            for ( s = 0 ; s < n_st ; ++s )
                w[i][o][s].setZero();

    w[0][0][start] = 1;

    IOPair IO;

    typedef matrix_io_index::for_io for_io;
    for_io *pLDW;

    for ( i = 0 ; i <= nIn ; ++i ) {
        for ( o = 0 ; o <= nOut ; ++o ) {
#ifdef DEBUGFB
            Config::debug() <<"("<<i<<","<<o<<")\n";
#endif
            IO.in = 0;
            IO.out = 0;
            for ( List<int>::const_iterator topI=eTopo.const_begin(),end=eTopo.const_end() ; topI != end; ++topI ) {
                s = *topI;
                matrix_io_index::for_state const& fs = io[s];
                matrix_forward_prop(w,find_second(fs,IO),s,i,o,0,0);
            }
            for ( s = 0 ; s < n_st; ++s ) {
                if ( w[i][o][s].isZero() )
                    continue;
                matrix_io_index::for_state const& fs = io[s];
                if ( o < nOut ) {
                    IO.in = 0;
                    IO.out = outLet[o];
                    matrix_forward_prop(w,find_second(fs,IO),s,i,o,0,1);
                    if ( i < nIn ) {
                        IO.in = inLet[i];
                        IO.out = outLet[o];
                        matrix_forward_prop(w,find_second(fs,IO),s,i,o,1,1);
                    }
                }
                if ( i < nIn ) {
                    IO.in = inLet[i];
                    IO.out = 0;
                    matrix_forward_prop(w,find_second(fs,IO),s,i,o,1,0);
                }
            }
        }
    }
}


void forward_backward::matrix_fb(IOSymSeq &s)
{
#ifdef DEBUGFB
    Config::debug() << "training example: \n"<<s << "\nForward\n" ;
#endif
    matrix_compute(s,false);
#ifdef DEBUGFB
    Config::debug() << "\nBackward\n";
#endif
    matrix_compute(s,true);

#ifdef DEBUGTRAINDETAIL // Yaser 7-20-2000
    matrix_dump(s.i.n,s.o.n);
#endif
}

void training_progress(unsigned train_example_no,unsigned scale)
{
    const unsigned EXAMPLES_PER_DOT=scale;
    const unsigned EXAMPLES_PER_NUMBER=(70*EXAMPLES_PER_DOT);
    if (train_example_no % EXAMPLES_PER_DOT == 0)
        Config::log() << '.' ;
    if (train_example_no % EXAMPLES_PER_NUMBER == 0)
        Config::log() << train_example_no << '\n' ;
}


void warn_no_derivations(WFST const& x,IOSymSeq const& s,unsigned n)
{

    Config::warn() << "No derivations in transducer for input/output #"<<n<<":\n";
    s.print(Config::warn(),x,"\n");
}

Weight forward_backward::estimate(Weight &unweighted_corpus_prob)
{
    arcs.visit(for_arcs::clear_count());
    unweighted_corpus_prob=1;
    Weight p;
    if (use_matrix)
        p=estimate_matrix(unweighted_corpus_prob);
    else
        p=estimate_cached(unweighted_corpus_prob);
    return p;
}

Weight forward_backward::estimate_cached(Weight &unweighted_corpus_prob_accum)
{
    assert(!use_matrix);
    Weight weighted_corpus_prob=1;
    unsigned n=0;
    for (derivs.rewind();derivs.advance();) {
        ++n;
        training_progress(n);
        derivations & d=derivs.current();
        Weight prob=estimate_cached(d);
        unweighted_corpus_prob_accum *= prob;
        weighted_corpus_prob *= prob.pow(d.weight);
    }
    return weighted_corpus_prob;
}

Weight forward_backward::estimate_cached(derivations & derivs)
{
    return derivs.collect_counts(arcs);
}



Weight forward_backward::estimate_matrix(Weight &unweighted_corpus_prob)
{
    assert(use_matrix && b);
    int i, o, s, nIn, nOut, *letIn, *letOut;

    // for perplexity
    Weight ret = 1;


    typedef matrix_io_index::for_io for_io;
    for_io *pLDW;
    IOPair io;


    List<IOSymSeq>::erase_iterator seq=corpus().examples.erase_begin(),lastExample=corpus().examples.erase_end();
    //#ifdef DEBUGTRAIN
    int train_example_no = 0 ; // Yaser 7-13-2000
    //#endif

#ifdef DEBUG_ESTIMATE_PP
    Config::debug() << " Exampleprobs:";
#endif

    while (seq != lastExample) { // loop over all training examples

        //#ifdef DEBUGTRAIN // Yaser 13-7-2000 - Debugging messages ..
        ++train_example_no ;
        training_progress(train_example_no);
        //#endif
        nIn = seq->i.n;
        nOut = seq->o.n;
        matrix_fb(*seq);
        Weight fin = f[nIn][nOut][x.final];
#ifdef DEBUG_ESTIMATE_PP
        Config::debug() << ',' << fin;
#endif

        ret *= fin.pow(seq->weight); // since perplexity = 2^(- avg log likelihood)=2^((-1/n)*sum(log2 prob)) = (2^sum(log2 prob))^(-1/n) , we can take prod(prob)^(1/n) instead; prod(prob) = ret, of course.  raising ^N does the multiplication N times for an example that is weighted N
        unweighted_corpus_prob *= fin;


        if (remove_bad_training)
            if ( !(fin.isPositive()) ) {
                warn_no_derivations(x,*seq,train_example_no);
                seq=trn->examples.erase(seq);
                continue;
            }
        check_fb_agree(fin,b[0][0][0]);

        letIn = seq->i.let;
        letOut = seq->o.let;

    arcs.visit(for_arcs::clear_scratch());

        // accumulate counts for each arc's contribution throughout all uses it has in explaining the training
        for ( i = 0 ; i <= nIn ; ++i ) // go over all symbols in input in the training pair
            for ( o = 0 ; o <= nOut ; ++o ) // go over all symbols in the output pair
                for ( s = 0 ; s < n_st ; ++s ) {
                    matrix_io_index::for_state const& fs = mio.forward[s];
                    if ( i < nIn ) { // input is not epsilon
                        io.in = letIn[i];
                        if ( o < nOut ) { // output is also not epsilon
                            io.out = letOut[o];
                            matrix_count(find_second(fs,io),s,i,o,1,1);
                        }
                        io.out = 0; // output is epsilon, input is not
                        matrix_count(find_second(fs,io),s,i,o,1,0);
                    }
                    io.in = 0; // input is epsilon
                    if ( o < nOut ) { // input is epsilon, output is not
                        io.out = letOut[o];
                        matrix_count(find_second(fs,io),s,i,o,0,1);
                    }
                    io.out = 0; // input and output are both epsilon
                    matrix_count(find_second(fs,io),s,i,o,0,0);
                }

        arcs.visit(for_arcs::add_weighted_scratch(seq->weight/fin));
        //        Weight mult=seq->weight;
        //        EACHDW(if (!dw->scratch.isZero()) dw->counts += mult*(dw->scratch / fin););

        ++seq;
    } // end of while(training examples)

    return ret; // ,trn->totalEmpiricalWeight); // return per-example perplexity = 2^entropy=p(corpus)^(-1/N)
}

void WFST::train_prune() {
    /*
      int n_states=numStates();
      bool *dead_states=NEW bool[n_states]; // blah: won't really work unless we also delete stuff from trn, so postponing
      for (int i=0;i<n_states;++i) {
      Weight sum=0;
      for ( HashTable<IOPair, List<DWPair> >::iterator ha(trn->forArcs[s]) ; ha ; ++ha ){ \
      List<DWPair>::val_iterator end = ha.val().val_end() ; \
      for ( List<DWPair>::val_iterator dw=ha.val().val_begin() ; dw !=end ; ++dw ) {\
      dead_states[i]=false;
      dead_states[i]=true;

      }
      delete[] dead_states;
    */

}

std::ostream& operator << (std::ostream &o,arc_counts const& ac)
{
    int pGroup;
    if ( !WFST::isNormal(pGroup = ac.groupId()) )
        o << pGroup << ' ' ;                                                                                                                                    \
    o<< ac.src << "->" << *ac.arc <<  " weight " << ac.weight() << " scratch: "<< ac.scratch  <<" counts " <<ac.counts  << '\n';
    return o;
}

std::ostream& operator << (std::ostream &o,gibbs_counts const& ac)
{
    o<< "->" << *ac.arc <<'\n';
    return o;
}

Weight forward_backward::maximize(WFST::NormalizeMethods const& methods,FLOAT_TYPE delta_scale)
{

#ifdef DEBUGTRAINDETAIL
#define DUMPDW(h) arcs.dump(Config::debug(),h)
#else
#define DUMPDW(h)
#endif

    DUMPDW("Weights before prior smoothing");

    //    arcs.pre_norm_counts(corpus.totalEmpiricalWeight);
    arcs.visit(for_arcs::prep_new_weights(1.0));


//    DUMPDW("Weights before normalization");

//    DWSTAT("Before normalize");
    cascade.use_counts(x,methods); // doesn't actually put weights back into x for nontrivial cascade, which is why the following is skipped for cascades.  update prior to estimate puts the weights in place.

    if (cascade.trivial) {
        DUMPDW("Weights after normalization");
//        DWSTAT("After normalize");

        arcs.visit(for_arcs::overrelax(delta_scale));

        //    arcs.overrelax();

        // find maximum change for convergence

        if (delta_scale > 1.)
            x.normalize(methods[0]);

        //    return arcs.max_change();
        for_arcs::max_change c;
        arcs.visit(c);
        return c.get();
    } else
        return 1;
}

Weight WFST::sumOfAllPaths(List<int> &inSeq, List<int> &outSeq)
{
    Assert(valid());
    training_corpus corpus;
    corpus.add(inSeq,outSeq);
    corpus.finish_adding();
    IOSymSeq const& s=corpus.examples.front();
    cascade_parameters trivial;
    forward_backward fb(*this,trivial,false,0,false,train_opts(),corpus);
    fb.matrix_compute(s,false);
    return fb.f[s.i.n][s.o.n][final];
}

ostream& operator << (ostream &out, struct State &s){ // Yaser 7-20-2000
    out << s.arcs << '\n';
    return(out);
}

ostream & operator << (ostream &o, IOPair p)
{
    return o << '(' << p.in << ' ' << p.out << ')';
}

ostream & operator << (ostream &o, DWPair p)
{
    return o << "#"<<p.id<<"->"<<p.dest;
}


ostream & hashPrint(const HashTable<IOPair, List<DWPair> > &h, ostream &o) {
    HashTable<IOPair,List<DWPair> >::const_iterator i=h.begin();
    if ( i==h.end() ) return o;
    goto FIRST;
    while ( ++i,i!=h.end() ) {
        o << ' ';
    FIRST:
        o << '(' << i->first << ' ' << i->second << ')';
    }
    return o;
}


ostream & operator << (ostream & out , const symSeq & s){   // Yaser 7-21-2000
    for (int i=0 ; i < s.n ; ++i)
        out << s.let[i] ;
    out << '\n';
    return(out);
}

ostream & operator << (ostream & out , const IOSymSeq & s){   // Yaser 7-21-2000
    out << s.i << s.o ;
    return(out);
}

void WFST::read_training_corpus(std::istream &in,training_corpus &corpus)
{
    string buf;
    unsigned input_lineno=0;
    for ( ; ; ) {
        FLOAT_TYPE weight = 1;
        getline(in,buf);
        if ( !in )
            break;
        ++input_lineno;

        if ( isdigit(buf[0]) || buf[0] == '-' || buf[0] == '.' ) {
            istringstream w(buf.c_str());
            w >> weight;
            if ( w.fail() ) {
                Config::warn() << "Bad training example weight: " << buf << std::endl;
                continue;
            }
            getline(in,buf);
            if ( !in )
                break;
            ++input_lineno;
        }
        WFST::symbol_ids ins(*this,buf.c_str(),0,input_lineno);
        getline(in,buf);
        if ( !in )
            break;
        ++input_lineno;

        WFST::symbol_ids outs(*this,buf.c_str(),1,input_lineno);
        corpus.add(ins, outs, weight);
    }
    corpus.finish_adding();
}

}
