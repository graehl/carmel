#include "config.h"
#include "compose.h"
#include "fst.h"

int WFST::indexThreshold = 32;
unsigned int TrioKey::aMax = 0;
unsigned int TrioKey::bMax = 0;

struct TrioNamer {
  char *buf;
  const WFST &a;
  const WFST &b;
  char *limit;
  TrioNamer(char *buf_,unsigned capacity,const WFST &a_,const WFST &b_) : buf(buf_),a(a_),b(b_) {
	limit=buf+capacity-5;
	Assert(limit>buf);
  }
  void make(unsigned aState,unsigned bState,char filter) {
	char *bufP=buf;
	const char *aName=a.stateName(aState);
    while ( *aName && buf<limit )
	  *bufP++ = *aName++;
	*bufP++ = '|';
	*bufP++ = filter + '0';
	*bufP++ = '|';
	const char *bName=b.stateName(bState);
    while ( *bName && bufP < limit)
	  *bufP++ = *bName++;
	*bufP = 0;
  }
};

//#define OLDCOMPOSEARC
#ifdef DEBUGCOMPOSE
#define DUMPARC(a,b,c,d) Config::debug() << "arc" << Arc(a,b,c,d)
#else
#define DUMPARC(a,b,c,d)
#endif

#ifdef OLDCOMPOSEARC
#define COMPOSEARC do { \
if ( (pDest = find_second(stateMap,triDest)) ) \
 { states[sourceState].addArc(Arc(in, out, *pDest, weight)); \
} else { \
  add(stateMap,triDest,numStates()); trioID.num = numStates(); trioID.tri = triDest;    queue.push(trioID);    states[sourceState].addArc(Arc(in, out, trioID.num, weight));    states.push_back();    \
  if ( namedStates ) {      namer.make(triDest.aState, triDest.bState, triDest.filter);      stateNames.add(buf);   \
  }  }} while(0)
#else

#define COMPOSEARC do { \
  HashTable<TrioKey,int>::insert_return_type i; \
  if ( (i = stateMap.insert(HashTable<TrioKey,int>::value_type(triDest,numStates()))).second ) { \
	trioID.num=numStates();trioID.tri = triDest;queue.push(trioID);states.push_back();\
	if ( namedStates ) { namer.make(triDest.aState, triDest.bState, triDest.filter);\
	  stateNames.add(buf);   \
	}} else trioID.num=i.first->second; \
  states[sourceState].addArc(Arc(in, out, trioID.num, weight)); DUMPARC(in, out, trioID.num, weight);} while(0)
#endif


