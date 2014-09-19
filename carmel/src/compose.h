#ifndef GRAEHL_CARMEL_COMPOSE_H
#define GRAEHL_CARMEL_COMPOSE_H
#include <graehl/shared/myassert.h>

#include <graehl/shared/2hash.h>



namespace graehl {

struct TrioKey {
  static unsigned int gAStates;
  static unsigned int gBStates;
  unsigned int qa;
  unsigned int qb;
  char filter;

  bool operator == (const TrioKey &t) const
  {
    return (qa == t.qa) && (qb == t.qb)
      && (filter == t.filter);
  }

  TrioKey() {}

  TrioKey(int a, int b, char c) :
    qa(a), qb(b), filter(c) {}
  size_t hash() const
  {
    Assert ( qa < gAStates && qb < gBStates);
  return uint32_hash((gBStates * (filter*gAStates + qa) + qb));
  }
};


struct HalfArcState {
  int l_dest;     // in unshared (lhs) transducer
  int r_source;     // in (rhs) transducer with the shared arcs
  int l_hiddenLetter;   // the matching letter that disappears in composition (index into lhs transducer's output alphabet)

  bool operator == ( const HalfArcState &s ) const
  {
    return ( l_dest == s.l_dest )
      && ( r_source == s.r_source )
      && ( l_hiddenLetter == s.l_hiddenLetter );
  }

    HalfArcState() {}

  HalfArcState(int a, int b, int c) :
    l_dest(a), r_source(b), l_hiddenLetter(c) {}
  size_t hash() const
  {
    return uint32_hash(TrioKey::gBStates*(l_hiddenLetter*TrioKey::gAStates + l_dest) + r_source);
  }
};

struct TrioID {
  int num;
  TrioKey tri;
};

}

BEGIN_HASH(graehl::TrioKey) {
  return x.hash();
  } END_HASH

  BEGIN_HASH(graehl::HalfArcState) {
      return x.hash();
  } END_HASH


#endif
