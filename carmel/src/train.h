#ifndef TRAIN_H
#define TRAIN_H 1
#include "config.h"
#include "myassert.h"
#include "2hash.h"
#include "weight.h"
#include "list.h"
#include "2hash.h"
#include "Arc.h"
#include <iostream>

struct IntKey {
  int i;
  int hash() const { return i * 2654435767U; }
  IntKey() {}
  IntKey(int a) : i(a) {}
  operator int() const { return i; }
};

HASHNS_B
//template <> struct hash<IntKey> { size_t operator()(IntKey i) const { return i.hash(); } };
template<>
struct hash<IntKey>
{
  size_t operator()(IntKey i) const {
	return i.hash();
  }
};
HASHNS_E
struct State {
  List<Arc> arcs;
  int size;
#ifdef BIDIRECTIONAL
  int hitcount;			// how many times index is used, negative for index on input, positive for index on output
#endif
  HashTable<IntKey, List<HalfArc> > *index;
  State() : arcs(), size(0), 
#ifdef BIDIRECTIONAL
       hitcount(0), 
#endif
       index(NULL) { }
  State(const State &s): arcs(s.arcs),size(s.size) {
#ifdef BIDIRECTIONAL
    hitcount = s.hitcount;
#endif
//    if (s.index == NULL)
      index = (HashTable<IntKey, List<HalfArc> > *) NULL ;
//    else
//     index = NEW HashTable<IntKey, List<HalfArc> >(*s.index);
  } 
  ~State() { flush(); }
  void indexBy(int output = 0) {
    List<HalfArc> *list;
    if ( output ) {
#ifdef BIDIRECTIONAL
      if ( hitcount++ > 0 )
	return;
      hitcount = 1;
      delete index;
#else
      if ( index )
	return;
#endif
      index = NEW HashTable<IntKey, List<HalfArc> >(size);
      for ( List<Arc>::val_iterator l=arcs.val_begin(),end=arcs.val_end() ; 
	    l != end  ; 
	    ++l ) {
#define QUEERINDEX
#ifdef QUEERINDEX
 	if ( !(list = find_second(*index,(IntKey)l->out)) )
	  add(*index,(IntKey)l->out, 
		     List<HalfArc>(&(*l)));
	else
	  list->push_front(&(*l));
#else
	list=&(*index)[l->out];
	list->push_front(&(*l));
#endif
      }
      return;
    }
#ifdef BIDIRECTIONAL
    if ( hitcount-- < 0 )
      return;
    hitcount = -1;
    delete index;
#else
    if ( index )
      return;
#endif
    index = NEW HashTable<IntKey, List<HalfArc> >(size);
    for ( List<Arc>::val_iterator l=arcs.val_begin(),end=arcs.val_end() ; l != end ; ++l ) {
#ifdef QUEERINDEX
	  if ( !(list = find_second(*index,(IntKey)l->in)) )
			  add(*index,(IntKey)l->in, 
		     List<HalfArc>(&(*l)));	
      else
	list->push_front(&(*l));
#else
	list=&(*index)[l->in];
	list->push_front(&(*l));
#endif

    }
  }
  void flush() {
    if (index)
      delete index;
    index = NULL;
#ifdef BIDIRECTIONAL
    hitcount = 0;
#endif
  }
  
  void addArc(const Arc &arc)
  {
    arcs.push(arc);
    ++size;
    Assert(!index);
#ifdef DEBUG
    if (index) {
      std::cerr << "Warning: adding arc to indexed state.\n";
      delete index;
      index = NULL;
    }
#endif
  }
  void reduce() {		// consolidate all duplicate arcs
    HashTable<UnArc, Weight *> hWeights;
    UnArc un;
    Weight **ppWt;
    for ( List<Arc>::erase_iterator l=arcs.erase_begin(),end=arcs.erase_end() ; l != end ;) {
      if ( l->weight.isZero() ) {
	l=remove(l);
	continue;
      }
      un.in = l->in;
      un.out = l->out;
      un.dest = l->dest;
      if ( (ppWt = find_second(hWeights,un)) ) {
	**ppWt += l->weight;
	if ( **ppWt > 1 )
	  **ppWt = Weight((FLOAT_TYPE)1.);
	
	l=remove(l);
      } else {
	hWeights[un]= &l->weight; // add?
	++l;
      }
    }
  }
  void prune(Weight thresh) {
    for ( List<Arc>::erase_iterator l=arcs.erase_begin(),end=arcs.erase_end() ; l != end ;) {
      if ( l->weight < thresh ) {
	l=remove(l);
      } else
      	++l;
    }
  }
  template <class T>
  T remove(T t) {
    --size;
    return arcs.erase(t);
  }
  void renumberDestinations(int *oldToNew) { // negative means remove transition
    Assert(!index);
    for (List<Arc>::erase_iterator l=arcs.erase_begin(),end=arcs.erase_end(); l != end; ) {
      int &dest = (int &)l->dest;
      if ( oldToNew[dest] < 0 ) {
	l=remove(l); 
      } else {
	dest = oldToNew[dest];
	++l;
      }
    }
  }
};