WFST::WFST(WFST &a, WFST &b, bool namedStates, bool preserveGroups) : ownerIn(0), ownerOut(0), in(a.in), out(b.out), states((a.numStates() + b.numStates())), trn(NULL)
{
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

  stateMap[trioID.tri]=0; // add the initial state
  states.push_back();
  if ( namedStates ) {
    namer.make(0,0,0);
    stateNames.add(buf);
    named_states=true;
  } else {
    named_states=false;
  }
  queue.push(trioID);

  List<HalfArc> *matches;

  if ( preserveGroups ) {       // use simpler 2 state filter since e transitions cannot be merged anyhow
    HashTable<HalfArcState, int> arcStateMap(2 * (a.numStates() +b.numStates()));
    while ( queue.notEmpty() ) {
      sourceState = queue.top().num;
      triSource = queue.top().tri;
      queue.pop();
      State *aState = &a.states[triSource.aState], *bState = &b.states[triSource.bState];
      aState->indexBy(1);
      bState->indexBy(0);
      int group;
	  const HashTable<IntKey, List<HalfArc> > &aindex=*aState->index;
	  for ( HashTable<IntKey, List<HalfArc> >::const_iterator ll=aindex.begin() ; ll != aindex.end(); ++ll ) {
        HalfArcState mediate;
        mediate.hiddenLetter = ll->first;
        mediate.source = triSource.bState;
        if (  ll->first == EMPTY ) {
          if ( triSource.filter == 0 ) {
            out = EMPTY;
            triDest.filter = 0;
            triDest.bState = triSource.bState;
            for ( List<HalfArc>::const_iterator l =ll->second.const_begin(),end=ll->second.const_end() ; l != end ; ++l ) {
              weight = (*l)->weight;
              triDest.aState = (*l)->dest;
              in = (*l)->in;
              COMPOSEARC;
              //!danger: should probably use IsTiedOrLocked() in Arc.h.
              if ( isTiedOrLocked(group = (*l)->groupId) )
                states[sourceState].arcs.top().groupId = group;
            }
          }
        } else if ( (matches = find_second(*bState->index,(IntKey)map[mediate.hiddenLetter])) ) {
          for ( List<HalfArc>::const_iterator l =ll->second.const_begin(),end=ll->second.const_end() ; l != end ; ++l ) {
            mediate.dest = (*l)->dest;
            int mediateState;
			HashTable<HalfArcState, int>::insert_return_type ins;
            if ( (ins = arcStateMap.insert(HashTable<HalfArcState, int>::value_type(mediate,numStates()))).second ) {			  
			  mediateState = numStates();
              states.push_back();
              if ( namedStates ) {
                char *p = buf;
				char *limit=namer.limit;
                const char *s = b.stateName(mediate.source),
                  *l = (*a.out)[mediate.hiddenLetter].c_str();
                while ( *s && p < limit)
                  *p++ = *s++;
                *p++ = ',';
                while ( *l && p < limit)
                  *p++ = *l++;
                *p++ = '-';
                *p++ = '>';
				const char *d= a.stateName(mediate.dest);
                while ( (*p++ = *d++) && p < limit) ;
                stateNames.add(buf);
            } else {
              mediateState = ins.first->second;
            }

              int temp = sourceState;
              sourceState = mediateState;
              triDest.aState = mediate.dest;
              in = EMPTY;
              triDest.filter = 0;
              for ( List<HalfArc>::const_iterator r=matches->const_begin(),end = matches->const_end(); r!=end ; ++r ) {
                Assert ( map[(*l)->out] == (*r)->in );
                out = (*r)->out;
                triDest.bState = (*r)->dest;
                weight = (*r)->weight;
                COMPOSEARC;
                if ( isTiedOrLocked(group = (*r)->groupId) )
                  states[sourceState].arcs.top().groupId = group;
              }
              sourceState = temp;
            }
            states[sourceState].addArc(Arc((*l)->in, EMPTY, mediateState, (*l)->weight));
            if ( (group = (*l)->groupId)>=0)
              states[sourceState].arcs.top().groupId = group;

          }
        }
      }
      if ( (matches = find_second(*bState->index,(IntKey)EMPTY)) ) {
        in = EMPTY;
        triDest.aState = triSource.aState;
        triDest.filter = 1;
        for ( List<HalfArc>::const_iterator r=matches->const_begin(),end = matches->const_end(); r!=end ; ++r ) {
          Assert ( (*r)->in == EMPTY );
          out = (*r)->out;
          weight = (*r)->weight;
          triDest.bState = (*r)->dest;
          COMPOSEARC;
          if ( (group = (*r)->groupId)>=0)
            states[sourceState].arcs.top().groupId = group;
        }
      }
    }
  } else {
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
          for ( List<Arc>::const_iterator l=aState->arcs.const_begin(),end=aState->arcs.const_end() ; l !=end ; ++l) {
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
                  for ( List<HalfArc>::const_iterator r=matches->const_begin(),end = matches->const_end(); r!=end ; ++r ) {
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
                for ( List<HalfArc>::const_iterator r=matches->const_begin(),end = matches->const_end(); r!=end ; ++r ) {
                  Assert ( map[l->out] == (*r)->in );
                  out = (*r)->out;
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
          for ( List<Arc>::const_iterator r=bState->arcs.const_begin(),end = bState->arcs.const_end() ; r!=end  ; ++r) {
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
        for ( List<Arc>::const_iterator l=aState->arcs.const_begin(),end = aState->arcs.const_end() ; l !=end  ; ++l) {
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
              for ( List<Arc>::const_iterator r=bState->arcs.const_begin(),end = bState->arcs.const_end() ; r !=end ; ++r ) {
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
            for ( List<Arc>::const_iterator r=bState->arcs.const_begin(),end = bState->arcs.const_end() ; r !=end ; ++r ) {
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
          for ( List<Arc>::const_iterator r=bState->arcs.const_begin(),end = bState->arcs.const_end() ; r !=end ; ++r ) {
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
    states.push_back();
    if ( namedStates )
      stateNames.add("final");
    for ( i = 0 ; i < 3 ; ++i )
      if ( pFinal[i] ) {
        states[*pFinal[i]].addArc(Arc(EMPTY, EMPTY, final, 1.0));
        states[*pFinal[i]].arcs.top().groupId = WFST::LOCKEDGROUP; // prevent weight from changing in training
      }
  }
  states.resize(states.size());
}
