#include <cmath>
//static const FLOAT_TYPE HUGE_FLOAT = (HUGE_VAL*HUGE_VAL);
#include "graph.h"
#include "myassert.h"
//#include <dynarray.h>
#include <array.hpp>

namespace graehl {

std::ostream & operator << (std::ostream &out, const GraphArc &a)
{
    out << '(' << a.src <<"->"<< a.dest << ' ' << a.weight;
    //out << ' ' << a.data;
    return out << ')';
}

void (*dfsFunc)(unsigned, unsigned) = NULL;
void (*dfsExitFunc)(unsigned, unsigned) = NULL;

void depthFirstSearch(Graph graph, unsigned startState, bool* visited, void (*func)(unsigned state, unsigned pred)) {
  dfsGraph = graph;
  dfsVis = visited;
  dfsFunc = func;
  dfsExitFunc = NULL;
  dfsRec(startState, DFS_NO_PREDECESSOR);
}

// g's arcs are reversed and added to graph dest
void add_reversed_arcs(GraphState *rev,GraphState const*src,unsigned n,bool data_point_to_forward)
{
    for ( unsigned i  = 0 ; i < n ; ++i ){
        List<GraphArc> const&arcs = src[i].arcs;
        for ( List<GraphArc>::const_iterator l=arcs.begin(),end = arcs.end(); l != end; ++l ) {
/*            GraphArc r;
            r.data = &(*l);
            Assert(i == l->src);
            r.dest = i;
            r.src = l->dest;
            r.weight = l->weight;
            rev[r.src].arcs.push(r);
*/
            List<GraphArc> & d=rev[l->dest].arcs;
            d.push_front(l->dest,l->src,l->weight,data_point_to_forward?(void*)&*l:(void*)l->data);
        }
    }
}


Graph reverseGraph(Graph g,bool data_point_to_forward)
  // This function creates NEW GraphState[] and because the
  // return Graph points to this newly created Graph, it is NOT deleted. Therefore
  // whatever the caller function is responsible for deleting this data.
  //

{
    Graph ret;
    ret.states = NEW GraphState[g.nStates];
    ret.nStates = g.nStates;
    add_reversed_arcs(ret.states,g.states,g.nStates,data_point_to_forward);
    return ret;
}

Graph dfsGraph;
bool *dfsVis;


void dfsRec(unsigned state, unsigned pred) {

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







FLOAT_TYPE *DistToState::weights = NULL;
DistToState **DistToState::stateLocations = NULL;
FLOAT_TYPE DistToState::unreachable = HUGE_VAL;

inline bool operator < (DistToState lhs, DistToState rhs) {
  return DistToState::weights[lhs.state] > DistToState::weights[rhs.state];
}

inline bool operator == (DistToState lhs, DistToState rhs) {
  return DistToState::weights[lhs.state] == DistToState::weights[rhs.state];
}

inline bool operator == (DistToState lhs, FLOAT_TYPE rhs) {
  return DistToState::weights[lhs.state] == rhs;
}

Graph shortestPathTreeTo(Graph g, unsigned dest, FLOAT_TYPE *dist)
  // returns graph (need to delete[] ret.states yourself)
  // computes best paths from all states to single destination, storing tree of arcs taken in *pathTree, and distances to dest in *dist
{
  unsigned i;

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
        GraphArc * rev_taken = taken[i]->data_as<GraphArc *>();
#ifdef DEBUGKBEST
      Config::debug() << ' ' << i << ' ' << rev_taken;
#endif

      pg.states[i].arcs.push(*rev_taken);
    }


#ifdef DEBUGKBEST
  Config::debug() << "\nshortestpathtree graph:\n" << pg;
#endif

  delete[] rev_graph.states;
  delete[] taken;
  return pg;
}

void shortestDistancesFrom(Graph g, unsigned src, FLOAT_TYPE *dist,GraphArc **taken)
  // computes best paths from single src to all other states
  // if taken == NULL, only compute weights (stored in dist)
  //  otherwise, store pointer to arc taken to get to state s in taken[s]
{
  unsigned nStates = g.nStates;
  unsigned i;

  if (taken)
    for ( i = 0 ; i < g.nStates ; ++i )
      taken[i] = NULL;

  GraphState *st = g.states;

  int nUnknown = nStates;

  DistToState *distQueue = NEW DistToState[nStates];

  //  FLOAT_TYPE *weights = NEW FLOAT_TYPE[nStates];
  FLOAT_TYPE *weights = dist;

  for ( i = 0 ; i < nStates ; ++i ) {
    weights[i] = HUGE_VAL;
  }

  DistToState **stateLocations = NEW DistToState *[nStates];
  DistToState::weights = weights;
  DistToState::stateLocations = stateLocations;

  weights[src] = 0;
  for ( i = 1; i < nStates ; ++i ) {
    int fillWith;
    if ( i <= src )
      fillWith = i-1;
    else
      fillWith = i;
    distQueue[i].state = fillWith;
    stateLocations[fillWith] = &distQueue[i];
  }
  distQueue[0].state = src;
  stateLocations[src] = &distQueue[0];


  FLOAT_TYPE candidate;
  for ( ; ; ) {
    if ( (FLOAT_TYPE)distQueue[0] == HUGE_VAL || nUnknown == 0 ) {
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


unsigned removeStates_inplace(Graph g,bool marked[])
{
    indices_after_removing ttable(marked,marked+g.nStates);
    rewrite_GraphState r;
    return graehl::shuffle_removing(g.states,ttable,r);
}



Graph removeStates(Graph g, bool marked[]) // not tested
  // This function creates NEW GraphState[] and because the
  // return Graph points to this newly created Graph, it is NOT deleted. Therefore
  // whatever the caller function is responsible for deleting this data.
  //

{
  unsigned *oldToNew = NEW unsigned[g.nStates];
  unsigned i = 0, f = 0;
  while ( i < g.nStates )
    if (!marked[i])
      oldToNew[i++] = f++;

  GraphState *reduced = NEW GraphState[f];

  for ( i = 0 ; i < g.nStates ; ++i )
    if ( !marked[i] ) {
        GraphState &new_state=reduced[oldToNew[i]];
        List<GraphArc> const&arcs = g.states[i].arcs;
        for ( List<GraphArc>::const_iterator oldArc=arcs.begin(),end=arcs.end() ; oldArc !=end ; ++oldArc )
            if ( !marked[oldArc->dest] )
                new_state.add(oldToNew[oldArc->dest],oldToNew[oldArc->src],oldArc->weight,oldArc->data);
    }

  delete[] oldToNew;

  Graph ret;
  ret.nStates = f;
  ret.states = reduced;
  return ret;
}

void printGraph(const Graph g, std::ostream &out)
{
  out << "(Graph #V=" << g.nStates << std::endl;
  for ( unsigned i = 0 ; i < g.nStates ; ++i ) {
    out << i;
    const List<GraphArc> &arcs = g.states[i].arcs;
    for ( List<GraphArc>::const_iterator a=arcs.const_begin(),end=arcs.const_end() ; a !=end ; ++a )
      out << ' ' << (*a);
    out << std::endl;
  }
  out << ")" << std::endl;
}

std::istream & operator >> (std::istream &istr, GraphArc &a)
{
  char c;
  int i;
  istr >> c;			// open paren
  istr >> a.src;
  istr >> a.dest;
  istr >> a.weight;
  istr >> c;			// close paren
  a.data = NULL;
  return istr;
}

std::istream & operator >> (std::istream &istr, GraphState &s)
{
  char c;
  return istr;
}

std::istream & operator >> (std::istream &istr, Graph &g)
{
  char c;
  GraphArc a;
  istr >> g.nStates;
  if ( istr && g.nStates > 0 )
    g.states = new GraphState[g.nStates];
  else
    g.states = NULL;
  for ( ; ; ) {
    istr >> c;
    if ( !istr || c != '(')
      break;
    istr.putback(c);
    istr >> a;
    if ( !(istr && a.src >= 0 && a.src < g.nStates) )
      break;
    g.states[a.src].arcs.push(a);
  }
  return istr;
}

}
