#include "graph.h"
#include "assert.h"
#include "node.h"


std::ostream & operator << (std::ostream &out, const GraphArc &a)
{
  return out << '(' << a.source << ' ' << a.dest << ' ' << a.weight << ')';
}

void (*dfsFunc)(int, int) = NULL;
void (*dfsExitFunc)(int, int) = NULL;

void depthFirstSearch(Graph graph, int startState, bool* visited, void (*func)(int state, int pred)) {
  dfsGraph = graph;
  dfsVis = visited;
  dfsFunc = func;
  dfsExitFunc = NULL;
  dfsRec(startState, -1);
}

Node<GraphArc> *Node<GraphArc>::freeList = NULL;
const int Node<GraphArc>::newBlocksize = 64;

Graph reverseGraph(Graph g)
// Comment by Yaser: This function creates new GraphState[] and because the
// return Graph points to this newly created Graph, it is NOT deleted. Therefore
// whatever the caller function is responsible for deleting this data.
// It is not a good programming practice but it will be messy to clean it up.
//

{
  GraphState *rev = new GraphState[g.nStates];
  for ( int i  = 0 ; i < g.nStates ; ++i ){
    List<GraphArc>::iterator end = g.states[i].arcs.end() ;
    for ( List<GraphArc>::iterator l=g.states[i].arcs.begin() ; l !=end ; ++l ) {
      GraphArc r;
      r.data = &(*l);
      Assert(i == l->source);
      r.dest = i;
      r.source = l->dest;
      r.weight = l->weight;
      rev[r.source].arcs.push(r);
    }
  }
  Graph ret;
  ret.states = rev;
  ret.nStates = g.nStates;
  return ret;
}

Graph dfsGraph;
bool *dfsVis;


void dfsRec(int state, int pred) {

  if ( dfsVis[state] )
    return;


  dfsVis[state] = true;
  if ( dfsFunc )
    dfsFunc(state, pred);

  List<GraphArc>::iterator end = dfsGraph.states[state].arcs.end() ;
  for ( List<GraphArc>::iterator l=dfsGraph.states[state].arcs.begin() ; l !=end ; ++l ) {
    int dest = l->dest;
    dfsRec(dest, state);
  }
  if ( dfsExitFunc )
    dfsExitFunc(state, pred);
}





void countNoCyclePaths(Graph g, Weight *nPaths, int source) {
  List<int> topo;
  //front_insert_iterator<List<int> > o(topo);
  
	  TopoSort sort(g,&topo);
	  sort.order_from(source);
  

  for ( int i = 0 ; i < g.nStates; ++i )
    nPaths[i].setZero();
  nPaths[source] = 1;
  List<int>::const_iterator end = topo.end();
  for ( List<int>::const_iterator t=topo.begin()  ; t != end; ++t ){
    List<GraphArc>::const_iterator end2 = g.states[*t].arcs.end() ;
    for ( List<GraphArc>::const_iterator a=g.states[*t].arcs.begin() ; a !=end2; ++a )
      nPaths[a->dest] += nPaths[(*t)];
  }
}


float *DistToState::weights = NULL;
DistToState **DistToState::stateLocations = NULL;
float DistToState::unreachable = Weight::HUGE_FLOAT;

inline bool operator < (DistToState lhs, DistToState rhs) {
  return DistToState::weights[lhs.state] > DistToState::weights[rhs.state];
}

inline bool operator == (DistToState lhs, DistToState rhs) {
  return DistToState::weights[lhs.state] == DistToState::weights[rhs.state];
}

inline bool operator == (DistToState lhs, float rhs) {
  return DistToState::weights[lhs.state] == rhs;
}

void shortestPathTree(Graph g, GraphState *pathTree,int dest, float *dist)
// computes best paths from each state to a single destination
// if pathTree == NULL, only compute weights (stored in dist)
//  otherwise, store arc used for state s in pathTree[s]

