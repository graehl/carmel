#ifndef GRAEHL_SHARED__CACHED_DERIVS
#define GRAEHL_SHARED__CACHED_DERIVS

#include <graehl/carmel/src/derivations.h>
#include <graehl/carmel/src/cascade.h>
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
    std::string out_derivfile;
    cascade_parameters const& cascade;
    training_corpus &corpus;
    WFST::deriv_cache_opts const& copt;
    bool cached;

    unsigned size()
    {
        return cached?derivs.size():corpus.size();
    }
    double n_output() const
    {
        return corpus.n_output;
    }

    cached_derivs(WFST &x,cascade_parameters const& cascade,training_corpus &corpus,WFST::deriv_cache_opts const& copt)
        : x(x),derivs(copt.use_disk(),copt.disk_cache_filename,true,copt.disk_cache_bufsize),arcs(x),out_derivfile(copt.out_derivfile),cascade(cascade),corpus(corpus),copt(copt)
    {
        if (cached=copt.cache())
            cache_derivations();
        first=true; // for non-caching
    }
    bool first;

    template <class F>
    void foreach_deriv(F &f)
    {
        if (first&&!out_derivfile.empty()) {
            std::ofstream o(out_derivfile.c_str());
            foreach_deriv(f,&o);
        } else
            foreach_deriv(f,0);
    }

    static void warn_no_derivations(WFST const& x,IOSymSeq const& s,unsigned n)
    {
        Config::warn() << "No derivations in transducer for input/output #"<<n<<":\n";
        s.print(Config::warn(),x,"\n");
    }

    template <class F>
    void foreach_deriv(F &f,std::ostream *od)
    {
        cascade_parameters::arcid_type aid;
        bool fem=od&&first;
        if (fem)
            cascade.arcids(aid);
        if (cached) {
            unsigned n=0;
            for (derivs.rewind();derivs.advance();) {
                ++n;
                derivations &d=derivs.current();
                f(n,d);
                if (fem)
                    cascade.fem_deriv(*od,arcs,aid,d);
            }
        } else {
            wfst_io_index io(x); // TODO: lift outside of foreach deriv?
            unsigned n=0;
            List<IOSymSeq> &ex=corpus.examples;
            for (List<IOSymSeq>::erase_iterator i=ex.erase_begin(),end=ex.erase_end();i!=end;) {
                ++n;
                derivations d;
                if (d.init_and_compute(x,io,arcs,i->i,i->o,i->weight,n,copt.cache_backward(),copt.prune())) {
                    f(n,d);
                    if (fem)
                        cascade.fem_deriv(*od,arcs,aid,d);
                } else if (first) {
                    warn_no_derivations(x,*i,n);
                    if (copt.prune()) {
                      i=ex.erase(i);
                      continue;
                    }
                }
                ++i;
            }
            if (first) {
                corpus.count();
            }
        }
        first=false;
    }

    //TODO: cascade arc ids for fem deriv out
    void cache_derivations()
    {
        bool cache_backward=copt.cache_backward();
        bool prune=copt.prune();
        typedef List<IOSymSeq> Examples;
        Examples &ex=corpus.examples;
        cached=true;
        std::ostream &log=Config::log();
        log<<"Caching derivations:\n";
        graehl::time_space_report r(log,"Computed cached derivations: ");
        wfst_io_index io(x);
        unsigned n=1;
        derivs.clear();
        for (Examples::const_iterator i=ex.begin(),end=ex.end();
             i!=end ; ++i,++n) {
            num_progress(log,n,10,70,".","\n");
            derivations &d=derivs.start_new();
            corpus.clear_counts();
            if (!d.init_and_compute(x,io,arcs,i->i,i->o,i->weight,n,cache_backward,prune)) {
                warn_no_derivations(x,*i,n);
                derivs.drop_new();
            } else {
#ifdef DEBUG_DERIVATIONS_EXTRA
                Config::debug() << "Derivations in transducer for input/output #"<<n<<" (final="<<d.final()<<"):\n";
                i->print(Config::debug(),x,"\n");
                printGraph(d.graph(),Config::debug());
#endif
                derivs.keep_new();
                corpus.count(*i);
            }
        }
        log << "\n";
        derivs.mark_end();
        log << derivations::global_stats;
    }
};


}


#endif
