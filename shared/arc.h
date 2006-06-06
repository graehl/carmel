#ifndef ARC_H
#define ARC_H

#include <graehl/shared/config.h>
#include <graehl/shared/weight.h>
#include <graehl/shared/2hash.h>
#include <graehl/shared/stream_util.hpp>

struct FSTArc {
  int in;
  int out;
  int dest;
  Weight weight;
  int groupId;
  static const int NOGROUP=-1;  
  static const int LOCKEDGROUP=0;
  FSTArc(int i, int o, int d, Weight w,int g = NOGROUP) :
    in(i), out(o), dest(d), weight(w), groupId(g)
    {}
    bool isNormal() const {
        //return groupId == NOGROUP;
        return groupId < 0;
    }
    bool isLocked() const {
        return groupId == LOCKEDGROUP;
    }
    bool isTied() const {
        return groupId > 0;
    }
    bool isTiedOrLocked() const {
        return groupId >= 0;
    }
    void setLocked() 
    {
        setGroup(LOCKEDGROUP);
    }
    void setGroup(int group) 
    {
        groupId = group;
    }
    template <class O> void print(O&o) const
    {
        o<<'(' << a.dest << ' ' << a.in << ' ' << a.out << ' ' << a.weight << ')';
    }
    TO_OSTREAM_PRINT
        
};

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
