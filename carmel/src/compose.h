#ifndef GRAEHL_CARMEL_COMPOSE_H
#define GRAEHL_CARMEL_COMPOSE_H

#include <graehl/shared/myassert.h>
#include <graehl/shared/2hash.h>


namespace graehl {

struct TrioKey {
  static unsigned gAStates;
  static unsigned gBStates;
  unsigned qa;
  unsigned qb;
  char filter;

  bool operator==(const TrioKey& t) const { return (qa == t.qa) && (qb == t.qb) && (filter == t.filter); }

  TrioKey() {}

  TrioKey(unsigned a, unsigned b, char c) : qa(a), qb(b), filter(c) {}
  size_t hash() const {
    Assert(qa < gAStates && qb < gBStates);
    return uint32_hash((gBStates * (filter * gAStates + qa) + qb));
  }
};


struct HalfArcState {
  unsigned l_dest;  // in unshared (lhs) transducer
  unsigned r_source;  // in (rhs) transducer with the shared arcs
  unsigned l_hiddenLetter;  // the matching letter that disappears in composition (index unsignedo lhs
                            // transducer's output alphabet)

  bool operator==(const HalfArcState& s) const {
    return (l_dest == s.l_dest) && (r_source == s.r_source) && (l_hiddenLetter == s.l_hiddenLetter);
  }

  HalfArcState() {}

  HalfArcState(unsigned a, unsigned b, unsigned c) : l_dest(a), r_source(b), l_hiddenLetter(c) {}
  size_t hash() const {
    return uint32_hash(TrioKey::gBStates * (l_hiddenLetter * TrioKey::gAStates + l_dest) + r_source);
  }
};

struct TrioID {
  unsigned num;
  TrioKey tri;
};
}

BEGIN_HASH(graehl::TrioKey) {
  return x.hash();
}
END_HASH

BEGIN_HASH(graehl::HalfArcState) {
  return x.hash();
}
END_HASH


#endif
