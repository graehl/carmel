#include "config.h"
#include "graph.h"
#include "myassert.h"

std::ostream & operator << (std::ostream &out, const GraphArc &a)
{
  return out << '(' << a.source << ' ' << a.dest << ' ' << a.weight << ' ' << a.data << ' ' << ')';
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

Graph reverseGraph(Graph g)
  // Comment by Yaser: This function creates NEW GraphState[] and because the
  // return Graph points to this newly created Graph, it is NOT deleted. Therefore
  // whatever the caller function is responsible for deleting this data.
  // It is not a good programming practice but it will be messy to clean it up.
  //

{
  GraphState *rev = NEW GraphState[g.nStates];
  for ( int i  = 0 ; i < g.nStates ; ++i ){
    List<GraphArc> &arcs = g.states[i].arcs;
    for ( List<GraphArc>::val_iterator l=arcs.val_begin(),end = arcs.val_end(); l != end; ++l ) {
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

  const List<GraphArc> &arcs = dfsGraph.states[state].arcs;
  for ( List<GraphArc>::const_iterator l=arcs.const_begin(),end = arcs.const_end(); l != end; ++l ) {
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
  for ( List<int>::const_iterator t=topo.const_begin(),end = topo.const_end() ; t != end; ++t ){
    const List<GraphArc> &arcs = g.states[*t].arcs;
    for ( List<GraphArc>::const_iterator a=arcs.const_begin(),end=arcs.const_end() ; a !=end ; ++a )
      nPaths[a->dest] += nPaths[(*t)];
  }
}


FLOAT_TYPE *DistToState::weights = NULL;
DistToState **DistToState::stateLocations = NULL;
FLOAT_TYPE DistToState::unreachable = HUGE_FLOAT;

inline bool operator < (DistToState lhs, DistToState rhs) {
  return DistToState::weights[lhs.state] > DistToState::weights[rhs.state];
}

inline bool operator == (DistToState lhs, DistToState rhs) {
  return DistToState::weights[lhs.state] == DistToState::weights[rhs.state];
}

inline bool operator == (DistToState lhs, FLOAT_TYPE rhs) {
  return DistToState::weights[lhs.state] == rhs;
}

Graph shortestPathTreeTo(Graph g, int dest, FLOAT_TYPE *dist)
  // returns graph (need to delete[] ret.states yourself)
  // computes best paths from all states to single destination, storing tree of arcs taken in *pathTree, and distances to dest in *dist
{
  int i;

  GraphArc **taken = NEW  /*const*/ GraphArc *[g.nStates];

  Graph pg;
  pg.nStates = g.nStates;
  pg.states = NEW GraphState[pg.nStates];

  Graph rev_graph = reverseGraph(g);
#ifdef DEBUGKBEST
  Config::debug() << "rev_graph = \n" << rev_graph << "\n\nTaken\n";
#endif
  shortestDistancesFrom(rev_graph,dest,dist,taken);

  for ( i = 0 ; i < g.nStates ; ++i )
    if ( taken[i] ) {
      GraphArc * rev_taken = (GraphArc *)taken[i]->data;
#ifdef DEBUGKBEST
      Config::debug() << ' ' << i << ' ' << rev_taken;
#endif

      pg.states[i].arcs.push(*rev_taken);
    }


#ifdef DEBUGKBEST
  Config::debug() << "shortestpathtree graph:\n" << pg;
#endif

  delete[] rev_graph.states;
  delete[] taken;
  return pg;
}

void shortestDistancesFrom(Graph g, int source, FLOAT_TYPE *dist,GraphArc **taken)
  // computes best paths from single source to all other states
  // if taken == NULL, only compute weights (stored in dist)
  //  otherwise, store pointer to arc taken to get to state s in taken[s]
{
  int nStates = g.nStates;
  int i;

  if (taken)
    for ( i = 0 ; i < g.nStates ; ++i )
      taken[i] = NULL;

  GraphState *st = g.states;

  int nUnknown = nStates;

  DistToState *distQueue = NEW DistToState[nStates];

  //  FLOAT_TYPE *weights = NEW FLOAT_TYPE[nStates];
  FLOAT_TYPE *weights = dist;

  for ( i = 0 ; i < nStates ; ++i ) {
    weights[i] = HUGE_FLOAT;
  }

  DistToState **stateLocations = NEW DistToState *[nStates];
  DistToState::weights = weights;
  DistToState::stateLocations = stateLocations;

  weights[source] = 0;
  for ( i = 1; i < nStates ; ++i ) {
    int fillWith;
    if ( i <= source )
      fillWith = i-1;
    else
      fillWith = i;
    distQueue[i].state = fillWith;
    stateLocations[fillWith] = &distQueue[i];
  }
  distQueue[0].state = source;
  stateLocations[source] = &distQueue[0];


  FLOAT_TYPE candidate;
  for ( ; ; ) {
    if ( (FLOAT_TYPE)distQueue[0] == HUGE_FLOAT || nUnknown == 0 ) {
      break;
    }
    int activeState = distQueue[0].state;
    //    dist[activeState] = (FLOAT_TYPE)distQueue[0];
    heapPop(distQueue, distQueue + nUnknown--);
    List<GraphArc> &arcs=st[activeState].arcs;
    for ( List<GraphArc>::val_iterator a = arcs.val_begin(),end=arcs.val_end() ; a !=end ; ++a ) {
      // future: compare only best arc to any given state
      int targetState = a->dest;
      if ( (candidate = (a->weight + weights[activeState])) < weights[targetState] ) {
        weights[targetState] = candidate;
        if (taken)
          taken[targetState] = &(*a);
        heapAdjustUp(distQueue, stateLocations[targetState]);
      }
    }
  }


  delete[] stateLocations;
  delete[] distQueue;
  //delete[] rev;

}

Graph removeStates(Graph g, bool marked[]) // not tested
  // Comment by Yaser: This function creates NEW GraphState[] and because the
  // return Graph points to this newly created Graph, it is NOT deleted. Therefore
  // whatever the caller function is responsible for deleting this data.
  // It is not a good programming practice but it will be messy to clean it up.
  //

{
  unsigned *oldToNew = NEW unsigned[g.nStates];
  int i = 0, f = 0;
  while ( i < g.nStates )
    if (!marked[i])
      oldToNew[i++] = f++;

  GraphState *reduced = NEW GraphState[f];

  for ( i = 0 ; i < g.nStates ; ++i )
    if ( !marked[i] ) {
      List<GraphArc> &newArcs = reduced[oldToNew[i]].arcs;
      const List<GraphArc> &arcs = g.states[i].arcs;
      for ( List<GraphArc>::const_iterator oldArc=arcs.const_begin(),end=arcs.const_end() ; oldArc !=end ; ++oldArc )
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
  out << "(Graph " << g.nStates << std::endl;
  for ( int i = 0 ; i < g.nStates ; ++i ) {
    out << i;
    const List<GraphArc> &arcs = g.states[i].arcs;
    for ( List<GraphArc>::const_iterator a=arcs.const_begin(),end=arcs.const_end() ; a !=end ; ++a )
      out << ' ' << (*a);
    out << std::endl;
  }
  out << ")" << std::endl;
}
