#ifndef ARC_H
#define ARC_H 1
#include "config.h"
#include "weight.h"
#include "strhash.h"


struct Arc {
  int in;
  int out;
  int dest;
  Weight weight;
  int groupId;  
  Arc(int i, int o, int d, Weight w,int g = -1) : 
    in(i), out(o), dest(d), weight(w), groupId(g){}
};


std::ostream & operator << (std::ostream &out,const Arc &a); // Yaser 7-20-2000

typedef Arc *HalfArc;

						  
struct UnArc {
  int in;
  int out;
  int dest;
  bool operator == (const UnArc& r) const {
    return in == r.in && out == r.out && dest == r.dest;
  }
  int hash() const
  {
    return (in * 235479241 + out * 67913 + dest) * 2654435767U;
  }
};
HASHNS_B
template<>
struct hash<UnArc>
{
  size_t operator()(const UnArc &x) const {
	return x.hash();
  }
};
HASHNS_E
#endif 
