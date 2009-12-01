#include <graehl/carmel/src/cached_derivs.h>
#include <graehl/carmel/src/cascade.h>
#include <graehl/carmel/src/train.h>
#include <graehl/carmel/src/fst.h>
#include <graehl/shared/gibbs.hpp>
#include <graehl/shared/segments.hpp>


namespace graehl {

struct carmel_gibbs : public gibbs_base
{
    bool have_names;

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
        , derivs(composed,cascade,corpus,topt.cache) // gets pre-init_sample_weights weight.
        , init_sample_weights(init_sample_weights)
    {
        set_cascadei();
        if (init_sample_weights && !cascade.trivial)
            composed.restore_weights(*init_sample_weights);
        have_names=gopt.rich_counts;
        set_gibbs_params(gopt.printing_sample()||have_names,have_names);
        cascade.set_trivial_gibbs_chains();
        pinit_differs_p0=init_sample_weights&&!gopt.em_p0;
    }

    void run()
    {
        gibbs_base::run_starts(*this);
        // copy weights to transd. so path weights are right?
        gibbs_base::print_all(*this);
        probs_to_cascade();
    }

    void probs_to_cascade()
    {
        for (unsigned i=0,N=cascade.size();i<N;++i) {
            WFST &w=*cascade.cascade[i];
            WFST::NormalizeMethod const& nm=methods[i];
            visit_wfst_params(*this,w,nm);
        }
    }
    // for probs_to_cascade()
    void operator()(unsigned src,FSTArc & f) const
    {
        f.weight.setReal(gibbs_base::final_prob(f.groupId));
    }

    enum { first_id=0 };

    void set_gibbs_params(bool addarcs=false,bool addsource=false)
    {
        for (unsigned norm=0,i=0,N=cascade.size();i<N;++i) {
            WFST &w=*cascade.cascade[i];
            WFST::NormalizeMethod const& nm=methods[i];
            norm=add_gibbs_params(norm,w,nm,addarcs,addsource);
        }
    }
    // add params w/ norm group==NONE
    void operator()(FSTArc const& a)
    {
        define_param(a.weight.getReal());
        const_cast<FSTArc&>(a).groupId=gps.size();
    }

    typedef dynamic_array<FSTArc *> arc_for_param_t;
    arc_for_param_t arcs;
    // compute the prior pseudocount gps[i].prior as alpha*M*p0 where M is the size of the normgroup and p0 is the (normalized) value on the arc.  if uniformp0, then pseudocount is just alpha (same as uniform p0)
    // return next free normgroup id, start at normidbase
    unsigned add_gibbs_params(unsigned /*normgroup start*/ id,WFST &w,WFST::NormalizeMethod const& nm,bool addarcs=false,bool addsource=false)
    {
        if (nm.group==WFST::NONE) {
//            w.lockArcs();
            w.visit_arcs_sourceless(*this);
            return id;
        }
        if (w.isEmpty())
            return id;
        if (nm.group==WFST::CONDITIONAL)
            w.indexInput();
        Weight ac=nm.add_count;
        double alpha=ac.getReal();
        bool uniformp0=gopt.uniformp0;
        for (NormGroupIter g(nm.group,w); g.moreGroups(); g.nextGroup()) {
            Weight sum=0;
            double N=0;
            Weight scale=one_weight();
            unsigned src=g.source();
            assert(src<w.numStates());
            if (!gopt.uniformp0) {
                for ( g.beginArcs(); g.moreArcs(); g.nextArc()) {
                    FSTArc const& a=**g;
                    if (!a.isLocked()) {
                        ++N;
                        sum+=a.weight;
                    }
                }
                if (N>0)
                    scale=N/sum;
            }
            for ( g.beginArcs(); g.moreArcs(); g.nextArc()) {
                FSTArc & a=**g;
                a.groupId=a.isLocked()
                    ? define_param(a.weight.getReal())
                    : define_param(id,(a.weight/sum).getReal(),alpha,N);
            /* FIXME: slight semantic difference: prob for locked doesn't compete w/ others in this normgroup.  solution: fixed array remains[normgrp] to go along w/ normsum[normgrp].  p=remains[normgrp]*count/normsum[normgrp].  where remains[i]=1-sum (locked arcs' prob in grp i)*/
                if (addarcs) arcs.push_back(&a);
                if (addsource) arc_sources.push_back(src);
            }
            ++id;
        }
        return id;
    }

