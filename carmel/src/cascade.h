#ifndef GRAEHL_CARMEL__CASCADE_H
#define GRAEHL_CARMEL__CASCADE_H

/* TODO: for --crp with cascade of 1 transducer (set_trivial) make gibbs.cc set_gibbs_params work (set cascade.cascade[0] to original xdcr)
 */

#include <graehl/shared/array.hpp>
#include <graehl/shared/dynarray.h>
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
    void write_trained(std::string const& suffix,bool *flags,std::string *filenames,bool show0=false)
    {
        for (unsigned i=0,n=cascade.size();i<n;++i) {
            std::string const& f=filenames[i];
            std::string const& f_trained=suffix.empty()?f:(f+"."+suffix);
            Config::log() << "Writing "<<suffix<<' '<<f<<" to "<<f_trained<<std::endl;
            std::ofstream of(f_trained.c_str());
            WFST::output_format(flags,&of);
            cascade[i]->writeLegible(of,show0);
        }
    }

    typedef HashTable<FSTArc const*,unsigned> arcid_type;
    struct arcid_marker
    {
        arcid_type &aid;
        unsigned id;
        arcid_marker(arcid_type &aid,unsigned start=1) : aid(aid),id(start) {  }
        void operator()(FSTArc const& a)
        {
            aid.add(&a,id++);
        }
    };

    template <class V>
    void visit_arcs(V &v) const
    {
        for (unsigned i=0,n=cascade.size();i<n;++i)
            cascade[i]->visit_arcs_sourceless(v);
    }

    void arcids(arcid_type &aid,unsigned start=1) const
    {
        arcid_marker m(aid,start);
        visit_arcs(m);
    }

    struct number_v
    {
        unsigned i;
        number_v(unsigned start) : i(start-1) {  }
        void operator()(unsigned src,FSTArc & a)
        {
            a.groupId=++i;
        }
    };

    // set groupids
    void number_from(unsigned start=1) const
    {
        number_v v(start);
        for (unsigned i=0,n=cascade.size();i<n;++i)
//            start=cascade[i]->numberArcsFrom(start); //TEST: same order as visit_arcs_sourceless
            cascade[i]->visit_arcs(v); // definitely same order!
    }

    struct alpha_v
    {
        std::ostream &o;
        double prior; // -1 = locked
        alpha_v(std::ostream &o,WFST::NormalizeMethod const& m)
            : o(o)
            , prior(m.group==WFST::NONE ? -1 : m.add_count.getReal()) {}
        void operator()(FSTArc const& a) const
        {
            o << (a.isLocked()?-1:prior) <<'\n';
        }
    };


    void fem_alpha(std::ostream &o,WFST::NormalizeMethods const& methods) const
    {
        graehl::word_spacer sp('\n');
        for (unsigned i=0,n=cascade.size();i<n;++i) {
//            o<<sp;
            alpha_v v(o,methods[i]);
            cascade[i]->visit_arcs_sourceless(v);
        }
    }


    void fem_norms(std::ostream &o,WFST::NormalizeMethods const& methods) const
    {
        arcid_type aid;
        arcids(aid);
        fem_norms(o,aid,methods);
    }

    void fem_norms(std::ostream &o,arcid_type const& aid,WFST::NormalizeMethods const& methods) const
    {
        o<<"(";
        for (unsigned i=0,n=cascade.size();i<n;++i) {
            o<<"\n";
            fem_norms(o,aid,*cascade[i],methods[i]);
        }
        o<<")\n";
    }

    void fem_norms(std::ostream &o,arcid_type const& aid,WFST &w,WFST::NormalizeMethod const& nm) const
    {
        WFST::norm_group_by group=nm.group;
        if (group==WFST::NONE)
            return;
        if (group==WFST::CONDITIONAL)
            w.indexInput();
        for (NormGroupIter g(group,w); g.moreGroups(); g.nextGroup()) {
            o<<'(';
            for ( g.beginArcs(); g.moreArcs(); g.nextArc()) {
                o<<' '<<aid[*g];
            }

            o<<" )\n";
        }
    }

    // assumes (correctly!) that final state in deriv lattice has no outgoing arcs
    template <class arc_counts>
    void fem_deriv(std::ostream &o,arcs_table<arc_counts> const&arcs,arcid_type const& aid,derivations const& deriv) const
    {
//        printGraph(deriv.graph(),Config::debug());
        unsigned start=deriv.start(),fin=deriv.final();
        Graph g=deriv.graph();
        backrefs br(g,start);
        fem_deriv(o,arcs,aid,br,g.states,start,fin);
        o<<"\n";
    }

    //TODO: could share just like chains cons are shared.  but -a is best and means no sharing so meh
    template <class arc_counts>
    void fem_deriv(std::ostream &o,arcs_table<arc_counts> const&arcs,arcid_type const& aid,backrefs &br,GraphState *states,unsigned s,unsigned fin) const
    {
        const unsigned BACKREF_DEFINED=0;
        backref &b=br.ids[s];
        bool backdef=b.uses>1; // now, MUST have enclosing parens even for singleton
        if (backdef) {
            o<<"#"<<b.id;
            b.uses=BACKREF_DEFINED;
        } else if (b.uses==BACKREF_DEFINED) {
            o<<"#"<<b.id;
            return;
        }
        const List<GraphArc> &st=states[s].arcs;
        bool ornode=st.has2();
        if (ornode)
            o<<"(OR";
        for ( List<GraphArc>::const_iterator l=st.const_begin(),end=st.const_end() ; l !=end ; ++l ) {
            if (ornode) o<<" ";
            GraphArc const& a=*l;
            chain_t p=(*this)[arcs.ac(a).arc];
            unsigned n=a.dest;
            bool mid=n!=fin;
            bool nonleaf1=backdef||p&&(p->next||mid); // could postpone decision to use parens if !p (recursive flag passed in)
            if (nonleaf1) o<<"(";
            graehl::word_spacer space;
            for (;p;p=p->next)
                o<<space<<aid[p->data];
            if (mid) {
                o<<space;
                fem_deriv(o,arcs,aid,br,states,n,fin);
            }
            if (nonleaf1) o<<")";
        }
        if (ornode) o<<")";
    }

    struct print_params_f
    {
        std::ostream &o;
        print_params_f(std::ostream &o) : o(o) {  }
        void operator()(FSTArc const& a)
        {
            o<<a.weight<<"\n";
        }
    };


    void print_params(std::ostream &o) const
    {
        print_params_f p=o;
        visit_arcs(p);
/*        graehl::word_spacer sp('\n');
        for (unsigned i=0,n=cascade.size();i<n;++i) {
//            o<<sp;
            cascade[i]->visit_arcs_sourceless(p);
            }*/
    }

    struct read_params_f
    {
        std::istream &i;
        read_params_f(std::istream &i) : i(i) {  }
        void operator()(unsigned /*src*/,FSTArc &a)
        {
            if (!(i>>a.weight)) {
                throw std::runtime_error("--load-fem-param file doesn't have enough params; make sure it was --fem-param saved for the same cascade");
            }
        }
    };

    void read_params(std::istream &i) const
    {
        read_params_f r=i;
        for (unsigned i=0,n=cascade.size();i<n;++i) {
            cascade[i]->visit_arcs(r);
        }
    }

    WFST *pcomposed;
