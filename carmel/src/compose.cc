#include <graehl/shared/config.h>
#include <graehl/carmel/src/compose.h>
#include <graehl/carmel/src/fst.h>
#include <graehl/carmel/src/cascade.h>
#include <graehl/shared/array.hpp>
#include <cstring>

namespace graehl {

int WFST::indexThreshold = 12;
unsigned int TrioKey::aMax = 0;
unsigned int TrioKey::bMax = 0;


//FIXME: use stringstream so there are no artifical name length limits
struct TrioNamer {
    char *buf,*buf_escape;
    const WFST &a;
    const WFST &b;
    char *limit;
    BOOST_STATIC_CONSTANT(unsigned,n_char=1 << (8 * sizeof(char)));
    BOOST_STATIC_CONSTANT(char,escape='\\');
    bool special[n_char];
    bool needs_escape;

    TrioNamer(unsigned capacity,const WFST &a_,const WFST &b_) : a(a_),b(b_) {
        buf=(char*)::operator new(capacity);
        buf_escape=(char*)::operator new(2*capacity+3);
        limit=buf+capacity-6; // shoudl be 4: end of string, | <filter-state> |.  but 6 to be safe.
        Assert(limit>buf);
        std::memset((char*)special,0,sizeof(bool)*n_char);
        special[escape]=special[' ']=special['(']=special[')']=1;
        needs_escape=false;
    }
    ~TrioNamer()
    {
        ::operator delete(buf);
        ::operator delete(buf_escape);
    }

    bool should_quote() const
    {
        if (buf[0]=='"')
            return true;
        for (char *p=buf;*p;++p) {
            if (*p=='(' || *p==')' || *p==' ') //*p=='\n' || *p=='\t'
                return true;
        }
        return false;
    }

    char const* finish()
    {
//        if (needs_escape) {
        if (should_quote()) {
            needs_escape=false;
            char *o=buf_escape;
            *o++='"';
            for (char *p=buf;*p;++p) {
                if (*p=='"' || *p==escape)
                    *o++=escape;
                *o++=*p;
            }
            *o++='"';
            *o=0;
            return buf_escape;
        }
        return buf;
    }

    void append_safe(char *&p,char const* str)
    {
        for (; *str && p<limit;++str ) {
            if (special[*p++ = *str])
                needs_escape=true;
        }

        if ( *str )
#define STATENAME_TOO_LONG_ERROR_TEXT(x)  "state name too long - exceeded " #x
            throw std::runtime_error(STATENAME_TOO_LONG_ERROR_TEXT(MAX_STATENAME_LEN));
    }

    char const* make(unsigned aState,unsigned bState,char filter) {
        char *p=buf;
        append_safe(p,a.stateName(aState));
        *p++ = '|';
        *p++ = filter + '0';
        *p++ = '|';
        append_safe(p,b.stateName(bState));
        *p = 0;
        return finish();
    }
    char const* make_mediate(unsigned astate,unsigned bstate,int middle_letter)
    {
        char *p=buf;
        append_safe(p,b.stateName(bstate));
        *p++=',';
        append_safe(p,a.outLetter(middle_letter));
        *p++='-';
        *p++='>';
        append_safe(p,a.stateName(astate));
        *p = 0;
        return finish();
    }

};

//#define OLDCOMPOSEARC
#ifdef DEBUGCOMPOSE
#define DUMPARC(a,b,c,d) Config::debug() << "arc" << FSTArc(a,b,c,d)
#else
#define DUMPARC(a,b,c,d)
#endif

#ifdef OLDCOMPOSEARC
#define COMPOSEARC do {       \
        if ( (pDest = find_second(stateMap,triDest)) ) \
        { states[sourceState].addArc(FSTArc(in, out, *pDest, weight)); \
        } else {             \
            add(stateMap,triDest,numStates()); trioID.num = numStates(); trioID.tri = triDest;    queue.push(trioID);    states[sourceState].addArc(FSTArc(in, out, trioID.num, weight));    push_back(states); \
            if ( namedStates ) {      stateNames.add(namer.make(triDest.aState, triDest.bState, triDest.filter)); \
            }  }} while(0)
