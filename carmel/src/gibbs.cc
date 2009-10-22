#include <graehl/carmel/src/cached_derivs.h>
#include <graehl/carmel/src/cascade.h>
#include <graehl/carmel/src/train.h>
#include <graehl/carmel/src/fst.h>
#include <graehl/shared/gibbs.hpp>
#include <graehl/shared/segments.hpp>

#define DEBUG_GIBBS
#ifdef DEBUG_GIBBS
#define DGIBBS(a) a;
#else
#define DGIBBS(a)
#endif
#define OUTGIBBS(a) DGIBBS(std::cerr<<a)

namespace graehl {

struct carmel_gibbs : public gibbs_base
{
    carmel_gibbs(
        WFST &composed
        ,cascade_parameters &cascade
        ,training_corpus &corpus
        ,WFST::NormalizeMethods & methods
        ,WFST::train_opts const& topt
        ,gibbs_opts const& gopt
        ,WFST::path_print const& printer
        ,WFST::saved_weights_t *init_sample_weights=NULL
        ) :
        gibbs_base(gopt,corpus.n_output,corpus.n_pairs,printer.out(),Config::log())
        , composed(composed)
        , cascade(cascade)
        , methods(methods)
        , printer(printer)
        , derivs(composed,corpus,topt.cache) // gets pre-init_sample_weights weight.
        , init_sample_weights(init_sample_weights)
    {
        cascade.set_composed(composed);
        set_cascadei();
        if (init_sample_weights && !cascade.trivial)
            composed.restore_weights(*init_sample_weights);
        set_gibbs_params();
        cascade.set_trivial_gibbs_chains();
        pinit_differs_p0=init_sample_weights&&!gopt.em_p0;
    }

    void run()
    {
        gibbs_base::run_starts(*this);
        // copy weights to transd. so path weights are right?
        gibbs_base::print_all(*this);
        cascade.update_gibbs(*this);
    }
    // for cascade.update_gibbs
    void operator()(unsigned src,FSTArc & f) const
    {
        f.weight=gibbs_base::proposal_prob(gps[f.groupId]);
    }

    void set_gibbs_params()
    {
        for (unsigned norm=0,i=0,N=cascade.size();i<N;++i)
            norm=add_gibbs_params(norm,*cascade.cascade[i],methods[i],gopt.uniformp0);
    }
    typedef dynamic_array<FSTArc *> arc_for_param_t;
    arc_for_param_t arcs;
    // compute the prior pseudocount gps[i].prior as alpha*M*p0 where M is the size of the normgroup and p0 is the (normalized) value on the arc.  if uniformp0, then pseudocount is just alpha (same as uniform p0)
    // return next free normgroup id, start at normidbase
    unsigned add_gibbs_params(unsigned normidbase,WFST &w,WFST::NormalizeMethod & nm,bool uniformp0=false)
    {
        unsigned id=normidbase;
        if (w.isEmpty())
            return id;
        if (nm.group==WFST::NONE) {
            Config::warn()<<"Can't turn off normalization for gibbs training; changing to conditional.\n";
            nm.group=WFST::CONDITIONAL;
        }
        if (nm.group==WFST::CONDITIONAL)
            w.indexInput();
        Weight ac=nm.add_count;
        double alpha=ac.getReal();
        for (NormGroupIter g(nm.group,w); g.moreGroups(); g.nextGroup()) {
            Weight sum=0;
            double N=0;
            Weight scale=one_weight();
            if (!uniformp0) {
                for ( g.beginArcs(); g.moreArcs(); g.nextArc()) {
                    ++N;
                    sum+=(*g)->weight;
                }
                if (N>0)
                    scale=N/sum;
            }
            for ( g.beginArcs(); g.moreArcs(); g.nextArc()) {
                FSTArc & a=**g;
                a.groupId=gps.size();
                define_param(id,uniformp0?alpha:(ac*scale*a.weight).getReal());
                if (gopt.printing_sample()) arcs.push_back(&a);
            }
            ++id;
        }
        return id;
    }

    WFST &composed;
    cascade_parameters &cascade;
    WFST::NormalizeMethods const& methods;
    WFST::path_print printer;
    segments<unsigned> cascadei;
    typedef cascade_parameters::chain_t param_list; // a composed arc has a derivs.arcs entry eventually linking to a list of original cascade arcs making it up (will have either 0 or 1 items in a -a composition, however; we could simplify by forcing -a)
    cached_derivs<arc_counts_base> derivs; //derivs.arcs is an arcs_table<arc_counts_base>.  derivs.arcs.ac(GraphArc a) gives the arc_counts_base for a deriv arc a.  arc_counts_base just has FSTArc *arc (unlike EM arc_counts which also has counts etc.)
    FSTArc* composed_arc(GraphArc const& a) const
    {
        return derivs.arcs.ac(a).arc;
    }
    param_list ac(GraphArc const& a) const
    {
        return cascade[composed_arc(a)];
    }

    typedef WFST::saved_weights_t saved_weights_t;
    WFST::saved_weights_t *init_sample_weights; //FIXME: should probably go by the wayside; not very productive since burnin should give conditional indep. anyway; if we keep it, then store as a double array instead and modify p_init to reference that.


