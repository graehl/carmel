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
#include "assert.h"
#include "compose.h"
using namespace std;

ostream & operator << (ostream &out, const trainInfo &t); // Yaser 7-20-2000



class WFST {
private:
 enum { STATE,ARC } PerLine;
 enum { BRIEF,FULL } ArcFormat;
static const int perline_index; // handle to ostream iword for LogBase enum (initialized to 0)
static const int arcformat_index; // handle for OutThresh

public:

template<class A,class B> static inline std::basic_ostream<A,B>&
out_state_per_line(std::basic_ostream<A,B>& os) { os.iword(perline_index) = STATE; return os; }
template<class A,class B> static inline std::basic_ostream<A,B>&
out_arc_per_line(std::basic_ostream<A,B>& os) { os.iword(perline_index) = ARC; return os; }
template<class A,class B> static inline std::basic_ostream<A,B>&
out_arc_brief(std::basic_ostream<A,B>& os) { os.iword(arcformat_index) = BRIEF; return os; }
template<class A,class B> static inline std::basic_ostream<A,B>&
out_arc_full(std::basic_ostream<A,B>& os) { os.iword(arcformat_index) = FULL; return os; }


 static inline float randomFloat()       // in range [0, 1)
{
  return rand() * (1.f / (RAND_MAX+1.f));
}

  bool ownerInOut;
  Alphabet *in;
  Alphabet *out;
  Alphabet stateNames;
  int final;	// final state number - initial state always number 0
  DynamicArray<State> states;
  	 
  //  HashTable<IntKey, int> tieGroup; // IntKey is Arc *; value in group number (0 means fixed weight)
//  WFST(WFST &) {}		// disallow copy constructor - Yaser commented this ow to allow copy constructors
//  WFST & operator = (WFST &){return *this;} Yaser
  //WFST & operator = (WFST &){std::cerr <<"Unauthorized use of assignemnt operator\n";;return *this;}
  int abort();			// called on a bad read
  int readLegible(istream &);	// returns 0 on failure (bad input)
  void writeLegible(ostream &);
  int numStates() const { return states.count(); }
  
  void setPathArc(PathArc *pArc,const Arc &a) {
	  pArc->in = inLetter(a.in);
	 pArc->out = outLetter(a.out);
	  pArc->destState = stateName(a.dest);
	 pArc->weight = a.weight;
  }
  std::ostream & printArc(const Arc &a,std::ostream &o) {
    PathArc p;
	setPathArc(&p,a);
	return o << p;
  }
  std::ostream & printArc(const Arc &a,int source,std::ostream &o) {
	return o << '(' << stateName(source) << " -> " << stateName(a.dest) << ' ' << inLetter(a.in) << " : " << outLetter(a.out) << " / " << a.weight << ")";
  }

void insertPathArc(GraphArc *gArc, List<PathArc>*);  
void insertShortPath(int source, int dest, List<PathArc> *);
  static int indexThreshold;
  Weight ***forwardSumPaths(List<int> &inSeq, List<int> &outSeq);
  trainInfo *trn;
  enum NormalizeMethod { CONDITIONAL, // all arcs from a state with the same input will add to one
	  JOINT, // all arcs from a state will add to one (thus sum of all paths from start to finish = 1 assuming no dead ends
	  NONE // 
  } ;
  Weight train(const int iter,NormalizeMethod method=CONDITIONAL,Weight *oldPerplexity = NULL); // returns max change in any arcs weight - Yaser 7-13-2000, oldPerplexity gives perplexity of training set = Nth root of product of model probabilities of N-weight training examples
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
	void scaleArcs(Weight w) {
		for ( int s = 0 ; s < numStates() ; ++s ) {
			states[s].scaleArcs(w);
		}
	}
  WFST() : ownerInOut(1), in(new Alphabet("*e*")),  out(new Alphabet("*e*")), trn(NULL) { }
//  WFST(const WFST &a): 
    //ownerInOut(1), in(((a.in == 0)? 0:(new Alphabet(*a.in)))), out(((a.out == 0)? 0:(new Alphabet(*a.out)))), 
    //stateNames(a.stateNames), final(a.final), states(a.states), 
    //trn(((a.trn ==0)? 0 : (new trainInfo(*a.trn)))){}; // Yaser added this 7-25-2000 copy constructor*/

  WFST(istream & istr) : ownerInOut(1), in(new Alphabet("*e*")),  out(new Alphabet("*e*")), trn(NULL) {
    if (!this->readLegible(istr))
      final = -1;
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
	  return randomPath(back_insert_iterator<List<PathArc> > (*l), max_len);
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
		LAit start=arcs.begin(),end=arcs.end();
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
  const char *inLetter(int i) {
    Assert ( i >= 0 );
    Assert ( i < in->count() );
    return (*in)[i];
  }
  const char *outLetter(int i) {
    Assert ( i >= 0 );
    Assert ( i < out->count() );
    return (*out)[i];
  }
  const char *stateName(int i) {
    Assert ( i >= 0 );
    Assert ( i < numStates() );
    return stateNames[i];
  }
  Weight sumOfAllPaths(List<int> &inSeq, List<int> &outSeq);
  // gives sum of weights of all paths from initial->final with the input/output sequence (empties are elided)
  void normalize(NormalizeMethod method=CONDITIONAL);
  /*void normalizePerInput() {	
	  normalize(CONDITIONAL);
  }*/

  // if weight_is_prior_count, weights before training are prior counts.  smoothFloor counts are also added to all arcs
  // new weight = normalize(induced forward/backward counts + weight_is_prior_count*old_weight + smoothFloor)
  void trainBegin(NormalizeMethod method=CONDITIONAL,bool weight_is_prior_count=false, Weight smoothFloor=0.0);
  void trainExample(List<int> &inSeq, List<int> &outSeq, float weight);
  void trainFinish(Weight converge_arc_delta, Weight converge_perplexity_ratio, int maxTrainIter,NormalizeMethod method=CONDITIONAL);
  // stop if greatest change in arc weight, or per-example perplexity is less than criteria, or after set number of iterations.  

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
  //  void unTieGroups() { tieGroup.~HashTable(); new (&tieGroup) HashTable<IntKey, int>; }
  void unTieGroups();
  
  
  int generate(int *inSeq, int *outSeq, int minArcs, int maxArcs);
  int valid() const { return ( final >= 0 ); }
  int count() const { if ( !valid() ) return 0; else return numStates(); }
  int numArcs() const {
    int a = 0;
    for (int i = 0 ; i < numStates() ; ++i )
      a += states[i].arcs.length();
    return a;
  }
  Weight numNoCyclePaths() const {
	  if ( !valid() ) return Weight::ZERO;
    Weight *nPaths = new Weight[numStates()];
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
    if ( !ownerInOut ) {
      in = new Alphabet(*in);
      out = new Alphabet(*out);
      ownerInOut = 1;
    }
  }
  void unNameStates() {
    stateNames.~Alphabet();
    new (&stateNames) Alphabet();
  }
  void clear() {
	unNameStates();
	states.clear();
    if ( ownerInOut ) {
      delete in;
      delete out;
      ownerInOut = 0;
    }
    final = -1;
  }
  ~WFST() {
	  clear();
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