    //same order as add_gibbs_params ; important: conditional norm will give different param order than --fem-param etc.
    template <class V>
    void visit_wfst_params(V &v,WFST &w,WFST::NormalizeMethod const& nm) const
    {
        if (nm.group==WFST::NONE) return;
        for (NormGroupIter g(nm.group,w); g.moreGroups(); g.nextGroup()) {
            unsigned src=g.source();
            for ( g.beginArcs(); g.moreArcs(); g.nextArc()) {
                FSTArc &a=**g;
                v(src,a);
            }
        }
    }

    dynamic_array<unsigned> arc_sources;
    void print_param(std::ostream &out,unsigned parami) const
    {
        assert(parami<arcs.size());
        assert(parami<arc_sources.size());
        wfst_for(parami).printArc(*arcs[parami],arc_sources[parami],out,false);
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
    unsigned cascadei_for(unsigned parami) const
    {
        unsigned ci=cascadei.segid0based(parami);
        assert(ci<cascade.cascade.size());
        return ci;
    }
    WFST &wfst_for(unsigned parami) const
    {
        unsigned ci=cascadei_for(parami);
        return *cascade.cascade[ci];
    }

    // single path in composition -> separate paths for each in cascade[i], for i in [a,b) put result in r[i]
    void paths(block_t const& p,unsigned a,unsigned b,cascade_paths &r)
    {
        assert(b<=r.size());
        for (unsigned i=a;i<b;++i)
            r[i].clear();
        for (block_t::const_iterator i=p.begin(),e=p.end();i!=e;++i) {
            unsigned parami=*i;
            unsigned ci=cascadei_for(parami);
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
//        printer.out()<<'\n'; // not needed; we already get a newline from printer
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
        cascadei.set_start(first_id);
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
#define OUTGIBBS3(x) //OUTGIBBS(x)
    double block_weight(unsigned block)
    {
        return derivs.derivs[block].weight;
    }

    void resample_block(unsigned block)
    {
        block_t &b=*blockp; // comes cleared.
        typedef dynamic_array<param_list> acpath;
        derivations &d=derivs.derivs[block];
        OUTGIBBS3(" block "<<block<<" line "<<d.lineno<<"\n");
        if (init_prob)
            d.random_path(p_init(*this),power);
        else
            d.random_path(*this,power);
        OUTGIBBS3('\n')
    }

#define CARMEL_GIBBS_FOR_ID(grapharc,paramid,body)        \
        for (param_list p=ac(grapharc);p;p=p->next) { \
            unsigned paramid=p->data->groupId; \
            body; }
    //for resample block:
    // *this is used as WeightFor in derivations pfor,random_path:
#define OUTGIBBS2(x) //OUTGIBBS(x)
#define DGIBBS2(x) //x

    Weight operator()(GraphArc const& a) const
    {
        Weight prob=one_weight();
        OUTGIBBS2("p("<<a<<"):");
        CARMEL_GIBBS_FOR_ID(a,id,{
                double p=gibbs_base::proposal_prob(id);
                prob*=p;
                OUTGIBBS2(" p("<<id<<")="<<p);
                //DGIBBS2(if (have_names) print_param(std::cerr<<"[",id)<<"]");
            })
            OUTGIBBS2(" = "<<prob<<'\n')
        return prob;
    }
//    block_t *blockp;
    void choose_arc(GraphArc const& a) const
    {
        DGIBBS2(CARMEL_GIBBS_FOR_ID(a,id,out<<" "<<id));
        OUTGIBBS2("=(ids for "<<a<<")\n");
        CARMEL_GIBBS_FOR_ID(a,id,blockp->push_back(id))
    }

    bool init_prob; // NOTE: unlike old method, composed weights don't get updated until all runs are done
    bool pinit_differs_p0;

    void init_run(unsigned r)
    {
        init_prob = r==0 && pinit_differs_p0;
        if (init_prob && init_sample_weights && cascade.trivial)
            composed.restore_weights(*init_sample_weights);
    }
    void init_iteration(unsigned i)
    {
        if (i>0) init_prob=false;
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
        cascade.save_weights(saved);
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
        cascade.restore_weights(saved);
    }
    //restore old weights; can't do gibbs init before em because groupId gets overwritten; em needs that id from composition
    carmel_gibbs g(*this,cascade,corpus,methods,topt,gopt,printer,restore?&init_sample_weights:0);
    saved.clear();
    g.run();
    cascade.clear_groups();
    cascade.update();
}

}
