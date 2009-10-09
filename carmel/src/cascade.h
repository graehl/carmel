#ifndef GRAEHL_CARMEL__CASCADE_H
#define GRAEHL_CARMEL__CASCADE_H

#include <vector>
#include <graehl/shared/array.hpp>
#include <graehl/carmel/src/fst.h>
#include <graehl/carmel/src/derivations.h>
#include <graehl/shared/slist.h>
#include <boost/pool/object_pool.hpp>

namespace graehl {
 // WARNING: thread unsafe for gibbs operator[](arc if trivial) identity node

// track pointers to arcs in a composition cascade, and host shared lists of them, given a global index in the cascade, so that the groupId of the composed FSTArc gives a list.
// note: composition of cascades gets its groupId set with this unique chain_id meaning a list of original input arcs
struct cascade_parameters
{
    WFST *pcomposed;
//FIXME: to simplify, in trivial case and otherwise, composed should be passed in once so it can be added to single chain (if trivial).  note: this simplification hasn't actually been done yet.
    void set_composed(WFST &c) {
        pcomposed=&c;
        if (trivial) {
            cascade.clear();
            cascade.push_back(pcomposed);
        }
    }
    bool trivial; // if true, we're not using cascade_parameters to do anything
                  // different from old carmel; we pass a trivial object around to
                  // avoid type/template dispatch at some small runtime cost

    typedef std::vector<WFST *> cascade_t;
    cascade_t cascade; // (in no particular order) all the transducers that were composed together
    //FIXME: per arc and global prior per cascade member (training of a cascade w/ prior has weird interpretation now, want per-cascade as well or instead)
    typedef FSTArc * param;
//    typedef unsigned param_id;
//    std::vector<param> params_byid;
    struct arcid
    {
        FSTArc *arc;
        unsigned xid; // transducer arc came from
    };


    typedef param param_id;
    typedef slist_node<param_id> node_t;
    typedef slist_shared<param_id> shared_list_t;
    typedef node_t * chain_t; //FIXME: make this a const pointer, but object_pool doesn't like it
    typedef std::vector<chain_t> chains_t; // // change w/ vector<arcid> ? so you know what component each arc came from
    chains_t chains;
    std::vector<Weight> chain_weights;
    typedef FSTArc::group_t chain_id;
    boost::object_pool<node_t> pool;
    chain_id nil_chain;
    unsigned debug; //bitfield
    enum { DEBUG_CHAINS=1,DEBUG_CASCADE=2,DEBUG_COMPOSED=4,DEBUG_COMPRESS=8,DEBUG_COMPRESS_VERBOSE=16 };

    typedef HashTable<param,chain_id> epsilon_map_t; // any pair of arcs from a*b will only occur once in composition, but a single epsilon a or b may reoccur many times. we want to use a single chain_id for all those, so we have to hash

    epsilon_map_t epsilon_chains;

    typedef WFST::saved_weights_t saved_weights_t;

    unsigned size() const
    {
        return trivial?1:cascade.size();
    }

    void save_weights(WFST const&composed,saved_weights_t &save) const
    {
        if (trivial) {
            composed.save_weights(save);
            return;
        }
        for (cascade_t::const_iterator i=cascade.begin(),e=cascade.end();
             i!=e;++i)
            (*i)->save_weights(save);
    }

    void restore_weights(WFST & composed,saved_weights_t const& save)
    {
        if (trivial) {
            composed.restore_weights(save.begin());
            return;
        }
        saved_weights_t::const_iterator si=save.begin();
        for (cascade_t::iterator i=cascade.begin(),e=cascade.end();
             i!=e;++i)
            si=(*i)->restore_weights(si);
        update(composed); // note: don't need to normalize
        //FIXME: is that update redundant in practice?
    }


    void clear_counts()
    {
        for (cascade_t::iterator i=cascade.begin(),e=cascade.end();
             i!=e;++i)
            (*i)->zero_arcs(); // skips actual locked arcs
    }

    void distribute_chain_counts(chain_t p,Weight counts)
    {
        for (;p;p=p->next)
            distribute_counts(*p->data,counts);
    }

    void distribute_counts(FSTArc &a,Weight counts)
    {
        if (!a.isLocked()) // because locked arcs w/ weight other than 1 need to appear in chain, but can't have their weights altered
            a.weight += counts;
    }


    void distribute_chain_id_counts(chain_id id,Weight counts)
    {
        assert(id>=0 && id<chains.size());             // note: by construction, a cascade-composed fst will have no locked arcs; rather, it will refer to a chain which references a locked arc
        distribute_chain_counts(chains[id],counts);
    }

