#ifndef FST_H 
#define FST_H 1



#include <vector>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <sstream>
#include <iostream>
#include "2hash.h"
#include "list.h"
#include "weight.h"
#include "dynarray.h"
#include "strhash.h"
#include "graph.h"
#include "train.h"
#include "myassert.h"
#include "compose.h"
#include <iterator>
using namespace std;

ostream & operator << (ostream &out, const trainInfo &t); // Yaser 7-20-2000

class WFST;
struct PathArc {
  //const char *in;
  //const char *out;
  //const char *destState;
  int in,out,destState;
  const WFST *wfst;
  Weight weight;
};

std::ostream & operator << (std::ostream & o, const PathArc &p);



class WFST {
 private:	 
  enum { STATE,ARC } PerLine;
  enum { BRIEF,FULL } ArcFormat;
  enum { epsilon_index=0 };
  static const int perline_index; // handle to ostream iword for LogBase enum (initialized to 0)
  static const int arcformat_index; // handle for OutThresh
  void initAlphabet() {
    trn=NULL;
#define EPSILON_SYMBOL "*e*"
    in = NEW Alphabet<StringKey,StringPool>(EPSILON_SYMBOL);
    out = NEW Alphabet<StringKey,StringPool>(EPSILON_SYMBOL);
    ownerIn=ownerOut=1;
  }
	void train_prune(); // delete states with zero counts
  void deleteAlphabet() {
    if ( ownerIn ) {
      delete in;
      ownerIn = 0;
    }
    if ( ownerOut ) {
      delete out;
      ownerOut = 0;
    }
  }
  int getStateIndex(const char *buf); // creates the state according to named_states, returns -1 on failure
 public:

  template<class A,class B> static inline std::basic_ostream<A,B>&
    out_state_per_line(std::basic_ostream<A,B>& os) { os.iword(perline_index) = STATE; return os; }
  template<class A,class B> static inline std::basic_ostream<A,B>&
    out_arc_per_line(std::basic_ostream<A,B>& os) { os.iword(perline_index) = ARC; return os; }
  template<class A,class B> static inline std::basic_ostream<A,B>&
    out_arc_brief(std::basic_ostream<A,B>& os) { os.iword(arcformat_index) = BRIEF; return os; }
  template<class A,class B> static inline std::basic_ostream<A,B>&
    out_arc_full(std::basic_ostream<A,B>& os) { os.iword(arcformat_index) = FULL; return os; }


  static inline FLOAT_TYPE randomFloat()       // in range [0, 1)
    {
      return rand() * (1.f / (RAND_MAX+1.f));
    }

  bool ownerIn;
  bool ownerOut;
  bool named_states;
  Alphabet<StringKey,StringPool> *in;
  Alphabet<StringKey,StringPool> *out;
  Alphabet<StringKey,StringPool> stateNames;
  unsigned int final;	// final state number - initial state always number 0
  DynamicArray<State> states;
  	 
  //  HashTable<IntKey, int> tieGroup; // IntKey is Arc *; value in group number (0 means fixed weight)
  //  WFST(WFST &) {}		// disallow copy constructor - Yaser commented this ow to allow copy constructors
  //  WFST & operator = (WFST &){return *this;} Yaser
  //WFST & operator = (WFST &){std::cerr <<"Unauthorized use of assignemnt operator\n";;return *this;}
  int abort();			// called on a bad read
  int readLegible(istream &,bool alwaysNamed=false);	// returns 0 on failure (bad input)
  int readLegible(const string& str, bool alwaysNamed=false);  
  void writeArc(ostream &os, const Arc &a,bool GREEK_EPSILON=false);
  void writeLegible(ostream &);
  void writeGraphViz(ostream &); // see http://www.research.att.com/sw/tools/graphviz/
  int numStates() const { return states.size(); }
  bool isFinal(int s) { return s==final; }
  void setPathArc(PathArc *pArc,const Arc &a) {
    pArc->in = a.in;
    pArc->out = a.out;
    pArc->destState = a.dest;
    pArc->weight = a.weight;
    pArc->wfst=this;
  }
  std::ostream & printArc(const Arc &a,std::ostream &o) {
    PathArc p;
    setPathArc(&p,a);
    return o << p;
  }
  std::ostream & printArc(const Arc &a,int source,std::ostream &o) {
    return o << '(' << stateName(source) << " -> " << stateName(a.dest) << ' ' << inLetter(a.in) << " : " << outLetter(a.out) << " / " << a.weight << ")";
  }