std::ostream& operator << (std::ostream &out, struct State &s); // Yaser 7-20-2000

struct IOPair {
  int in;
  int out;
  int hash() const
  {
    return (67913 * out + in) * 2654435767U;
  }
};

HASHNS_B
template<>
struct hash<IOPair>
{
  size_t operator()(const IOPair &x) const {
	return x.hash();
  }
};
HASHNS_E
std::ostream & operator << (std::ostream &o, IOPair p);

inline bool operator == (const IOPair l, const IOPair r) { 
  return l.in == r.in && l.out == r.out; }

struct DWPair {
  int dest;
  Arc *arc;
  Weight scratch;
  Weight em_weight;
  Weight best_weight;
  Weight counts;
  Weight prior_counts;
  Weight &weight() const { return arc->weight; }
};

std::ostream & operator << (std::ostream &o, DWPair p);


std::ostream & hashPrint(HashTable<IOPair, List<DWPair> > &h, std::ostream &o);

struct symSeq {
  int n;
  int *let;
  int *rLet;
};

std::ostream & operator << (std::ostream & out , const symSeq & s);

struct IOSymSeq {
  symSeq i;
  symSeq o;
  FLOAT_TYPE weight;
  void init(List<int> &inSeq, List<int> &outSeq, FLOAT_TYPE w) {
    i.n = inSeq.count_length();
    o.n = outSeq.count_length();
    i.let = NEW int[i.n];
    o.let = NEW int[o.n];
    i.rLet = NEW int[i.n];
    o.rLet = NEW int[o.n];
    int *pi, *rpi;
    pi = i.let;
    rpi = i.rLet + i.n;
    for ( List<int>::const_iterator inL=inSeq.const_begin(),end=inSeq.const_end() ; inL != end ; ++inL )
      *pi++ = *--rpi = *inL;
    pi = o.let;
    rpi = o.rLet + o.n;
    for ( List<int>::const_iterator outL=outSeq.const_begin(),end=outSeq.const_end() ; outL != end ; ++outL )
      *pi++ = *--rpi = *outL;
    weight = w;
  }
  void kill() {
    delete[] o.let;
    delete[] o.rLet;
    delete[] i.let;
    delete[] i.rLet;
  }
};

std::ostream & operator << (std::ostream & out , const IOSymSeq & s);   // Yaser 7-21-2000

class trainInfo {
 public:
  HashTable<IOPair, List<DWPair> > *forArcs;
  HashTable<IOPair, List<DWPair> > *revArcs;
  List<int> *forETopo;
  List<int> *revETopo;
  Weight ***f;
  Weight ***b;
  int maxIn, maxOut;
  List <IOSymSeq> examples;
  //Weight smoothFloor;
  FLOAT_TYPE totalEmpiricalWeight;
  int nStates; // Yaser added this . number of States 
#ifdef N_E_REPS // Yaser : the following variables need to be taken care of in the copy constructor
  Weight *wNew;
  Weight *wOld;
#endif

  trainInfo() {};
 private:
  trainInfo(const trainInfo& a){
  }
  /*  if (a.forArcs == NULL)
      forArcs = NULL ;
      else 
      forArcs=NEW HashTable<IOPair, List<DWPair> >(*a.forArcs);
      revArcs=NEW HashTable<IOPair, List<DWPair> >(*a.revArcs);
      forETopo=NEW List<int>(*a.forETopo);
      revETopo=NEW List<int>(*a.revETopo);
      maxIn=(a.maxIn);
      maxOut=(a.maxOut);
      examples=(a.examples);
      smoothFloor =(a.smoothFloor);
      nStates=a.nStates;
      f = NEW Weight **[maxIn+1];
      b = NEW Weight **[maxIn+1];
      for ( int i = 0 ; i <= maxIn ; ++i ) {
      f[i] = NEW Weight *[maxOut+1];
      b[i] = NEW Weight *[maxOut+1];
      for ( int o = 0 ; o <= maxOut ; ++o ) {
      f[i][o] = NEW Weight [nStates];
      b[i][o] = NEW Weight [nStates];
      for (int s = 0 ; s < nStates ; s++){
      f[i][o][s] = a.f[i][o][s] ;
      b[i][o][s] = a.b[i][o][s] ;
      }
      }
      }    
      };*/
};

#endif
