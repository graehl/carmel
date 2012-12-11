// binary heap allowing adjusting the cost of a node not at the top
#ifndef ADJUSTABLEHEAP_HPP
#define ADJUSTABLEHEAP_HPP

#include <graehl/shared/2heap.h>
#include <graehl/shared/threadlocal.hpp>
#include <graehl/shared/byref.hpp>
#include <graehl/shared/dummy.hpp>

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#endif


namespace graehl {

//! NOTE: key to only SetLocWeight according to a stack discipline - cannot leave multiple instances around using heap allocation!

//! NOTE: default comparison direction is reversed ... making max-heaps into min-heaps (as desired for best-tree) and vice versa
// loc[key] = void * (or DistToState<K,W,L> *)
// weight[key] = some weight class
// ok to copy/construct HeapKey without SetLocWeight initializing maps, but when you go to compare,assign, they need to be set.
template <class K,class W,class L>
struct HeapKey {
  typedef K value_type;
  typedef W weightmap_type;
  typedef L locmap_type;
  typedef HeapKey<K,W,L> Self;
  typedef Self *Loc;
    typedef typename boost::unwrap_reference<W>::type::value_type weight_type;
  K key;
  static THREADLOCAL L locmap;
  static THREADLOCAL W weightmap;
  struct SetLocWeight {
    L old_loc;
    W old_weight;
    SetLocWeight(L l,W w) : old_loc(locmap),old_weight(weightmap) {
      locmap=l;
      weightmap=w;
    }
    ~SetLocWeight() {
      locmap=old_loc;
      weightmap=old_weight;
    }
  };
  HeapKey() : key() {}
  HeapKey(K k) : key(k) {}
  HeapKey(const Self &s) : key(s.key) {}

  Self *&location() {
    return *(Self **)&(deref(locmap)[key]);
  }
  weight_type &weight() {
    return deref(weightmap)[key];
  }
//  static FLOAT_TYPE unreachable;
  //operator weight_type() const { return weight[key]; }
  //operator K () const { return key; }
  void operator = (HeapKey<K,W,L> rhs) {
    //location()=NULL;
    key=rhs.key;
    location() = this;
  }

  template <typename Heap>
  inline void heapAdjustOrAdd (Heap &heap) {
    typename Heap::iterator heapLoc=location();
    if (heapLoc)
      heapAdjustUp(heap.begin(),heapLoc);
    else
      heapAdd(heap,*this);
  }

};

  //!! Assumes never adjusted after heapPop and loc initialized to 0 (NULL) until first heapAdd
// this is really ok because only operator = sets loc; passing/constructing/copying doesn't

//! not allowed to deduce nested template args, moved to member fn
template <typename Heap,class K,class W,class L>
inline void heapAdjustOrAdd ( Heap &heap, HeapKey<K,W,L> k) {
  typename Heap::iterator heapLoc=k.location();
  if (heapLoc)
    heapAdjustUp(heap.begin(),heapLoc);
  else
    heap_add(heap,k);
}

template <typename Heap,class K,class W,class L>
inline void heapSafeAdd ( Heap &heap, HeapKey<K,W,L> k) {
  if (!k.location())
    heap.push_back(k);
}


#ifdef GRAEHL__SINGLE_MAIN
template<class K,class W,class L>
THREADLOCAL W HeapKey<K,W,L>::weightmap(dummy<W>::var());

template<class K,class W,class L>
THREADLOCAL L HeapKey<K,W,L>::locmap(dummy<L>::var());

#endif

template<class K,class W,class L>
inline bool operator < (HeapKey<K,W,L> lhs, HeapKey<K,W,L> rhs) {
  return lhs.weight() > rhs.weight();
}



#ifdef GRAEHL_TEST
BOOST_AUTO_TEST_CASE( TEST_ADJUSTABLEHEAP )
{
}
#endif
}

#endif