    struct arcs_table_distribute_counts
    {
        cascade_parameters &cascade;
        arcs_table_distribute_counts(cascade_parameters &cascade) : cascade(cascade) {}
        void operator()(unsigned /*source*/,FSTArc const&a) const
        {
            // note: using weight which is, after prep_new_weights, including the global prior. //FIXME: per-arc prior in original transducers also
            cascade.distribute_chain_id_counts(a.groupId,a.weight);
        }
    };

    // for composed wfst that has arc weights which are unnormalized counts
    void distribute_counts(WFST &composed)
    //arcs_table &at
    {
        if (trivial) return;
        clear_counts();
        arcs_table_distribute_counts v(*this);
        composed.visit_arcs(v);
    }

    void use_counts(WFST &composed,WFST::NormalizeMethods const& methods)
    {
        distribute_counts(composed);
        normalize(composed,methods);
    }

    void use_counts_final(WFST &composed,WFST::NormalizeMethods const& methods)
    {
        if (trivial) return;
        use_counts(composed,methods);
        update(composed);
    }

    void set_trivial()
    {
        chain_weights.clear();
        chains.clear();
        epsilon_chains.clear();
        trivial=true;
    }

    cascade_parameters(bool remember_cascade=false,unsigned debug=0)
        : debug(debug),tempnode(NULL,NULL)
    {
        if ((trivial=!remember_cascade)) return;

        //chains.push_back(0); // we don't mind using a 0 index since we don't use groupids at all when cascade composing

        nil_chain=chains.size();
        chains.push_back(0); // canonical nil index for compositions  where every parameter was locked with weight of 1.
        // note composition will create locked -> final state arcs for epsilon filter finals.  but locked_group is 0, so you get nil_chain anyway
        assert(nil_chain==FSTArc::locked_group);

        // empty chain means: don't update the original arc in any way
    }

    unsigned set_gibbs_params(WFST &composed,WFST::NormalizeMethods & methods,WFST::gibbs_params &gps,bool p0init=true,unsigned startid=0)
    {
//        gps.push_back(0,0,0); //FIXME: unused index 0 for locked arcs
        if (trivial)
            return composed.set_gibbs_params(methods[0],startid,gps,p0init,0);
        else {
            unsigned id=startid;
            for (unsigned i=0,n=cascade.size();i<n;++i)
                id=cascade[i]->set_gibbs_params(methods[i],id,gps,p0init,i);
            return id;
        }
    }

    void normalize(WFST &composed,WFST::NormalizeMethods const& methods)
    {
        if (trivial)
            composed.normalize(methods[0]);
        else
            normalize(methods);
    }

    void normalize(WFST::NormalizeMethods const& methods)
    {
        for (unsigned i=0,n=cascade.size();i<n;++i)
            cascade[i]->normalize(methods[i]);
    }


    void randomize()
    {
            for (cascade_t::iterator i=cascade.begin(),e=cascade.end();
                 i!=e;++i)
                (*i)->randomSet();
    }
    void randomize(WFST &composed)
    {
        if (trivial)
            composed.randomSet();
        else
            randomize();
    }

    void normalize_and_update(WFST &composed,WFST::NormalizeMethods const& methods)
    {
        normalize(composed,methods);
        update(composed);
    }

    void random_restart(WFST &composed,WFST::NormalizeMethods const& methods)
    {
        randomize(composed);
        normalize(composed,methods); // update happens at top of random restarts loop already
    }

    // not to be used on arcs that weren't composed via cascade
    void update_composed_arc(FSTArc &comp)
    {
//        assert(!comp.isLocked());
        assert(comp.groupId < chain_weights.size());
        comp.weight=chain_weights[comp.groupId];
    }

    void accumulate_chain(Weight &w,chain_t p)
    {
        for (;p;p=p->next)
            w *= p->data->weight;
    }

    // todo: using structure of composition, could perhaps use the shared structure to reduce number of accumulates (linked list by index might be tricky but possible, toposort dependencies then go?
    void calculate_chain_weights()
    {
        chain_weights.clear();
        chain_weights.resize(chains.size(),Weight::ONE());
        for (unsigned i=0,e=chains.size();
             i!=e;++i) {
            accumulate_chain(chain_weights[i],chains[i]); // recall, nil chain will keep the default ONE as it's an empty list
        }
    }

    void print(std::ostream &o,bool cascade=true,bool chains=true)
    {
        if (cascade)
            print_cascade(o);
        if (chains)
            print_chains(o);
    }