    typedef dynamic_array<unsigned> cascade_path;
    typedef fixed_array<cascade_path> cascade_paths;
    typedef cascade_parameters::cascade_t casc;
    // single path in composition -> separate paths for each in cascade[i], for i in [a,b) put result in r[i]
    void paths(block_t const& p,unsigned a,unsigned b,cascade_paths &r)
    {
        assert(b<=r.size());
        for (unsigned i=a;i<b;++i)
            r[i].clear();
        for (block_t::const_iterator i=p.begin(),e=p.end();i!=e;++i) {
            unsigned parami=*i;
            unsigned ci=cascadei.segid0based(parami);
            assert(ci<r.size());
            if (ci>=a && ci<b)
                r[ci].push_back(parami);
        }
    }
    void print_sample(blocks_t const& sample,unsigned a,unsigned b,casc const& c)
    {
        if (!(b>a && a<c.size())) {
            if (b!=0)
                Config::warn() << "--print-from,-to gibbs ["<<a<<","<<b<<") is out of range for "<<c.size()<<" input transducers.\n";
            return;
        }
        if (b>c.size()) b=c.size();
        if (a==b) return;
        for (blocks_t::const_iterator i=sample.begin(),e=sample.end();i!=e;++i)
            print_path(*i,a,b,c);
    }
    void print_cascade_path(WFST &w,cascade_path const& p)
    {
        unsigned N=p.size();
        fixed_array<FSTArc> apath(N); // could use transforming iterator but this is simpler, perf. noncritical
        for (unsigned i=0;i<N;++i) {
            unsigned parami=p[i];
            FSTArc const&a=*arcs[parami];
            FSTArc &ai=apath[i];
            ai.dest=a.dest;
            ai.in=a.in;
            ai.out=a.out;
            ai.groupId=FSTArc::no_group;
            ai.weight=gibbs_base::proposal_prob(parami);
        }
        printer(w,apath);
    }
    void print_path(block_t const& p,unsigned a,unsigned b,casc const &c)
    {
        cascade_paths ps(c.size());
        paths(p,a,b,ps);
        for (unsigned i=a;i<b;++i)
            print_cascade_path(*c[i],ps[i]);
        printer.out()<<'\n';
    }
    void print_sample(blocks_t const& sample,unsigned print_from,unsigned print_to)
    {
        print_sample(sample,print_from,print_to,cascade.cascade);
    }
    void print_sample(blocks_t const& sample)
    {
        print_sample(sample,gopt.print_from,gopt.print_to);
    }
    void set_cascadei()
    {
        cascadei.start(0);
        for (unsigned i=0,N=cascade.size();i!=N;++i)
            cascadei.add_delta(cascade.cascade[i]->n_edges());
    }

    struct p_init
    {
        carmel_gibbs const&c;
        p_init(carmel_gibbs const&c) : c(c) {  }
        double operator()(GraphArc const& a) const
        {
            return c.composed_arc(a)->weight.getReal(); //TESTME
        }
        void choose_arc(GraphArc const& a) const
        {
            return c.choose_arc(a);
        }
    };

    Weight resample_block(unsigned block)
    {
        block_t &b=sample[block];  // this is cleared for us already
        blockp=&b;
        typedef dynamic_array<param_list> acpath;
        derivations &d=derivs.derivs[block];
//        OUTGIBBS(" block "<<block<<" line "<<d.lineno<<"\n");
        if (init_prob)
            d.random_path(p_init(*this),power);
        else
            d.random_path(*this,power);
    }

    //for resample block:
    // *this is used as WeightFor in derivations pfor,random_path:
    double operator()(GraphArc const& a) const
    {
        double prob=1;
        for (param_list p=ac(a);p;p=p->next)
            prob*=gibbs_base::proposal_prob(p->data->groupId);
        return prob;
    }
    block_t *blockp;
    void choose_arc(GraphArc const& a) const
    {
        for (param_list p=ac(a);p;p=p->next)
            blockp->push_back(p->data->groupId);
    }

    bool init_prob; // NOTE: unlike old method, composed weights don't get updated until all runs are done
    bool pinit_differs_p0;

    void init_run(unsigned r)
    {
        init_prob = r==0 && pinit_differs_p0;
        if (init_prob && init_sample_weights && cascade.trivial)
            composed.restore_weights(*init_sample_weights);
    }
};


void WFST::train_gibbs(cascade_parameters &cascade, training_corpus &corpus, NormalizeMethods & methods, train_opts const& topt
                       , gibbs_opts const &gopt1, path_print const& printer, double min_prior)
{
    std::ostream &log=Config::log();
    for (NormalizeMethods::iterator i=methods.begin(),e=methods.end();i!=e;++i) {
        if (i->add_count<=0) {
            Config::warn() << "Gibbs sampling requires positive --priors for base model / initial sample.  Setting to "<<min_prior<<"\n";
            i->add_count=min_prior;
        }
    }
    gibbs_opts gopt=gopt1;
    gopt.iter=topt.max_iter;
    bool em=gopt.init_em>0;
    bool restore=em && !gopt.em_p0;
    saved_weights_t saved,init_sample_weights;
    if (restore)
        cascade.save_weights(*this,saved);
    if (em) {
        NormalizeMethods m2=methods;
        for (NormalizeMethods::iterator i=m2.begin(),e=m2.end();i!=e;++i)
            i->add_count=0;
        train_opts t2=topt;
        t2.max_iter=gopt.init_em;
        // EM:
        train(cascade,corpus,m2,false,0,0,1,t2,true);
    }
    if (restore) {
        save_weights(init_sample_weights);
        cascade.restore_weights(*this,saved);
    }
    //restore old weights; can't do gibbs init before em because groupId gets overwritten; em needs that id from composition
    carmel_gibbs g(*this,cascade,corpus,methods,topt,gopt,printer,restore?&init_sample_weights:0);
    saved.clear();
    g.run();
    cascade.clear_groups();
    cascade.update(*this);
}

}
