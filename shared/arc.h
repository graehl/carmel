#ifndef ARC_H
#define ARC_H

#include "config.h"
#include "weight.h"
#include "2hash.h"
#include <iostream>

struct FSTArc {
  int in;
  int out;
  int dest;
  Weight weight;
  int groupId;
  FSTArc(int i, int o, int d, Weight w,int g = -1) :
    in(i), out(o), dest(d), weight(w), groupId(g)
    {}
};

inline
std::ostream & operator << (std::ostream &out,const FSTArc &a) {
  out << '(' << a.dest << ' ' << a.in << ' ' << a.out << ' ' << a.weight << ')';
  return(out);
}

typedef FSTArc *HalfArc;

struct UnArc {
  int in;
  int out;
  int dest;
  bool operator == (const UnArc& r) const {
    return in == r.in && out == r.out && dest == r.dest;
  }
  size_t hash() const
  {
    return uint_hash((in * 193 + out * 6151 + dest));
  }
};

/*
HASHNS_B
template<>
struct hash<UnArc>
{
  size_t operator()(const UnArc &x) const {
    return x.hash();
  }
};
HASHNS_E
*/

BEGIN_HASH(UnArc) {
  return x.hash();
} END_HASH
#endif