#else

    //uses: stateMap[triDest] states, queue, in, out, weight, [namedStates,stateNames,namer,buf]

#define COMPOSEARC_GROUP(g) do {                                          \
        hash_traits<HashTable<TrioKey,int> >::insert_return_type i; \
        if ( (i = stateMap.insert(HashTable<TrioKey,int>::value_type(triDest,numStates()))).second ) { \
            trioID.num=numStates();trioID.tri = triDest;queue.push(trioID);push_back(states); \
            if ( namedStates ) stateNames.add(namer.make(triDest.aState, triDest.bState, triDest.filter),trioID.num); \
        } else trioID.num=i.first->second; \
        states[sourceState].addArc(FSTArc(in, out, trioID.num, weight,g)); DUMPARC(in, out, trioID.num, weight);} while(0)

#define COMPOSEARC COMPOSEARC_GROUP(FSTArc::no_group)
#endif


WFST::WFST(cascade_parameters &cascade,WFST &a, WFST &b, bool namedStates,bool groups)
{
    init_index();
    alph[0]=alph[1]=0;
    owner_alph[0]=owner_alph[1]=0;
    if (false && groups && !cascade.trivial)
        throw std::runtime_error("Don't set preserve groups (-a) along with --train-cascade; --train-cascade maps original parameters through a more efficient mechanism.");
    set_compose(cascade,a,b,namedStates,groups);
}

WFST::WFST(WFST &a, WFST &b, bool namedStates, bool preserveGroups)
{
    init_index();
    alph[0]=alph[1]=0;
    owner_alph[0]=owner_alph[1]=0;
    cascade_parameters c;
    set_compose(c,a,b,namedStates,preserveGroups);
}