{
  int nStates = g.nStates;
  GraphArc **best = NULL;
  int i;
  if (pathTree) {
	   best = new GraphArc *[nStates];
	    for ( i = 0 ; i < nStates ; ++i )
		    best[i] = NULL;
  }


  GraphState *rev = reverseGraph(g).states;

  //GraphState *pathTree = new GraphState[nStates];
  int nUnknown = nStates;

  DistToState *distQueue = new DistToState[nStates];

  //  float *weights = new float[nStates];
  float *weights = dist;
  
  for ( i = 0 ; i < nStates ; ++i ) {
          weights[i] = Weight::HUGE_FLOAT;
  }

  DistToState **stateLocations = new DistToState *[nStates];
  DistToState::weights = weights;
  DistToState::stateLocations = stateLocations;

  weights[dest] = 0;
  for ( i = 1; i < nStates ; ++i ) {
    int fillWith;
    if ( i <= dest )
      fillWith = i-1;
    else
      fillWith = i;
    distQueue[i].state = fillWith;
    stateLocations[fillWith] = &distQueue[i];
  }
  distQueue[0].state = dest;
  stateLocations[dest] = &distQueue[0];


  float candidate;
  for ( ; ; ) {
          if ( (float)distQueue[0] == Weight::HUGE_FLOAT || nUnknown == 0 ) {
      break;
    }
    int targetState, activeState = distQueue[0].state;
    //    dist[activeState] = (float)distQueue[0];
    heapPop(distQueue, distQueue + nUnknown--);
    List<GraphArc>::const_iterator end = rev[activeState].arcs.end()  ;
    for ( List<GraphArc>::const_iterator a = rev[activeState].arcs.begin() ; a !=end ; ++a ) {
      // future: compare only best arc to any given state
      targetState = a->dest;
      if ( (candidate = a->weight + weights[activeState] )
           < weights[targetState] ) {

        weights[targetState] = candidate;
		if (best)
			best[targetState] = (GraphArc *)a->data;
        heapAdjustUp(distQueue, stateLocations[targetState]);
      }
    }
  }

  if (pathTree)
	for ( i = 0 ; i < nStates ; ++i )
		if ( best[i] )
			pathTree[i].arcs.push(*best[i]);

  delete[] stateLocations;
  delete[] distQueue;
  delete[] rev;
  if (best)
	delete[] best;
}

Graph removeStates(Graph g, bool marked[]) // not tested
// Comment by Yaser: This function creates new GraphState[] and because the
// return Graph points to this newly created Graph, it is NOT deleted. Therefore
// whatever the caller function is responsible for deleting this data.
// It is not a good programming practice but it will be messy to clean it up.
//

{
  int *oldToNew = new int[g.nStates];
  int i = 0, f = 0;
  while ( i < g.nStates )
    if (marked[i])
      oldToNew[i++] = -1;
    else
      oldToNew[i++] = f++;

  GraphState *reduced = new GraphState[f];

  for ( i = 0 ; i < g.nStates ; ++i )
    if ( !marked[i] ) {
      List<GraphArc> &newArcs = reduced[oldToNew[i]].arcs;
       List<GraphArc>::const_iterator end = g.states[i].arcs.end() ;
      for ( List<GraphArc>::const_iterator oldArc=g.states[i].arcs.begin() ; oldArc !=end ; ++oldArc )
        if ( !marked[oldArc->dest] ) {
          GraphArc newArc = *oldArc;
          newArc.dest = oldToNew[newArc.dest];
          newArcs.push(newArc);
        }
    }

  delete[] oldToNew;

  Graph ret;
  ret.nStates = f;
  ret.states = reduced;
  return ret;
}

void printGraph(const Graph g, std::ostream &out)
{
  for ( int i = 0 ; i < g.nStates ; ++i ) {
    out << i;
    List<GraphArc>::const_iterator end = g.states[i].arcs.end();
    for ( List<GraphArc>::const_iterator a=g.states[i].arcs.begin() ; a !=end ; ++a )
      out << (*a);
    out << '\n';
  }
}