    void print_cascade(std::ostream &o)
    {
        for (unsigned i=0,e=cascade.size();
             i!=e;++i)
            o << "cascade["<<i<<"]:\n"<<*cascade[i]<<"\n";
    }

    void print_chains(std::ostream &o,bool weights=true)
    {
        o << "Composed chains:\n";
        for (unsigned i=0,e=chains.size();
             i!=e;++i)
            print_chain(o,i,weights);
    }

    struct param_writer
    {
        template <class Ch, class Tr,class value_type>
        std::basic_ostream<Ch,Tr>&
        operator()(std::basic_ostream<Ch,Tr>& o,const value_type &l) const {
            return o << '@'<<l<<':'<<*l;
        }
    };

    void print_chain(std::ostream &o,unsigned i,bool weights=true)
    {
        o<<i<<": \t";
        if (weights)
            o<<chain_weights[i]<<" \t";
        shared_list_t c(chains[i]);
        c.print_writer(o,param_writer());
        o << "\n";
    }


    void update(WFST &composed)
    {
        if (trivial) return;
        if (debug&DEBUG_COMPOSED)
            Config::debug() << "composed pre:\n" << composed << std::endl;
        // composed has groupids that are indices into chains, unless trivial
        calculate_chain_weights();
        for (WFST::StateVector::iterator i=composed.states.begin(),e=composed.states.end();i!=e;++i) {
            State::Arcs &arcs=i->arcs;
            for ( State::Arcs::val_iterator l=arcs.val_begin(),end=arcs.val_end() ; l != end ; ++l )
                update_composed_arc(*l);
        }
        print(Config::debug(),debug&DEBUG_CASCADE,debug&DEBUG_CHAINS);
        if (debug&DEBUG_COMPOSED)
            Config::debug() << "composed post:\n" << composed << std::endl;
    }


    void clear_groups()
    {
        for (unsigned i=0,e=cascade.size();i!=e;++i)
            cascade[i]->clear_groups();
    }

    /*
    template <class P>
    struct gibbs_update
    {
        WFST &w;
        P const& p;
        gibbs_update(WFST &w,P const& p) : w(w),p(p) {}
        void operator()(unsigned src,FSTArc & a) const
        {
            a.weight=p(a);
        }
    };
    */
    template <class P>
    void update_gibbs(P const& p)
    {
        for (unsigned i=0,e=cascade.size();i!=e;++i)
            update_gibbs(*cascade[i],p);
    }

    template <class P>
    void update_gibbs(WFST &w,P const& p)
    {
//        gibbs_update<P> up(w,p);
        w.visit_arcs(p);
    }

    bool is_chain[2]; // is_chain[second] tells if arcs given to record are already chained, i.e. should you cons then append or just append

    void prepare_compose()
    {
        prepare_compose(false,false);
    }


    void prepare_compose(bool right_assoc) // shorthand for left->right.  right_assoc -> first is not chain, second is
    {
        if (right_assoc)
            prepare_compose(false,true);
        else
            prepare_compose(true,false);
    }

    void prepare_compose(bool first_chain,bool second_chain)
    {
        is_chain[0]=first_chain;
        is_chain[1]=second_chain;
    }


    chain_id record1(FSTArc const* e)
    {
        return record_eps(const_cast<param>(e),is_chain[0]);
    }

    chain_id record2(FSTArc const* e)
    {
        return record_eps(const_cast<param>(e),is_chain[1]);
    }

    inline chain_t cons(param a,chain_t cdr=0)
    {
        if (is_locked_1(a)) return cdr;
        return pool.construct(a,cdr);
    }

    inline chain_t cons(param a,param b)
    {
        return cons(a,cons(b));
    }

    /*
    inline chain_t cons(chain_t a,param b)
    {
        return cons(b,a);
        }*/

    //FIXME: untested, since we only compose strictly left or right assoc. - maybe better sharing w/ a tree cons structure (type flag indicates leaf? if you allow arbitrary assoc.
    inline chain_t cons(chain_t a,chain_t b)
    {
        for(;a;a=a->next)
            b=cons(a->data,b);
        return b;
    }

    chain_t operator[](FSTArc const*arc) const
    {
        return operator[](*arc);
    }
    mutable node_t tempnode;

    chain_t operator[](FSTArc const&arc) const
    {
        if (trivial) {
            tempnode.data=const_cast<FSTArc *>(&arc);
            return &tempnode;
        }
        return chains[arc.groupId];
    }

