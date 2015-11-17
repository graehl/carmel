// Copyright 2014 Jonathan Graehl - http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "config.h"
#include <vector>
#include "kbest.h"
#include <cmath>

Graph sidetracks;
GraphHeap** pathGraph;
GraphState* shortPathTree;
using namespace std;


List<GraphHeap*> GraphHeap::usedBlocks;
GraphHeap* GraphHeap::freeList = NULL;
const unsigned GraphHeap::newBlocksize = 64;

vector<pGraphArc*> Repository;

void freeAllSidetracks() {
  for (unsigned i = 0; i < Repository.size(); i++)
    if (Repository[i]) delete[](pGraphArc*)Repository[i];
  Repository.clear();
}

void buildSidetracksHeap(unsigned state, unsigned pred) {
  // IMPORTANT NOTE: This function create NEW memory
  // of type (pGraphArc *). This memory is not deleted inside the function
  // because it is used else where. For this reason addresses for
  // the memory created is kept in a global variable "Repsitory" so that
  // the caller function (e.g., bestPaths) deletes the memory when it is done.
  GraphHeap* prev;

  if (pred == DFS_NO_PREDECESSOR)
    prev = NULL;
  else
    prev = pathGraph[pred];

#ifdef DEBUGKBEST
  Config::debug() << "buildSidetracksHeap state=" << state << " predecessor=" << pred << "\n";
#endif

  List<GraphArc>& arcs = sidetracks.states[state].arcs;
  List<GraphArc>::val_iterator s = arcs.val_begin(), end = arcs.val_end();
  if (s != end) {
    unsigned heapSize = 0;
    GraphArc* min;
    min = &(*s);
    while (++s != end) {
      if (s->weight < min->weight) min = &(*s);
      ++heapSize;
    }
    pathGraph[state] = new GraphHeap;
    pathGraph[state]->arc = min;
    pathGraph[state]->arcHeapSize = heapSize;
    if (heapSize) {
      pGraphArc* heapStart = pathGraph[state]->arcHeap = NEW pGraphArc[heapSize];
      Repository.push_back(heapStart);  // keep track of it so that we can delete it later
      pGraphArc* heapI = heapStart;
      //      List<GraphArc>::iterator end = sidetracks.states[state].arcs.end()  ;
      //    for ( List<GraphArc>::iterator gArc=sidetracks.states[state].arcs.begin() ; gArc !=end ; ++gArc )
      for (List<GraphArc>::val_iterator gArc = arcs.val_begin(), end = arcs.val_end(); gArc != end; ++gArc)
        if (&(*gArc) != min) (heapI++)->p = &(*gArc);
      Assert(heapI == heapStart + heapSize);
      heapBuild(heapStart, heapStart + heapSize);
    } else
      pathGraph[state]->arcHeap = NULL;
    pathGraph[state] = newTreeHeapAdd(prev, pathGraph[state]);
  } else
    pathGraph[state] = prev;
}  // end of buildSidetracksHeap()

// lG: regular graph
// rG: shortest path tree -> dest
// dist: array mapping vertex # -> cost to reach dest
Graph sidetrackGraph(Graph lG, Graph rG, FLOAT_TYPE* dist)
// This function creates new GraphState[] and because the
// return Graph points to this newly created Graph, it is NOT deleted. Therefore
//  the caller function is responsible for deleting this data.
//

{
  Assert(lG.nStates == rG.nStates);
  unsigned nStates = lG.nStates;
  GraphState* sub = NEW GraphState[nStates];
  for (unsigned i = 0; i < nStates; ++i)
    if (dist[i] != HUGE_VAL) {
      const List<GraphArc>& la = lG.states[i].arcs;
      for (List<GraphArc>::const_iterator l = la.const_begin(), end = la.const_end(); l != end; ++l) {
        Assert(i == l->src);

        const List<GraphArc>& ra = rG.states[i].arcs;
        for (List<GraphArc>::const_iterator r = ra.const_begin(), end = ra.const_end(); r != end; ++r)
          if (r->data == l->data) goto short_done;
        if (dist[l->dest] != HUGE_VAL) {
          GraphArc w = *l;
          telescope_cost(w, dist);  // w.weight = w.weight - (dist[i] - dist[w.dest]);
          sub[i].arcs.push(w);
        }
      short_done:
        ;
      }
    }
  Graph ret;
  ret.nStates = lG.nStates;
  ret.states = sub;
  return ret;
}

void printTree(GraphHeap* t, unsigned n) {
  unsigned i;
  for (i = 0; i < n; ++i) cout << ' ';
  if (!t) {
    cout << "-\n";
    return;
  }
  cout << (*t->arc);
  cout << " [";
  pGraphArc* heap = t->arcHeap;
  for (i = 0; i < t->arcHeapSize; ++i) {
    cout << *heap[i].p;
  }
  cout << "]\n";
  if (!t->left && !t->right) return;
  printTree(t->left, n + 1);
  printTree(t->right, n + 1);
}

void shortPrintTree(GraphHeap* t) {
  cout << *t->arc;
  if (!t->left && !t->right) return;
  cout << " (";
  if (t->left) {
    shortPrintTree(t->left);
    if (t->right) {
      cout << ' ';
      shortPrintTree(t->right);
    }
  } else if (t->right)
    shortPrintTree(t->right);
  cout << ')';
}
