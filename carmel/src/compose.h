#ifndef COMPOSE_H
#define COMPOSE_H 1
#include "assert.h"



#define EMPTY 0

struct TrioKey {
  static int aMax;
  static int bMax;
  int aState;
  int bState;
  char filter;
  int operator == (const TrioKey &t) const 
  {
    return (aState == t.aState) && (bState == t.bState)
      && (filter == t.filter);
  }
  TrioKey() {}
  TrioKey(int a, int b, char c) : 
    aState(a), bState(b), filter(c) {}
  int hash() const
  {
    Assert ( aState < aMax && bState < bMax);
    return (bMax * (filter*aMax + aState) + bState) * 2654435767U;
  }
};

struct HalfArcState {
  int dest;			// in unshared (lhs) transducer
  int source;			// in (rhs) transducer with the shared arcs
  int hiddenLetter;		// the matching letter that disappears in composition (index into lhs transducer's output alphabet)
  int operator == ( const HalfArcState &s ) const
  {
    return ( dest == s.dest )
      && ( source == s.source )
      && ( hiddenLetter == s.hiddenLetter );
  }
  HalfArcState() {}
  HalfArcState(int a, int b, int c) :
    dest(a), source(b), hiddenLetter(c) {}
  int hash() const
  {
    return (TrioKey::bMax*(hiddenLetter*TrioKey::aMax + dest) + source) * 2654435767U;
  }
};


struct TrioID {
  int num;
  TrioKey tri;
};



#endif
