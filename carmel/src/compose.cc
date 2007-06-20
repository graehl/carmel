#include <graehl/shared/config.h>
#include <graehl/carmel/src/compose.h>
#include <graehl/carmel/src/fst.h>
#include <graehl/shared/array.hpp>

namespace graehl {

int WFST::indexThreshold = 12;
unsigned int TrioKey::aMax = 0;
unsigned int TrioKey::bMax = 0;

struct TrioNamer {
    char *buf;
    const WFST &a;
    const WFST &b;
    char *limit;
    TrioNamer(char *buf_,unsigned capacity,const WFST &a_,const WFST &b_) : buf(buf_),a(a_),b(b_) {
        limit=buf+capacity-6; // shoudl be 4: end of string, | <filter-state> |.  but 6 to be safe.
        Assert(limit>buf);
    }
    void append_safe(char *&p,char const* str) 
    {
        while ( *str && p<limit )
            *p++ = *str++;
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
        return buf;
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
        return buf;
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



WFST::WFST(WFST &a, WFST &b, bool namedStates, bool preserveGroups) : ownerIn(0), ownerOut(0), in(a.in), out(b.out), states((a.numStates() + b.numStates())), trn(NULL)
{
    const int EMPTY=epsilon_index;
    if ( !(a.valid() && b.valid()) ) {
        invalidate();
        return;
    }
    int *map = NEW int[a.out->size()];
    int *revMap = NEW int[b.in->size()];
    Assert(a.out->verify());
    Assert(b.in->verify());
    char buf[MAX_STATENAME_LEN+1];
    TrioNamer namer(buf,MAX_STATENAME_LEN+1,a,b);
    a.out->mapTo(*b.in, map);     // find matching symbols in interfacing alphabet
    b.in->mapTo(*a.out, revMap);
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
        namer.make(0,0,0);
        stateNames.add(buf,0);
        named_states=true;
    } else {
        named_states=false;
    }
    queue.push(trioID);

    List<HalfArc> *matches;

    if ( preserveGroups ) {       // use simpler 2 state filter since e transitions cannot be merged anyhow
        //FIXME: -a ... kbest paths look nothing like non -a.  find the bug!
        HashTable<HalfArcState, int> arcStateMap(2 * (a.numStates() +b.numStates()));
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
                            HalfArc const& la=*l;
                            weight = la->weight;
                            triDest.aState = la->dest;
                            in = la->in;
                            COMPOSEARC_GROUP(la->groupId);
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
                                    HalfArc const& ra=*r;
                                    Assert ( map[la->out] == ra->in );
                                    out = ra->out;
                                    triDest.bState = ra->dest;
                                    weight = ra->weight;
                                    COMPOSEARC_GROUP(ra->groupId);
                                }
                            }
                            sourceState = temp;
                            
                        } else {
                            mediateState = ins.first->second;                            
                        }
                        
                        states[sourceState].addArc(FSTArc(la->in, EMPTY, mediateState, la->weight,la->groupId));
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
                    triDest.bState = ra->dest;
                    COMPOSEARC_GROUP(ra->groupId);
                }
            }
        }

    } else {
        // 3 state filter; results in fewer epsilons (merge a:*e* composed with *e*:z into a single a:z in some cases?

        
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
                                COMPOSEARC;
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
                                        COMPOSEARC;
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
                                    COMPOSEARC;
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
                            COMPOSEARC;
                        }
                    }
                } else {                        // aState (lhs transducer) is larger
                    for ( List<FSTArc>::const_iterator r=bState->arcs.const_begin(),end = bState->arcs.const_end() ; r!=end  ; ++r) {
                        out = r->out;
                        triDest.bState = r->dest;
                        if ( r->in == EMPTY ) {
                            if ( triSource.filter != 1 ) {
                                in = EMPTY;
                                weight = r->weight;
                                triDest.filter = 2;
                                triDest.aState = triSource.aState;
                                COMPOSEARC;
                            }
                            if ( triSource.filter == 0 )
                                if ( (matches = find_second(*aState->index,(IntKey)EMPTY)) ) {
                                    triDest.filter = 0;
                                    for ( List<HalfArc>::const_iterator l=matches->const_begin(),end = matches->const_end() ; l != end ; ++l ) {
                                        Assert((*l)->out == EMPTY);
                                        in = (*l)->in;
                                        weight = (*l)->weight * r->weight;
                                        triDest.aState = (*l)->dest;
                                        COMPOSEARC;
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
                                    COMPOSEARC;
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
                            COMPOSEARC;
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
                            COMPOSEARC;
                        }
                        if ( triSource.filter == 0 ){
                            for ( List<FSTArc>::const_iterator r=bState->arcs.const_begin(),end = bState->arcs.const_end() ; r !=end ; ++r ) {
                                if ( r->in == EMPTY ) {
                                    out = r->out;
                                    weight = l->weight * r->weight;
                                    triDest.bState = r->dest;
                                    triDest.filter = 0;
                                    COMPOSEARC;
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
                                COMPOSEARC;
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
                            COMPOSEARC;
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
                states[*pFinal[i]].addArc(FSTArc(EMPTY, EMPTY, final, 1.0));
                states[*pFinal[i]].arcs.top().groupId = WFST::locked_group; // prevent weight from changing in training
            }
    }
    states.resize(states.size());
}

}