void WFST::set_compose(cascade_parameters &cascade,WFST &a, WFST &b, bool namedStates, bool preserveGroups)
{
    deleteAlphabet();
    owner_alph[0]=owner_alph[1]=0;
    alph[0]=a.alph[0];
    alph[1]=b.alph[1];
    alphabet_type &aout=a.alphabet(1),&bin=b.alphabet(0);

    states.reserve(a.numStates()+b.numStates());

    const int EMPTY=epsilon_index;
    if ( !(a.valid() && b.valid()) ) {
        invalidate();
        return;
    }

    int *map = NEW int[aout.size()];
    int *revMap = NEW int[bin.size()];
    Assert(aout.verify());
    Assert(bin.verify());
    TrioNamer namer(MAX_STATENAME_LEN+1,a,b);
    aout.mapTo(bin, map);     // find matching symbols in interfacing alphabet
    bin.mapTo(aout, revMap);
    Assert(map[0]==0);
    Assert(revMap[0]==0); // *e* always 0
    TrioKey::aMax = a.numStates(); // used in hash function
    TrioKey::bMax = b.numStates();

    HashTable<TrioKey,int> stateMap(2 * (a.numStates() + b.numStates())); // assign state numbers
    // to composite states in the order they are first visited

    List<TrioID> queue;
    int sourceState = -1;
#ifdef OLDCOMPOSEARC
    int *pDest;
#endif
    int in, out;
    Weight weight;
    TrioKey triSource, triDest;
    TrioID trioID;
    trioID.num = 0;
    trioID.tri = TrioKey(0,0,0);
    states.clear();

    stateMap[trioID.tri]=0; // add the initial state
    push_back(states);
    if ( namedStates ) {
        stateNames.clear();
        stateNames.add(namer.make(0,0,0),0);
        named_states=true;
    } else {
        named_states=false;
    }
    queue.push(trioID);

    List<HalfArc> *matches;

    if ( preserveGroups) {       // use simpler 2 state filter since e transitions cannot be merged anyhow
        /* 2 state filter:
           0->0 : a:c from a:b (in l) and b:c (in r), incl. b=*e*
           1->0 : a:c from a:b (in l) and b:c (in r), b!=*e*
           0->1 or 1->1 : *e*:c from *e*:c (in r)
         */
        //FIXME: -a ... kbest paths look nothing like non -a.  find the bug!
        HashTable<HalfArcState, int> arcStateMap(2 * (a.numStates() +b.numStates())); // of course you may need 2*a*b+k states; this is just to get a larger initial table
        // a mediate state has a name like: bstate,"m"->astate, where "m" is a letter in the interface (output of a, input of b)
        while ( queue.notEmpty() ) {
            sourceState = queue.top().num;
            triSource = queue.top().tri;
            queue.pop();
            State *aState = &a.states[triSource.aState], *bState = &b.states[triSource.bState];
            aState->indexBy(1);
            bState->indexBy(0);
            const HashTable<IntKey, List<HalfArc> > &aindex=*aState->index;
            for ( HashTable<IntKey, List<HalfArc> >::const_iterator ll=aindex.begin() ; ll != aindex.end(); ++ll ) {
                HalfArcState mediate;
                mediate.l_hiddenLetter = ll->first;
                mediate.r_source = triSource.bState;
                if (  ll->first == EMPTY ) {
                    if ( triSource.filter == 0 ) {
                        out = EMPTY;
                        triDest.filter = 0;
                        triDest.bState = triSource.bState;
                        for ( List<HalfArc>::const_iterator l =ll->second.const_begin(),end=ll->second.const_end() ; l != end ; ++l ) {
                            HalfArc const& la=*l; // arc from a
                            weight = la->weight;
                            triDest.aState = la->dest;
                            in = la->in;
                            COMPOSEARC_GROUP(cascade.record1(la));
                        }
                    }
                } else if ( (matches = find_second(*bState->index,(IntKey)map[mediate.l_hiddenLetter])) ) {
                    for ( List<HalfArc>::const_iterator l =ll->second.const_begin(),end=ll->second.const_end() ; l != end ; ++l ) {
                        HalfArc const& la=*l;
                        mediate.l_dest = la->dest;
                        int mediateState=numStates();
                        typedef HashTable<HalfArcState, int> HAT;
                        hash_traits<HAT>::insert_return_type ins;
                        if ( (ins = arcStateMap.insert(HAT::value_type(mediate,mediateState))).second ) {
// populate new mediateState
                            push_back(states);
                            if ( namedStates )
                                stateNames.add(namer.make_mediate(mediate.l_dest,mediate.r_source,mediate.l_hiddenLetter),mediateState);
                            int temp = sourceState;
                            {
                                sourceState = mediateState;
                                triDest.aState = mediate.l_dest;
                                in = EMPTY;
                                triDest.filter = 0;
                                for ( List<HalfArc>::const_iterator r=matches->const_begin(),end = matches->const_end(); r!=end ; ++r ) {
                                    HalfArc const& ra=*r;  //arc from b
                                    Assert ( map[la->out] == ra->in );
                                    out = ra->out;
                                    triDest.bState = ra->dest;
                                    weight = ra->weight;
                                    COMPOSEARC_GROUP(cascade.record2(ra));
                                }
                            }
                            sourceState = temp;
                        } else {
                            mediateState = ins.first->second;
                        }
                        states[sourceState].addArc(FSTArc(la->in, EMPTY, mediateState, la->weight,
                                                          cascade.record1(la))); //arc from a
                    }
                }
            }
            if ( (matches = find_second(*bState->index,(IntKey)EMPTY)) ) {
                in = EMPTY;
                triDest.aState = triSource.aState;
                triDest.filter = 1;
                for ( List<HalfArc>::const_iterator r=matches->const_begin(),end = matches->const_end(); r!=end ; ++r ) {
                    HalfArc const& ra=*r;
                    Assert ( ra->in == EMPTY );
                    out = ra->out;
                    weight = ra->weight;
                    triDest.bState = ra->dest; // arc from b
                    COMPOSEARC_GROUP(cascade.record2(ra));
                }
            }
        }

    } else {
        // 3 state filter:
/* composing l.r
   filter state:
  0->0 : a:c from a:b in l, b:c in r, incl. b=*e*
  0->1 or 1->1 : a:*e* from a:*e* in l
  0->2 or 2->2 : *e*:c from *e*:c in r
  2->0 or 1->0 : a:c from a:b (l) b:c (r) where b != *e*
 */

        while ( queue.notEmpty() ) {
            sourceState = queue.top().num;
            triSource = queue.top().tri;
            queue.pop();
            State  *smaller, *larger;
            State *aState = &a.states[triSource.aState], *bState = &b.states[triSource.bState];
            if ( aState->size > bState->size ) {
                larger = aState;
                smaller = bState;
            } else {
                larger = bState;
                smaller = aState;
            }
            if ( larger->size > WFST::indexThreshold ) {
                larger->indexBy( larger == aState ); // create hash table
                if ( larger == bState ) {       // bState (rhs transducer) is larger
                    for ( List<FSTArc>::const_iterator l=aState->arcs.const_begin(),end=aState->arcs.const_end()
                              ; l !=end ; ++l) {
                        in = l->in;
                        triDest.aState = l->dest;
                        if ( l->out == EMPTY ) {
                            if ( triSource.filter != 2 ) {
                                out = EMPTY;
                                weight = l->weight;
                                triDest.filter = 1;
                                triDest.bState = triSource.bState;
                                COMPOSEARC_GROUP(cascade.record1(&*l));
                            }
                            if ( triSource.filter == 0 )
                                if ( (matches = find_second(*bState->index,(IntKey)EMPTY)) ) {
                                    triDest.filter = 0;
                                    for ( List<HalfArc>::const_iterator r=matches->const_begin(),end = matches->const_end()
                                              ; r!=end ; ++r ) {
                                        Assert((*r)->in == EMPTY);
                                        out = (*r)->out;
                                        weight = l->weight * (*r)->weight;
                                        triDest.bState = (*r)->dest;
                                        COMPOSEARC_GROUP(cascade.record(&*l,*r));
                                    }
                                }
                        } else {
                            if ( (matches = find_second(*bState->index,(IntKey)map[l->out])) ) {
                                triDest.filter = 0;
                                for ( List<HalfArc>::const_iterator r=matches->const_begin(),end = matches->const_end()
                                          ; r!=end ; ++r ) {
                                    Assert ( map[l->out] == (*r)->in );
                                    out = (*r)->out; //FIXME: uninit
                                    weight = l->weight * (*r)->weight;
                                    triDest.bState = (*r)->dest;
                                    COMPOSEARC_GROUP(cascade.record(&*l,*r));
                                }
                            }
                        }
                    }
                    if ( triSource.filter != 1 && (matches = find_second(*bState->index,(IntKey)EMPTY)) ) {
                        in = EMPTY;
                        triDest.aState = triSource.aState;
                        triDest.filter = 2;
                        for ( List<HalfArc>::const_iterator r=matches->const_begin(),end = matches->const_end(); r!=end ; ++r ) {
                            Assert ( (*r)->in == EMPTY );
                            out = (*r)->out;
                            weight = (*r)->weight;
                            triDest.bState = (*r)->dest;
                            COMPOSEARC_GROUP(cascade.record2(*r));
                        }
                    }
                } else {                        // aState (lhs transducer) is larger
                    //FIXME: total duplicated code from above case, except switching order of in/out.  a macro could factor this w/ no runtime cost
                    for ( List<FSTArc>::const_iterator r=bState->arcs.const_begin(),end = bState->arcs.const_end() ; r!=end  ; ++r) {
                        out = r->out;
                        triDest.bState = r->dest;
                        if ( r->in == EMPTY ) {
                            if ( triSource.filter != 1 ) {
                                in = EMPTY;
                                weight = r->weight;
                                triDest.filter = 2;
                                triDest.aState = triSource.aState;
                                COMPOSEARC_GROUP(cascade.record2(&*r));
                            }
                            if ( triSource.filter == 0 )
                                if ( (matches = find_second(*aState->index,(IntKey)EMPTY)) ) {
                                    triDest.filter = 0;
                                    for ( List<HalfArc>::const_iterator l=matches->const_begin(),end = matches->const_end() ; l != end ; ++l ) {
                                        Assert((*l)->out == EMPTY);
                                        in = (*l)->in;
                                        weight = (*l)->weight * r->weight;
                                        triDest.aState = (*l)->dest;
                                        COMPOSEARC_GROUP(cascade.record(*l,&*r));
                                    }
                                }
                        } else {
                            triDest.filter = 0;
                            if ( (matches = find_second(*aState->index,(IntKey)revMap[r->in])) ) {
                                for ( List<HalfArc>::const_iterator l=matches->const_begin(),end = matches->const_end() ; l != end ; ++l ) {
                                    Assert ( map[(*l)->out] == r->in );
                                    in = (*l)->in;
                                    weight = (*l)->weight * r->weight;
                                    triDest.aState = (*l)->dest;
                                    COMPOSEARC_GROUP(cascade.record(*l,&*r));
                                }
                            }
                        }
                    }
                    if ( triSource.filter != 2 && (matches = find_second(*aState->index,(IntKey)EMPTY)) ) {
                        out = EMPTY;
                        triDest.bState = triSource.bState;
                        triDest.filter = 1;
                        for ( List<HalfArc>::const_iterator l=matches->const_begin(),end = matches->const_end() ; l != end ; ++l ) {
                            Assert ( (*l)->out == EMPTY );
                            in = (*l)->in;
                            weight = (*l)->weight;
                            triDest.aState = (*l)->dest;
                            COMPOSEARC_GROUP(cascade.record1(*l));
                        }
                    }
                }
            } else {                  // both states too small to bother hashing
                for ( List<FSTArc>::const_iterator l=aState->arcs.const_begin(),end = aState->arcs.const_end() ; l !=end  ; ++l) {
                    in = l->in;
                    triDest.aState = l->dest;
                    if ( l->out == EMPTY ) {
                        if ( triSource.filter != 2 ) {
                            out = EMPTY;
                            weight = l->weight;
                            triDest.filter = 1;
                            triDest.bState = triSource.bState;
                            COMPOSEARC_GROUP(cascade.record1(&*l));
                        }
                        if ( triSource.filter == 0 ){
                            for ( List<FSTArc>::const_iterator r=bState->arcs.const_begin(),end = bState->arcs.const_end() ; r !=end ; ++r ) {
                                if ( r->in == EMPTY ) {
                                    out = r->out;
                                    weight = l->weight * r->weight;
                                    triDest.bState = r->dest;
                                    triDest.filter = 0;
                                    COMPOSEARC_GROUP(cascade.record(&*l,&*r));
                                }
                            }
                        }
                    } else {
                        triDest.filter = 0;
                        for ( List<FSTArc>::const_iterator r=bState->arcs.const_begin(),end = bState->arcs.const_end() ; r !=end ; ++r ) {
                            if ( map[l->out] == r->in ) {
                                out = r->out;
                                weight = l->weight * r->weight;
                                triDest.bState = r->dest;
                                COMPOSEARC_GROUP(cascade.record(&*l,&*r));
                            }
                        }
                    }
                }
                if ( triSource.filter != 1 ) {
                    in = EMPTY;
                    triDest.aState = triSource.aState;
                    triDest.filter = 2;
                    for ( List<FSTArc>::const_iterator r=bState->arcs.const_begin(),end = bState->arcs.const_end() ; r !=end ; ++r ) {
                        if ( r->in == EMPTY ) {
                            out = r->out;
                            weight = r->weight;
                            triDest.bState = r->dest;
                            COMPOSEARC_GROUP(cascade.record2(&*r));
                        }
                    }
                }
            }
        }
    }
    delete[] map;
    delete[] revMap;

    triDest.aState = a.final;
    triDest.bState = b.final;
    int *pFinal[3];
    int nFinal = 0;
    int i;
    for ( i = 0 ; i < 3 ; ++i ) {
        triDest.filter = i;
        if ( (pFinal[i] = find_second(stateMap,triDest)) ) {
            ++nFinal;
            final = *pFinal[i];
        }
    }
    if ( nFinal == 0 ) {
        invalidate();
        return;
    }
    if ( nFinal > 1 ) {
        final = numStates();
        push_back(states);
        if ( namedStates )
            stateNames.add("final",final);
        for ( i = 0 ; i < 3 ; ++i )
            if ( pFinal[i] ) {
                State &s=states[*pFinal[i]];
                s.addArc(FSTArc(EMPTY, EMPTY, final, 1.0,cascade.locked_1_groupid())); // prevent weight from changing in training
            }
    }
    states.resize(states.size());
}

}
