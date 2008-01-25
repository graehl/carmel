// simple adjacency graph with path-additive weight/cost (lower is better, 0 is no cost, +INF is infinite cost)

#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>
#include <vector>

#include <graehl/shared/2heap.h>
#include <graehl/shared/list.h>
#include <iterator>

namespace graehl {

static const unsigned DFS_NO_PREDECESSOR=(unsigned)-1;

struct GraphArc {
  int source;
  int dest;
  FLOAT_TYPE weight;
  void *data;
    template <class T>
    T &data_as() 
    {
        return reinterpret_cast<T &>(data);
    }
    template <class T>
    T const&data_as() const
    {
        return reinterpret_cast<T const&>(data);
    }
    
    GraphArc() {}
    GraphArc(int source,int dest,FLOAT_TYPE weight,void *data) :
        source(source),dest(dest),weight(weight),data(data)
    {}
};

std::ostream & operator << (std::ostream &out, const GraphArc &a);

struct GraphState {
    typedef  List<GraphArc> arcs_type;
    arcs_type arcs;
    void add(GraphArc const& a) 
    {
        arcs.push(a);
    }
    void add(int source,int dest,FLOAT_TYPE weight,void *data) 
    {
        /*
        GraphArc a;
        a.source=source;
        a.dest=dest;
        a.weight=weight;
        a.data=data;
        add(a);
        */
        arcs.push_front(source,dest,weight,data);
    }
};


struct Graph {
  GraphState *states;
  unsigned nStates;
};

// take the arcs in graph (source,n_states) and add them, reversing source<->dest, to destination graph (destination,n_states)
void add_reversed_arcs(GraphState *destination,GraphState const*source,unsigned n_states);

Graph reverseGraph(Graph g) ;

extern Graph dfsGraph;
extern bool *dfsVis;

void dfsRec(unsigned state, unsigned pred);

void depthFirstSearch(Graph graph, unsigned startState, bool* visited, void (*func)(unsigned state, unsigned pred));

template <class Weight>
void countNoCyclePaths(Graph g, Weight *nPaths, int source);

class TopoSort {
  typedef std::front_insert_iterator<List<int> > IntPusher;
  Graph g;
  bool *done;
  bool *begun;
  IntPusher o;
  int n_back_edges;
 public:

  TopoSort(Graph g_, List<int> *l) : g(g_), o(*l), n_back_edges(0) {
    done = NEW bool[g.nStates];
    begun = NEW bool[g.nStates];
    for (unsigned i=0;i<g.nStates;++i)
      done[i]=begun[i]=false;
  }
  void order_all() {
    order(true);
  }
  void order_crucial() {
    order(false);
  }
  void order(bool all=true) {
    for ( unsigned i = 0 ; i < g.nStates ; ++i )
      if ( all || !g.states[i].arcs.empty() )
    order_from(i);
  }
  int get_n_back_edges() const { return n_back_edges; }
  void order_from(int s) {
    if (done[s]) return;
    if (begun[s]) { // this state previously started but not finished already = back edge/cycle!
      ++n_back_edges;
      return;
    }
    begun[s]=true;
    const List<GraphArc> &arcs = g.states[s].arcs;
    for ( List<GraphArc>::const_iterator l=arcs.const_begin(),end=arcs.const_end() ; l !=end ; ++l ) {
      order_from(l->dest);
    }
    done[s]=true;

    // insert at beginning of sorted list (we must come before anything reachable by us!)
    *o++ = s;
  }
  ~TopoSort() { delete[] done; delete[] begun; }
};


// serves as adjustable heap (tracks where each state is, and its weight)
struct DistToState {
  int state;
  static DistToState **stateLocations;
  static FLOAT_TYPE *weights;
  static FLOAT_TYPE unreachable;
  operator FLOAT_TYPE() const { return weights[state]; }
  void operator = (DistToState rhs) {
    stateLocations[rhs.state] = this;
    state = rhs.state;
  }
};

Graph shortestPathTreeTo(Graph g, unsigned dest, FLOAT_TYPE *dist);
// returns graph (need to delete[] ret.states yourself)
// computes best paths from all states to single destination, storing tree of arcs taken in *pathTree, and distances to dest in *dist

void shortestDistancesFrom(Graph g, unsigned source, FLOAT_TYPE *dist,GraphArc **taken=NULL);
// computes best paths from single source to all other states
// if taken == NULL, only compute weights (stored in dist)
//  otherwise, store pointer to arc taken to get to state s in taken[s]


// if marked[i], remove state i and arcs leading to it
Graph removeStates(Graph g, bool marked[]); // not tested

struct rewrite_GraphState
{
    template <class OldToNew>
    void rewrite_arc(GraphArc &a,OldToNew const& m) const 
    {
        a.source=m[a.source];
        a.dest=m[a.dest];
    }

    // OldToNew[i] = i -> this new index if keeping i, (unsigned)-1 if removing i
    template <class OldToNew>
    void operator()(GraphState &s,OldToNew const& t) const 
    {
        typedef GraphState::arcs_type A;
        
        for (A::erase_iterator i=s.arcs.begin(),e=s.arcs.end();i!=e;) {
            if (t[i->dest]==(unsigned)-1) {
                i=s.arcs.erase(i);
            } else {
                i->source=t[i->source];
                i->dest=t[i->dest];
                ++i;
            }
        }
    }

    void operator()(GraphState &s) 
    {
        s.arcs.clear();
    }
};

// as removeStates(..), but graph g is modified by swapping deleted states to the end (and clearing their arcs), returns number of states remaining
unsigned removeStates_inplace(Graph g,bool marked[]);


inline void freeGraph(Graph graph) {
    delete[] graph.states;
}

void printGraph(Graph g, std::ostream &out);

inline std::ostream & operator << (std::ostream &out, const Graph &g) {
  printGraph(g,out);
  return out;
}

template <class Weight>
void countNoCyclePaths(Graph g, Weight *nPaths, int source) {
  List<int> topo;

  TopoSort sort(g,&topo);
  sort.order_from(source);

  for ( unsigned i = 0 ; i < g.nStates; ++i )
    nPaths[i]=0;
  nPaths[source] = 1;
  for ( List<int>::const_iterator t=topo.const_begin(),end = topo.const_end() ; t != end; ++t ){
    const List<GraphArc> &arcs = g.states[*t].arcs;
    for ( List<GraphArc>::const_iterator a=arcs.const_begin(),end=arcs.const_end() ; a !=end ; ++a )
      nPaths[a->dest] += nPaths[(*t)];
  }
}

template <class Weight>
Weight countNoCyclePaths(Graph g, int source, int dest) 
{
    Weight *w=new Weight[g.nStates];
    countNoCyclePaths(g,w,source);
    Weight wd=w[dest];
    delete w;
    return wd;    
}

}


#ifdef GRAEHL__SINGLE_MAIN
# include "graph.cc"
#endif
#endif
