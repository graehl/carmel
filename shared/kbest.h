/*******************************************************************************
* This software ("Carmel") is licensed for research use only, as described in  *
* the LICENSE file in this distribution.  The software can be downloaded from  *
* http://www.isi.edu/natural-language/licenses/carmel-license.html.  Please    *
* contact Yaser Al-Onaizan (yaser@isi.edu) or Kevin Knight (knight@isi.edu)    *
* with questions about the software or commercial licensing.  All software is  *
* copyrighted C 2000 by the University of Southern California.                 *
*******************************************************************************/
#ifndef KBEST_H
#define KBEST_H 1

#include "graph.h"
#include "assert.h"
#include "train.h"
#include "list.h"
#include "fst.h"

struct pGraphArc {
  GraphArc *p;
  GraphArc * operator ->() const { return p; }
  operator GraphArc *() const { return p; }
};

int operator < (const pGraphArc l, const pGraphArc r) ;

struct GraphHeap {
  static List<GraphHeap *> usedBlocks;
  static GraphHeap *freeList;
  static const int newBlocksize;
  GraphHeap *left, *right;	// for balanced heap
  int nDescend;
  //  GraphHeap *cross;
  // cross edge is implicitly determined by arc
  GraphArc *arc;		// data at each vertex
  pGraphArc *arcHeap;		// binary heap of sidetracks originating from a state
  int arcHeapSize;
  void *operator new(size_t s)
    {
      size_t dummy = s;
      dummy = dummy;
      GraphHeap *ret, *max;
      if (freeList) {
	ret = freeList;
	freeList = freeList->left;
	return ret;
      }
      freeList = (GraphHeap *)::operator new(newBlocksize * sizeof(GraphHeap));
      usedBlocks.push(freeList);
      freeList->left = NULL;
      max = freeList + newBlocksize -1;
      for ( ret = freeList++; freeList < max ; ret = freeList++ )
	freeList->left = ret;
      return freeList--;
    }
  void operator delete(void *p) 
    {
      GraphHeap *e = (GraphHeap *)p;
      e->left = freeList;
      freeList = e;
    }
  static void freeAll()
    {
      while ( usedBlocks.notEmpty() ) {
	::operator delete((void *)usedBlocks.top());
	usedBlocks.pop();
      }
      freeList = NULL;
    }
};


int operator < (const GraphHeap &l, const GraphHeap &r);
struct EdgePath {
  GraphHeap *node;
  int heapPos;			// -1 if arc is GraphHeap.arc
  EdgePath *last;
  float weight;
};

int operator < (const EdgePath &l, const EdgePath &r);

Graph sidetrackGraph(Graph lG, Graph rG, float *dist); 
void buildSidetracksHeap(int state, int pred); 
void printTree(GraphHeap *t, int n) ; 
void shortPrintTree(GraphHeap *t);
#endif