  /*void insertPathArc(GraphArc *gArc, List<PathArc>*);  
    void insertShortPath(int source, int dest, List<PathArc> *);*/
  template <class T>
    void insertPathArc(GraphArc *gArc,T &l)
    {
      PathArc pArc;
      Arc *taken = (Arc *)gArc->data;
      setPathArc(&pArc,*taken);
      *(l++) = pArc;
    }
  template <class T>
    void insertShortPath(GraphState *shortPathTree,int source, int dest, T &l)
    {
      GraphArc *taken;
      for ( int iState = source ; iState != dest; iState = taken->dest ) {
	taken = &shortPathTree[iState].arcs.top();
	insertPathArc(taken,l);
      }
    }
  /*template <>
    void insertPathArc(GraphArc *gArc, List<PathArc>*l) {
    insertPathArc(gArc,insert_iterator<List<PathArc> >(*l,l->erase_begin()));
    }
    template <>
    void insertShortPath(GraphState *shortPathTree,int source, int dest, List<PathArc> *l) {
    insertShortPath(shortPathTree,source,dest,insert_iterator<List<PathArc> >(*l,l->erase_begin()));
    }*/

  static int indexThreshold;
  Weight ***forwardSumPaths(List<int> &inSeq, List<int> &outSeq);
  trainInfo *trn;
  enum NormalizeMethod { CONDITIONAL, // all arcs from a state with the same input will add to one
			 JOINT, // all arcs from a state will add to one (thus sum of all paths from start to finish = 1 assuming no dead ends
			 NONE // 
  } ;
  //     newPerplexity = train_estimate();
  //	lastChange = train_maximize(method);
  Weight train_estimate(bool delete_bad_training=true); // accumulates counts, returns perplexity of training set = 2^(- avg log likelihood) = 1/(Nth root of product of model probabilities of N-weight training examples)  - optionally deletes training examples that have no accepting path
  Weight train_maximize(NormalizeMethod method=CONDITIONAL,FLOAT_TYPE delta_scale=1); // normalize then exaggerate (then normalize again), returning maximum change
  WFST(const WFST &a) {}
 public:
  void index(int dir) {
    for ( int s = 0 ; s < numStates() ; ++s ) {
      states[s].flush();
      states[s].indexBy(dir);
    }
  }
  void indexInput() {
    index(0);
  }
  void indexOutput() {
    index(1);
  }

  void indexFlush() { // index on input symbol or output symbol depending on composition direction
    for ( int s = 0 ; s < numStates() ; ++s ) {
      states[s].flush();
    }
  }
  WFST() { initAlphabet(); named_states=0;}
  //  WFST(const WFST &a): 
  //ownerInOut(1), in(((a.in == 0)? 0:(NEW Alphabet(*a.in)))), out(((a.out == 0)? 0:(NEW Alphabet(*a.out)))), 
  //stateNames(a.stateNames), final(a.final), states(a.states), 
  //trn(((a.trn ==0)? 0 : (NEW trainInfo(*a.trn)))){}; // Yaser added this 7-25-2000 copy constructor*/

  WFST(istream & istr,bool alwaysNamed=false) {
    initAlphabet();
    if (!this->readLegible(istr,alwaysNamed))
      final = (unsigned)INVALID;
  }

  WFST(const string &str, bool alwaysNamed){
    initAlphabet();
    if (!this->readLegible(str,alwaysNamed))
      final = (unsigned)INVALID;
  }

  WFST(const char *buf); // make a simple transducer representing an input sequence
  WFST(const char *buf, int& length,bool permuteNumbers); // make a simple transducer representing an input sequence lattice - Yaser
  WFST(WFST &a, WFST &b, bool namedStates = false, bool preserveGroups = false);	// a composed with b
  // resulting WFST has only reference to input/output alphabets - use ownAlphabet()
  // if the original source of the alphabets must be deleted
  void listAlphabet(ostream &out, int output = 0);
  friend ostream & operator << (ostream &,  WFST &); // Yaser 7-20-2000
  // I=PathArc output iterator; returns length of path or -1 on error
  int randomPath(List<PathArc> *l,int max_len=-1) {
    return randomPath(insert_iterator<List<PathArc> >(*l,l->erase_begin()), max_len);
  }
  template <class I> int randomPath(I i,int max_len=-1)
    {
      PathArc p;
      int s=0;
      unsigned int len=0;
      unsigned int max=*(unsigned int *)&max_len;
      for (;;) {
	if (s == final)
	  return len;
	if  ( len > max || states[s].arcs.isEmpty() )
	  return -1;
	// choose random arc:
	Weight arcsum;
	typedef List<Arc> LA;
	typedef LA::const_iterator LAit;
	const LA& arcs=states[s].arcs;
	LAit start=arcs.const_begin(),end=arcs.const_end();
	for (LAit li = start; li!=end; ++li) {
	  arcsum+=li->weight;
	}
	Weight which_arc = arcsum * randomFloat();
	arcsum.setZero();
	for (LAit li = start; li!=end; ++li) {
	  if ( (arcsum += li->weight) >= which_arc) {
	    // add arc
	    setPathArc(&p,*li);
	    *i++ = p;
	    s=li->dest;
	    ++len;
	    //				if (!(i))
	    //					return -1;
	    break;
	  }
	}

	++len;
		
      }
    }

