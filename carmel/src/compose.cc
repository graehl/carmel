/*******************************************************************************
* This software ("Carmel") is licensed for research use only, as described in  *
* the LICENSE file in this distribution.  The software can be downloaded from  *
* http://www.isi.edu/natural-language/licenses/carmel-license.html.  Please    *
* contact Yaser Al-Onaizan (yaser@isi.edu) or Kevin Knight (knight@isi.edu)    *
* with questions about the software or commercial licensing.  All software is  *
* copyrighted C 2000 by the University of Southern California.                 *
*******************************************************************************/

#include "compose.h"
#include "fst.h"

static void makeTrioName(char *bufP, const char *aName, const char *bName, int filter)
{
  while ( *aName )
    *bufP++ = *aName++;
  *bufP++ = '|';
  *bufP++ = filter + '0';
  *bufP++ = '|';
  while ( *bName )
    *bufP++ = *bName++;
  *bufP = 0;
}

#define COMPOSEARC { if ( (pDest = stateMap.find(triDest)) ) { states[sourceState].addArc(Arc(in, out, *pDest, weight)); } else { stateMap.add(triDest, numStates()); trioID.num = numStates(); trioID.tri = triDest;    queue.push(trioID);    states[sourceState].addArc(Arc(in, out, trioID.num, weight));    states.pushBack();    if ( namedStates ) {      makeTrioName(buf, a.stateNames[triDest.aState], b.stateNames[triDest.bState], triDest.filter);      stateNames.add(buf);    }  }}



WFST::WFST(WFST &a, WFST &b, bool namedStates, bool preserveGroups) : ownerInOut(0), in(a.in), out(b.out), states((a.numStates() + b.numStates())), trn(NULL)
{
  if ( !(a.valid() && b.valid()) ) {
    invalidate();
    return;
  }
  int *map = new int[a.out->count()];
  int *revMap = new int[b.in->count()];
  char buf[8096];
  a.out->mapTo(*b.in, map);	// find matching symbols in interfacing alphabet
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
  }
  queue.push(trioID);

  List<HalfArc> *matches;

  if ( preserveGroups ) {	// use simpler 2 state filter since e transitions cannot be merged anyhow
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
	    List<HalfArc>::const_iterator end = ll.val().end() ;	    
	    for ( List<HalfArc>::const_iterator l =ll.val().begin() ; l != end ; ++l ) {
	      weight = (*l)->weight;
	      triDest.aState = (*l)->dest;
	      in = (*l)->in;
	      COMPOSEARC;
	      if ( (group = (*l)->groupId) >= 0)
		states[sourceState].arcs.top().groupId = group;
	    }
	  }
	} else if ( (matches = bState->index->find(map[mediate.hiddenLetter])) ) {
	  List<HalfArc>::const_iterator end = ll.val().end() ;
	  
	  for ( List<HalfArc>::const_iterator l =ll.val().begin() ;l!=end  ; ++l ) {
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
	      List<HalfArc>::const_iterator end = matches->end();	      
	      for ( List<HalfArc>::const_iterator r=matches->begin() ; r!=end ; ++r ) {
		Assert ( map[l->out] == (*r)->in );
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
	List<HalfArc>::const_iterator end = matches->end() ;	
	for ( List<HalfArc>::const_iterator r=matches->begin() ; r != end; ++r ) {
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
	if ( larger == bState ) {	// bState (rhs transducer) is larger
	  List<Arc>::const_iterator end = aState->arcs.end() ;	  
	  for ( List<Arc>::const_iterator l=aState->arcs.begin() ; l !=end ; ++l) {
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
		  List<HalfArc>::const_iterator end = matches->end();
		  
		  for ( List<HalfArc>::const_iterator r= matches->begin() ; r != end ; ++r ) {
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
		List<HalfArc>::const_iterator end = matches->end();
		
		for ( List<HalfArc>::const_iterator r = matches->begin() ; r !=end ; ++r ) {
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
	    List<HalfArc>::const_iterator end = matches->end() ;
	    
	    for ( List<HalfArc>::const_iterator r=matches->begin() ; r != end ; ++r ) {
	      Assert ( (*r)->in == EMPTY );
	      out = (*r)->out;
	      weight = (*r)->weight;
	      triDest.bState = (*r)->dest;
	      COMPOSEARC; 
	    }
	  }
	} else {			// aState (lhs transducer) is larger
	  List<Arc>::const_iterator end = bState->arcs.end() ;	  
	  for ( List<Arc>::const_iterator r=bState->arcs.begin() ; r!=end  ; ++r) {
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
		  List<HalfArc>::const_iterator end = matches->end();
		  for ( List<HalfArc>::const_iterator l=matches->begin() ; l != end ; ++l ) {
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
		List<HalfArc>::const_iterator end =  matches->end() ;
		
		for ( List<HalfArc>::const_iterator l=matches->begin() ; l !=end; ++l ) {
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
	    List<HalfArc>::const_iterator  end=matches->end();
	    
	    for ( List<HalfArc>::const_iterator l=matches->begin() ; l !=end; ++l ) {
	      Assert ( (*l)->out == EMPTY );
	      in = (*l)->in;
	      weight = (*l)->weight;
	      triDest.aState = (*l)->dest;
	      COMPOSEARC; 
	    }
	  }
	}
      } else {			// both states too small to bother hashing
	List<Arc>::const_iterator end = aState->arcs.end() ;
	
	for ( List<Arc>::const_iterator l=aState->arcs.begin() ; l !=end  ; ++l) {
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
	      List<Arc>::const_iterator end = bState->arcs.end() ;	      
	      for ( List<Arc>::const_iterator r=bState->arcs.begin() ; r !=end ; ++r ) {
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
	    List<Arc>::const_iterator end =  bState->arcs.end() ;
	    for ( List<Arc>::const_iterator r=bState->arcs.begin() ; r!= end ; ++r ) {
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
	  List<Arc>::const_iterator end = bState->arcs.end() ; 
	  for ( List<Arc>::const_iterator r=bState->arcs.begin() ; r != end ; ++r ) {
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
	states[*pFinal[i]].arcs.top().groupId = 0; // prevent weight from changing in training
      }
  }
  states.resize(states.count());
}
