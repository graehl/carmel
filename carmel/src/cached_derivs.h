#ifndef GRAEHL_SHARED__CACHED_DERIVS
#define GRAEHL_SHARED__CACHED_DERIVS

#include <graehl/carmel/src/derivations.h>
#include <graehl/shared/serialize_batch.hpp>
#include <graehl/shared/time_space_report.hpp>
#include <graehl/shared/periodic.hpp>

namespace graehl {


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
            compute_derivations(corpus.examples,copt.cache_backward());
        }
    }
    template <class Examples>
    void compute_derivations(Examples const &ex,bool cache_backward)
    {
        std::ostream &log=Config::log();
        log<<"Caching derivations:\n";
        graehl::time_space_report r(log,"Computed cached derivations: ");
        wfst_io_index io(x);
        unsigned n=1;
        derivs.clear();
        for (typename Examples::const_iterator i=ex.begin(),end=ex.end();
             i!=end ; ++i,++n) {
            num_progress(log,n,10,70,".","\n");
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
        log << "\n";
        derivs.mark_end();
        log << derivations::global_stats;
    }
    template <class F>
    void foreach_deriv(F &f)
    {
        unsigned n=0;
        for (derivs.rewind();derivs.advance();) {
            ++n;
            f(n,derivs.current());
        }
    }
};


}


#endif