  List<List<PathArc> > * randomPaths(int k, int max_len=-1); // k) gives a list of k 
  // random paths to final
  // labels are pointers to names in WFST so do not
  // use the path after the WFST is deleted
  // list is dynamically allocated - delete it
  // yourself when you are done with it

  List<List<PathArc> > *bestPaths(int k); // bestPaths(k) gives a list of the (up to ) k 
  // best paths to final
  // labels are pointers to names in WFST so do not
  // use the path after the WFST is deleted
  // list is dynamically allocated - delete it
  // yourself when you are done with it
  List<int> *symbolList(const char *buf, int output=0) const;   
  // takes space-separated symbols and returns a list of symbol numbers in the
  // input or output alphabet
  const char *inLetter(int i) const {
    Assert ( i >= 0 );
    Assert ( i < in->size() );
    return (*in)[i];
  }
  const char *outLetter(int i) const {
    Assert ( i >= 0 );
    Assert ( i < out->size() );
    return (*out)[i];
  }
  //NB: uses static (must use or copy before next call) return string buffer if !named_states
  const char *stateName(int i) const {
    Assert ( i >= 0 );
    Assert ( i < numStates() );
    if (named_states)
      return stateNames[i];
    else
      return static_itoa(i);
  }
  Weight sumOfAllPaths(List<int> &inSeq, List<int> &outSeq);
  // gives sum of weights of all paths from initial->final with the input/output sequence (empties are elided)
	void randomScale() {  // randomly scale weights (of unlocked arcs) before training by (0..1]
		changeEachParameter(scaleRandom);
	}
	void randomSet() { // randomly set weights (of unlocked arcs) on (0..1]
		changeEachParameter(setRandom);
	}
  void normalize(NormalizeMethod method=CONDITIONAL);
  /*void normalizePerInput() {	
    normalize(CONDITIONAL);
    }*/

  // if weight_is_prior_count, weights before training are prior counts.  smoothFloor counts are also added to all arcs
  // NEW weight = normalize(induced forward/backward counts + weight_is_prior_count*old_weight + smoothFloor)
  void trainBegin(NormalizeMethod method=CONDITIONAL,bool weight_is_prior_count=false, Weight smoothFloor=0.0);
  void trainExample(List<int> &inSeq, List<int> &outSeq, FLOAT_TYPE weight);
  Weight trainFinish(Weight converge_arc_delta, Weight converge_perplexity_ratio, int maxTrainIter,FLOAT_TYPE learning_rate_growth_factor,NormalizeMethod method=CONDITIONAL, int ran_restarts=0);
  // stop if greatest change in arc weight, or per-example perplexity is less than criteria, or after set number of iterations.  
	// returns per-example perplexity achieved

