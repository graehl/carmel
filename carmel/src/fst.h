/*******************************************************************************
* This software ("Carmel") is licensed for research use only, as described in  *
* the LICENSE file in this distribution.  The software can be downloaded from  *
* http://www.isi.edu/natural-language/licenses/carmel-license.html.  Please    *
* contact Yaser Al-Onaizan (yaser@isi.edu) or Kevin Knight (knight@isi.edu)    *
* with questions about the software or commercial licensing.  All software is  *
* copyrighted C 2000 by the University of Southern California.                 *
*******************************************************************************/
#ifndef FST_H 
#define FST_H 1


#define CUSTOMNEW

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
 //public:
  bool ownerInOut;
  Alphabet *in;
  Alphabet *out;
  Alphabet stateNames;
  int final;	// final state number - initial state always number 0
  DynamicArray<State> states;
  //  HashTable<IntKey, int> tieGroup; // IntKey is Arc *; value in group number (0 means fixed weight)
//  WFST(WFST &) {}		// disallow copy constructor - Yaser commented this ow to allow copy constructors
//  WFST & operator = (WFST &){return *this;} Yaser
  //WFST & operator = (WFST &){cerr <<"Unauthorized use of assignemnt operator\n";;return *this;}
  int abort();			// called on a bad read
  int readLegible(istream &);	// returns 0 on failure (bad input)
  void writeLegible(ostream &);
  int numStates() const { return states.count(); }
#ifdef _MSC_VER
typedef PathArc T;
#else
template <typename T>
#endif   
void insertPathArc(GraphArc *gArc, List<T>*,
#ifndef _MSC_VER
				   typename 
#endif
				   List<T>::iterator &p);  
#ifndef _MSC_VER
template <typename T>
#endif    
void insertShortPath(int source, int dest, List<T> *,
#ifndef _MSC_VER
					 typename 
#endif
					 List<T>::iterator &p);
  static int indexThreshold;
  Weight ***forwardSumPaths(List<int> &inSeq, List<int> &outSeq);
  trainInfo *trn;
  Weight train(const int iter); // returns max change in any arcs weight - Yaser 7-13-2000
  WFST(const WFST &a) {}
public:
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
  void trainBegin();
  void trainExample(List<int> &inSeq, List<int> &outSeq, float weight);
  void trainFinish(Weight epsilon, Weight smoothFloor, int maxTrainIter);
  void invert();		// switch input letters for output letters
  void reduce();		// eliminate all states not along a path from
                                // initial state to final state
  void consolidateArcs();	// combine identical arcs, with combined weight = sum
  void prune(Weight thresh);	// remove all arcs with weight < thresh
  void normalizePerInput();	// all arcs from a state with the same input will add to one
  void assignWeights(const WFST &weightSource); // for arcs in this transducer with the same group number as an arc in weightSource, assign the weight of the arc in weightSource
  void numberArcsFrom(int labelStart); // sequentially number each arc (placing it into that group) starting at labelStart - labelStart must be >= 1
  void lockArcs();		// put all arcs in group 0 (weights are locked)
  //  void unTieGroups() { tieGroup.~HashTable(); new (&tieGroup) HashTable<IntKey, int>; }
  void unTieGroups();
  
  
  int generate(int *inSeq, int *outSeq, int minArcs = 0);
  int valid() const { return ( final >= 0 ); }
  int count() const { if ( !valid() ) return 0; else return numStates(); }
  int numArcs() const {
    int a = 0;
    for (int i = 0 ; i < numStates() ; ++i )
      a += states[i].arcs.length();
    return a;
  }
  Weight numNoCyclePaths() const {
    if ( !valid() ) return 0;
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
  ~WFST() {
    if ( ownerInOut ) {
      delete in;
      delete out;
      ownerInOut = 0;
    }
    final = -1;
  }
private:
  //  static const int NOGROUP(-1);  
  static const int NOGROUP=-1;  
  void removeMarkedStates(bool marked[]);  // remove states and all arcs to
                                             // states marked true
  void invalidate() {		// make into empty/invalid transducer
    if ( valid() ) {
      stateNames.~Alphabet();	// safe to call these more than once
      states.~DynamicArray();
      if ( ownerInOut ) {
	delete in;
	delete out;
	ownerInOut = 0;
      }
      final = -1;
    }
  }
};

ostream & operator << (ostream &o, WFST &w);

ostream & operator << (ostream &o, List<PathArc> &l);



#endif
