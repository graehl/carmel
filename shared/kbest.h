#ifndef KBEST_H
#define KBEST_H 1

#include "graph.h"
#include "myassert.h"
#include "list.h"
//#include "arc.h"

struct pGraphArc {
  GraphArc *p;
  GraphArc * operator ->() const { return p; }
  operator GraphArc *() const { return p; }
};

int operator < (const pGraphArc l, const pGraphArc r) ;

struct GraphHeap {
  GraphHeap *left, *right;	// for balanced heap
  int nDescend;
  //  GraphHeap *cross;
  // cross edge is implicitly determined by arc
  GraphArc *arc;		// data at each vertex
  pGraphArc *arcHeap;		// binary heap of sidetracks originating from a state
  int arcHeapSize;

  // custom allocator not optional because of how freeAll works!
  static GraphHeap *freeList;
  static const int newBlocksize;
  static List<GraphHeap *> usedBlocks;
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
  FLOAT_TYPE weight;
};

int operator < (const EdgePath &l, const EdgePath &r);

Graph sidetrackGraph(Graph lG, Graph rG, FLOAT_TYPE *dist); 
void buildSidetracksHeap(int state, int pred); // call depthfirstsearch with this; see usage in kbest.cc
void freeAllSidetracks(); // must be called after you buildSidetracksHeap
void printTree(GraphHeap *t, int n) ; 
void shortPrintTree(GraphHeap *t);

extern Graph sidetracks;
extern GraphHeap **pathGraph;
extern GraphState *shortPathTree;


// you can inherit from this or just provide the same interface
struct BestPathsVisitor {
    enum { SIDETRACKS_ONLY=0 };
    void start_path(unsigned k,FLOAT_TYPE cost) {} // called with k=rank of path (1-best, 2-best, etc.) and cost=sum of arcs from start to finish
    void visit_best_arc(const GraphArc &a) {}
    void visit_sidetrack_arc(const GraphArc &a) { visit_best_arc(a); }
};

template <class Visitor>
void bestPaths(Graph graph,unsigned source, unsigned dest,unsigned k,Visitor &v) {
}
#endif