  void invert();		// switch input letters for output letters
  void reduce();		// eliminate all states not along a path from
                                // initial state to final state
  void consolidateArcs();	// combine identical arcs, with combined weight = sum
  void pruneArcs(Weight thresh);	// remove all arcs with weight < thresh
  enum {UNLIMITED=-1};
  void prunePaths(int max_states=UNLIMITED,Weight keep_paths_within_ratio=Weight::INF); 
  // throw out rank states by the weight of the best path through them, keeping only max_states of them (or all of them, if max_states<0), after removing states and arcs that do not lie on any path of weight less than (best_path/keep_paths_within_ratio)
  
  
  void assignWeights(const WFST &weightSource); // for arcs in this transducer with the same group number as an arc in weightSource, assign the weight of the arc in weightSource
  void numberArcsFrom(int labelStart); // sequentially number each arc (placing it into that group) starting at labelStart - labelStart must be >= 1
  void lockArcs();		// put all arcs in group 0 (weights are locked)
  //  void unTieGroups() { tieGroup.~HashTable(); PLACEMENT_NEW (&tieGroup) HashTable<IntKey, int>; }
  void unTieGroups();
  
  
  int generate(int *inSeq, int *outSeq, int minArcs, int maxArcs);
  enum{INVALID=-1};
  int valid() const { return ( final != (unsigned)INVALID ); }
  unsigned int size() const { if ( !valid() ) return 0; else return numStates(); }
  int numArcs() const {
    int a = 0;
    for (int i = 0 ; i < numStates() ; ++i )
      a += states[i].size;
    return a;
  }
  Weight numNoCyclePaths() const {
    if ( !valid() ) return Weight::ZERO;
    Weight *nPaths = NEW Weight[numStates()];
    Graph g = makeGraph();
    countNoCyclePaths(g, nPaths, 0);
    delete[] g.states;
    Weight ret = nPaths[final];
    delete[] nPaths;
    return ret;
  }
  static void setIndexThreshold(int t) {
    if ( t < 0 )
      WFST::indexThreshold = 0;
    else
      WFST::indexThreshold = t; 
  }
  Graph makeGraph() const; // weights = -log, so path length is sum and best path 
                           // is the shortest; GraphArc::data is a pointer 
                           // to the Arc it corresponds to in the WFST
  Graph makeEGraph() const; // same as makeGraph, but restricted to *e* / *e* arcs
  void ownAlphabet() {
    ownInAlphabet();
    ownOutAlphabet();
  }
  void stealInAlphabet(WFST &from) {
    if (from.ownerIn && from.in == in) {
      from.ownerIn=0;
      ownerIn=1;
    } else
      ownInAlphabet();
  }
  void stealOutAlphabet(WFST &from) {
    if (from.ownerOut && from.out == out) {
      from.ownerOut=0;
      ownerOut=1;
    } else
      ownOutAlphabet();
  }
  void ownInAlphabet() {
    if ( !ownerIn ) {
      in = NEW Alphabet<StringKey,StringPool>(*in);
      ownerIn = 1;
    }
  }
  void ownOutAlphabet() {
    if ( !ownerOut ) {
      out = NEW Alphabet<StringKey,StringPool>(*out);
      ownerOut = 1;
    }
  }
  void unNameStates() {
    if (named_states) {
      stateNames.~Alphabet();
      named_states=false;
      PLACEMENT_NEW (&stateNames) Alphabet<StringKey,StringPool>();
    }
  }

  void clear() {
    unNameStates();
    states.clear();
    deleteAlphabet();
    final = (unsigned)INVALID;
  }
  ~WFST() {
    clear();
  }
	typedef void (*WeightChanger)(Weight *w);
	static void setRandom(Weight *w) {
		Weight random;
		random.setRandomFraction();
		*w = random;
	}
	static void scaleRandom(Weight *w) {
		Weight random;
		random.setRandomFraction();
		*w *= random;
	}
		template <class F> void readEachParameter(F f) {
		  HashTable<IntKey, bool> seenGroups;
		  for ( int s = 0 ; s < numStates() ; ++s )
				for ( List<Arc>::val_iterator a=states[s].arcs.val_begin(),end = states[s].arcs.val_end(); a != end ; ++a )  {
				  int group=a->groupId;
				  if (isNormal(group) || isTied(group) && seenGroups.insert(HashTable<IntKey, bool>::value_type(group,true)).second)
					f((const Weight *)&(a->weight));
			 }
		}
	template <class F> void changeEachParameter(F f) {
			HashTable<IntKey, Weight> tiedWeights;
		  for ( int s = 0 ; s < numStates() ; ++s )
				for ( List<Arc>::val_iterator a=states[s].arcs.val_begin(),end = states[s].arcs.val_end(); a != end ; ++a )  {
					int group=a->groupId;
					if (isNormal(group))
						f(&(a->weight));
					if (isTied(group)) { // assumption: all the weights for the tie group are the same (they should be, after normalization at least)
//#define OLD_EACH_PARAM
#ifdef OLD_EACH_PARAM
						Weight *pw;
						if (pw=find_second(tiedWeights,group))
							a->weight=*pw;
						else {
							f(&(a->weight));
							add(tiedWeights,group,a->weight);
						}
#else
	  					HashTable<IntKey, Weight>::insert_return_type it;
						if ((it=tiedWeights.insert(HashTable<IntKey, Weight>::value_type(group,0))).second) {
						  f(&(a->weight));
						  it.first->second = a->weight;
						} else
						  a->weight = it.first->second;
#endif
					}
				}
	}
  void removeMarkedStates(bool marked[]);  // remove states and all arcs to
  // states marked true
  static inline bool isNormal(int groupId) {
    //return groupId == WFST::NOGROUP;
    return groupId < 0;
  }
  static inline bool isLocked(int groupId) {
    return groupId == WFST::LOCKEDGROUP;
  }
  static inline bool isTied(int groupId) {
    return groupId > 0;
  }
  static inline bool isTiedOrLocked(int groupId) {
    return groupId >= 0;
  }
  static const int NOGROUP=-1;  
  static const int LOCKEDGROUP=0;
 private:
  //  static const int NOGROUP(-1);  
  void invalidate() {		// make into empty/invalid transducer
    clear();
  }
};

ostream & operator << (ostream &o, WFST &w);

ostream & operator << (std::ostream &o, List<PathArc> &l);


#endif
