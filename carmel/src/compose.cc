#include "config.h"
#include "compose.h"
#include "fst.h"


static void makeTrioName(char *bufP, const char *aName, const char *bName, int filter)
{
  char *limit=bufP+MAX_STATENAME_LEN;
  while ( *aName )
    *bufP++ = *aName++;
  *bufP++ = '|';
  *bufP++ = filter + '0';
  *bufP++ = '|';
  while ( *bName && bufP < limit)
    *bufP++ = *bName++;
  *bufP = 0;
}

#define COMPOSEARC { if ( (pDest = stateMap.find(triDest)) ) { states[sourceState].addArc(Arc(in, out, *pDest, weight)); } else { stateMap.add(triDest, numStates()); trioID.num = numStates(); trioID.tri = triDest;    queue.push(trioID);    states[sourceState].addArc(Arc(in, out, trioID.num, weight));    states.pushBack();    if ( namedStates ) {      makeTrioName(buf, a.stateName(triDest.aState), b.stateName(triDest.bState), triDest.filter);      stateNames.add(buf);    }  }}



WFST::WFST(WFST &a, WFST &b, bool namedStates, bool preserveGroups) : ownerIn(0), ownerOut(0), in(a.in), out(b.out), states((a.numStates() + b.numStates())), trn(NULL)
{
  if ( !(a.valid() && b.valid()) ) {
    invalidate();
    return;
  }
  int *map = NEW int[a.out->count()];
  int *revMap = NEW int[b.in->count()];
  char buf[MAX_STATENAME_LEN+1];
  a.out->mapTo(*b.in, map);     // find matching symbols in interfacing alphabet
  b.in->mapTo(*a.out, revMap);
  TrioKey::aMax = a.numStates(); // used in hash function
  TrioKey::bMax = b.numStates();

  HashTable<TrioKey,int> stateMap(2 * (a.numStates() + b.numStates())); // assign state numbers
  // to composite states in the order they are first visited

  List<TrioID> queue;
  int sourceState = -1;
  int *pDest;
  int in, out;
  Weight weight;
  TrioKey triSource, triDest;
  TrioID trioID;
  trioID.num = 0;
  trioID.tri = TrioKey(0,0,0);

  stateMap.add(trioID.tri, 0); // add the initial state
  states.pushBack();
  if ( namedStates ) {
    makeTrioName(buf, a.stateNames[0], b.stateNames[0], 0);
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
      for ( HashIter<IntKey, List<HalfArc> > ll(*aState->index) ; ll ; ++ll ) {
        HalfArcState mediate;
        mediate.hiddenLetter = ll.key();
        mediate.source = triSource.bState;
        if (  ll.key() == EMPTY ) {
          if ( triSource.filter == 0 ) {
            out = EMPTY;
            triDest.filter = 0;
            triDest.bState = triSource.bState;
            for ( List<HalfArc>::const_iterator l =ll.val().const_begin(),end=ll.val().const_end() ; l != end ; ++l ) {
              weight = (*l)->weight;
              triDest.aState = (*l)->dest;
              in = (*l)->in;
              COMPOSEARC;
              //!danger: should probably use IsTiedOrLocked() in Arc.h.
              if ( (group = (*l)->groupId) >= 0 )
                states[sourceState].arcs.top().groupId = group;
            }
          }
        } else if ( (matches = bState->index->find(map[mediate.hiddenLetter])) ) {
          for ( List<HalfArc>::const_iterator l =ll.val().const_begin(),end=ll.val().const_end() ; l != end ; ++l ) {
            mediate.dest = (*l)->dest;
            int mediateState;
            if ( (pDest = arcStateMap.find(mediate)) ) {
              mediateState = *pDest;
            } else {
              mediateState = numStates();
              arcStateMap.add(mediate, mediateState);
              states.pushBack();
              if ( namedStates ) {
                char *p = buf;
                const char *s = b.stateNames[mediate.source],
                  *l = (*a.out)[mediate.hiddenLetter],
                  *d = a.stateNames[mediate.dest];
                while ( *s )
                  *p++ = *s++;
                *p++ = ',';
                while ( *l )
                  *p++ = *l++;
                *p++ = '-';
                *p++ = '>';
                while ( (*p++ = *d++) ) ;
                stateNames.add(buf);
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
                if ( (group = (*r)->groupId)>= 0)
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
      if ( (matches = bState->index->find(EMPTY)) ) {
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
      if ( larger->size * smaller->size > WFST::indexThreshold ) {
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
                if ( (matches = bState->index->find(EMPTY)) ) {
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
              if ( (matches = bState->index->find(map[l->out])) ) {
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
          if ( triSource.filter != 1 && (matches = bState->index->find(EMPTY)) ) {
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
                if ( (matches = aState->index->find(EMPTY)) ) {
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
              if ( (matches = aState->index->find(revMap[r->in])) ) {
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
          if ( triSource.filter != 2 && (matches = aState->index->find(EMPTY)) ) {
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
    if ( (pFinal[i] = stateMap.find(triDest)) ) {
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
    states.pushBack();
    if ( namedStates )
      stateNames.add("final");
    for ( i = 0 ; i < 3 ; ++i )
      if ( pFinal[i] ) {
        states[*pFinal[i]].addArc(Arc(EMPTY, EMPTY, final, 1.0));
        states[*pFinal[i]].arcs.top().groupId = WFST::LOCKEDGROUP; // prevent weight from changing in training
      }
  }
  states.resize(states.count());
}