//FIXME: to simplify, in trivial case and otherwise, composed should be passed in once so it can be added to single chain (if trivial).  note: this simplification hasn't actually been done yet.
    void set_composed(WFST *c) {
        pcomposed=c;
        if (trivial)
            cascade.reinit(1,pcomposed);
    }
    bool trivial; // if true, we're not using cascade_parameters to do anything
                  // different from old carmel; we pass a trivial object around to
                  // avoid type/template dispatch at some small runtime cost

    typedef dynamic_array<WFST *> cascade_t;
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
    typedef dynamic_array<chain_t> chains_t; // // change w/ vector<arcid> ? so you know what component each arc came from
    chains_t chains;

    void set_trivial_gibbs_chains()
    {
        if (trivial) {
            assert(pcomposed);
            pcomposed->visit_arcs(*this);
        }
    }
    void operator()(unsigned s,FSTArc &a)
    {
        chains.at_grow(a.groupId)=pool.construct(&a,(chain_t)0);
    }

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

    void save_weights(saved_weights_t &save) const
    {
        for (cascade_t::const_iterator i=cascade.begin(),e=cascade.end();
             i!=e;++i)
            (*i)->save_weights(save);
    }

    void restore_weights(saved_weights_t const& save)
    {
        saved_weights_t::const_iterator si=save.begin();
        for (cascade_t::iterator i=cascade.begin(),e=cascade.end();
             i!=e;++i)
            si=(*i)->restore_weights(si);
        update();
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

    WFST &composed() const
    {
        return *pcomposed;
    }

    // take counts from composed, and add them to chain arcs' weight.  (clear_counts() 0s weight out first)
    void distribute_counts()
    //arcs_table &at
    {
        if (trivial) return;
        clear_counts();
        arcs_table_distribute_counts v(*this);
        composed().visit_arcs(v);
    }

    dynamic_array<WFST::saved_weights_t> none_saves;

#define DO_FOR_NONE(i,cond)                        \
    for (unsigned i=0,n=std::min(methods.size(),cascade.size());i!=n;++i)       \
        if (methods[i].group cond WFST::NONE)
#define FOR_NONE(i) DO_FOR_NONE(i,==)
#define EXCEPT_FOR_NONE(i) DO_FOR_NONE(i,!=)


    // a save,modify,load is used instead of associating each arc in chains with which transducer it is, so as to skip the NONE-normalized (weight-locked) ones.  only the NONE weights are saved/loaded, allowing the others to change.
    void save_none(WFST::NormalizeMethods const& methods)
    {
        none_saves.reinit(methods.size());
        FOR_NONE(i)
            cascade[i]->save_weights(none_saves[i]);
    }

    void load_none(WFST::NormalizeMethods const& methods)
    {
        FOR_NONE(i)
        {
            cascade[i]->restore_weights(none_saves[i]);
            none_saves[i].clear();
        }
    }


    void use_counts(WFST::NormalizeMethods const& methods)
    {
        distribute_counts();
        normalize(methods);
    }

    void use_counts_final(WFST::NormalizeMethods const& methods)
    {
        if (trivial) return;
        save_none(methods);
        use_counts(methods);
        load_none(methods);
        update();
    }

    void set_trivial()
    {
        chain_weights.clear();
//        chains.clear();
        epsilon_chains.clear();
        trivial=true;
    }

    cascade_parameters(bool remember_cascade=false,unsigned debug=0)
        : pcomposed(0),debug(debug)//,tempnode(NULL,NULL)
    {
        if ((trivial=!remember_cascade)) return;

        //chains.push_back(0); // we don't mind using a 0 index since we don't use groupids at all when cascade composing

        nil_chain=chains.size();
        chains.push_back((chain_t)0); // canonical nil index for compositions  where every parameter was locked with weight of 1.
        // note composition will create locked -> final state arcs for epsilon filter finals.  but locked_group is 0, so you get nil_chain anyway
        assert(nil_chain==FSTArc::locked_group);

        // empty chain means: don't update the original arc in any way
    }

    void normalize(WFST::NormalizeMethods const& methods)
    {
        assert(methods.size()==cascade.size());
        for (unsigned i=0,n=cascade.size();i<n;++i)
            cascade[i]->normalize(methods[i]);
    }


    void randomize(WFST::NormalizeMethods const& methods)
    {
        EXCEPT_FOR_NONE(i)
            cascade[i]->randomSet();
    }

    void normalize_and_update(WFST::NormalizeMethods const& methods)
    {
        normalize(methods);
        update();
    }

    void random_restart(WFST::NormalizeMethods const& methods)
    {
        randomize(methods);
        normalize(methods); // update happens at top of random restarts loop already
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


    void update()
    {
        if (trivial) return;
        if (debug&DEBUG_COMPOSED)
            Config::debug() << "composed pre:\n" << composed() << std::endl;
        // composed has groupids that are indices into chains, unless trivial
        calculate_chain_weights();
        WFST::StateVector &st=composed().states;
        for (WFST::StateVector::iterator i=st.begin(),e=st.end();i!=e;++i) {
            State::Arcs &arcs=i->arcs;
            for ( State::Arcs::val_iterator l=arcs.val_begin(),end=arcs.val_end() ; l != end ; ++l )
                update_composed_arc(*l);
        }
        print(Config::debug(),debug&DEBUG_CASCADE,debug&DEBUG_CHAINS);
        if (debug&DEBUG_COMPOSED)
            Config::debug() << "composed post:\n" << composed() << std::endl;
    }


    void clear_groups()
    {
        for (unsigned i=0,e=cascade.size();i!=e;++i)
            cascade[i]->clear_groups();
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

    chain_t operator[](FSTArc const&arc) const
    {
        /* // doesn't work because these are persisted in sample (acpaths).  would need memory pool; instead just initialize chains explicitly w/ set_trivial_gibbs_chains
        if (trivial) {
            tempnode.data=const_cast<FSTArc *>(&arc);
            return &tempnode;
        }
        */
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
    void compress_chains()
    {
        bool v=DEBUG_COMPRESS_VERBOSE&debug;
        bool d=DEBUG_COMPRESS&debug;
        debug_chains(d,"compress chains pre",v);
        remap_chains r(chains.size(),nil_chain);
        r.find_used(composed());
        r.rewrite_arcs(composed());
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

    void done_composing(WFST *composed,bool compress_removed_arcs=false)
    {
        set_composed(composed);
        if (trivial) return;
        epsilon_chains.clear();
        if (compress_removed_arcs)
            compress_chains();
    }

    void add(WFST *w)
    {
        if (trivial) return;
        cascade.push_back(w);
    }

};

}//ns


#endif