    chain_t cons_chain(param a,param b)
    {
        assert(!is_chain[0] || a->groupId <= chains.size());
        assert(!is_chain[1] || b->groupId <= chains.size());
        if (is_chain[0]) {
            chain_t ca=chains[a->groupId];
            if (is_chain[1])
                return cons(ca,chains[b->groupId]);
            else
                return cons(b,ca);
        } else {
            if (is_chain[1])
                return cons(a,chains[b->groupId]);
            else
                return cons(a,b);
        }
    }

    chain_id record(FSTArc const* a,FSTArc const* b)
    {
        return record(const_cast<param>(a),const_cast<param>(b));
    }

    static inline bool is_locked_1(param e)
    {
        return e->isLocked() && e->weight.isOne();
    }

    chain_id locked_1_groupid()
    {
        return trivial?WFST::locked_group:nil_chain;
    }

    inline chain_id original_id(param e)
    {
        return is_locked_1(e) ? nil_chain : e->groupId;
    }

    //if the entire (e) or (a,b) is all is_locked_1(), then undo push_back to vector and reference a canonical 'nil' chain_t  index
    chain_id record_eps(param e,bool chain=false)
    {
        if (trivial) return e->groupId; // for -a composition, but also means epsilons with tie groups in regular composition maintain group
        if (chain) return original_id(e);
        epsilon_map_t::insert_return_type ins=epsilon_chains.insert(e,chains.size()); // can't cache chains.size() for return since we only want to return that if we newly inserted
        if (ins.second) {
            chain_t v=cons(e);
            if (!v)
                return (ins.first->second=nil_chain);
            chains.push_back(v);
        }
        return ins.first->second;
    }

    chain_id record(param a,param b)
    {
        if (trivial) return FSTArc::no_group;
        // a * b should be generated at most once, so add it immediately
        //TODO: in theory a and/or b could be epsilons from earlier, and although we bothered to collapse the epsilons with epsilon_chains hash earlier, we'd be making redundant pairs.  a similar hash on pairs (a,b) could ensure that same-bracketing cons structures are stored only once
        chain_t v=cons_chain(a,b);
        if (!v)
            return nil_chain;
        chain_id ret=chains.size();
        chains.push_back(v);
        return ret;
    }

    // used to find now-defunct (after final-state-reachability reduction) arcs' chains and remove them.  should lead to some slight per-iteration speedup in calculate_weights and a little memory saving.
// the arc visitation skips locked arcs which come up e.g. with multiple finals during composition w.r.t epsilon-filter state.
    struct remap_chains
    {
        indices_after_removing newids;
        fixed_array<bool> remove;

        remap_chains(unsigned n,unsigned nil_chain) : remove(true,n) {
            remove[nil_chain]=false;
        }

        void find_used(WFST const& w)
        {
            w.visit_arcs_sourceless(*this);
            newids.init(remove);
            remove.clear();
        }

        // from above find_used
        void operator()(FSTArc const& a)
        {
            if (a.isLocked()) return;
            remove[a.groupId]=false;
        }

        void rewrite_arcs(WFST & w)
        {
            w.visit_arcs(*this);
        }

        // from above rewrite_arcs_using
        void operator()(unsigned s,FSTArc & a)
        {
            if (a.isLocked()) return;
            assert(!newids.removing(a.groupId));
            a.groupId=newids[a.groupId];
        }
    };

    // call *before* calculate_chain_weights (weights aren't moved also)
    void compress_chains(WFST &composed)
    {
        bool v=DEBUG_COMPRESS_VERBOSE&debug;
        bool d=DEBUG_COMPRESS&debug;
        debug_chains(d,"compress chains pre",v);
        remap_chains r(chains.size(),nil_chain);
        r.find_used(composed);
        r.rewrite_arcs(composed);
        r.newids.do_moves(chains);
        chain_weights.clear();
        debug_chains(d,"compress chains post",v);
    }

    void debug_chains(bool enable=true,char const* header="chains",bool verbose=true)
    {
        if (enable) {
            Config::debug()<<"\n"<<header;
            Config::debug()<<" ("<<chains.size()<<" entries)";
            if (verbose) {
                Config::debug()<<":\n";
                print_chains(Config::debug(),false);
                Config::debug()<<"\n";
            } else {
                Config::debug()<<".\n";
            }
        }
    }

    void done_composing(WFST &composed,bool compress_removed_arcs=false)
    {
        if (trivial) return;
        epsilon_chains.clear();
        if (compress_removed_arcs)
            compress_chains(composed);
    }

    void add(WFST *w)
    {
        if (trivial) return;
        cascade.push_back(w);
    }

};

}//ns


#endif
