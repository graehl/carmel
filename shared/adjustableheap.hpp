#ifndef _ADJUSTABLEHEAP_HPP
#define _ADJUSTABLEHEAP_HPP

#include "2heap.h"
#include "threadlocal.hpp"
//! NOTE: key to only SetLocWeight according to a stack discipline - cannot leave multiple instances around using heap allocation!

//! NOTE: default comparison direction is reversed ... making max-heaps into min-heaps (as desired for best-tree) and vice versa
// loc[key] = void * (or DistToState<K,W,L> *)
// weight[key] = some weight class
template <class K,class W,class L>
struct HeapKey {
  typedef K value_type;
  typedef W weightmap_type;
  typedef L locmap_type;
  typedef HeapKey<K,W,L> Self;
  typedef Self *Loc;
  typedef typename W::value_type weight_type;
  K key;
  static THREADLOCAL L loc;
  static THREADLOCAL W weight;
  struct SetLocWeight {
    L old_loc;
    W old_weight;
    SetLocWeight(L l,W w) : old_loc(loc),old_weight(weight) {
      loc=l;
      weight=w;
    }
    ~SetLocWeight() {
      loc=old_loc;
      weight=old_weight;
    }
  };
  HeapKey() : key() {}
  HeapKey(K k) : key(k) {}
  HeapKey(Self s) : key(s.k) {}

  Self *loc() const {
    return loc[key];
  }
  weight_type &weight() {
    return weight[key];
  }
//  static FLOAT_TYPE unreachable;
  //operator weight_type() const { return weight[key]; }
  //operator K () const { return key; }
  void operator = (HeapKey<K,W,L> rhs) { 
    loc[rhs.key] = this;
    loc = rhs.loc;
  }
};

//!! Assumes never adjusted after heapPop and loc initialized to 0 (NULL) until first heapAdd
// this is really ok because only operator = sets loc; passing/constructing/copying doesn't
template <typename Heap,K,W,L> 
inline void heapAdjustOrAdd ( Heap &heap, HeapKey<K,W,L> k) {
  typename Heap::iterator heapLoc=k.loc();
  if (heapLoc)
    heapAdjustUp(heap.begin(),heapLoc);
  else
    heapAdd(heap,k);
}

#ifdef MAIN
template<class K,W,L>
THREADLOCAL typename HeapKey<K,W,L>::weightmap_type HeapKey<K,W,L>::weight;

template<class K,W,L>
THREADLOCAL typename HeapKey<K,W,L>::locmap_type HeapKey<K,W,L>::loc;
#endif

template<class K,W,L>
inline bool operator < (HeapKey<K,W,L> lhs, HeapKey<K,W,L> rhs) {
  return lhs.weight() > rhs.weight();
}

/*
template<class K,W,L>
 inline bool operator == (HeapKey<K,W,L> lhs, HeapKey<K,W,L> rhs) {
  return HeapKey<K,W,L>::weight[lhs.key] == HeapKey<K,W,L>::weight[rhs.key];
}

template<class K,W,L>
inline bool operator == (HeapKey<K,W,L> lhs, K rhs) {
  return HeapKey<K,W,L>::weight[lhs.key] == rhs;
}
*/



#ifdef TEST
#include "test.hpp"
#endif

#ifdef TEST
BOOST_AUTO_UNIT_TEST( ADJUSTABLEHEAP )
{
}
#endif

#endif
