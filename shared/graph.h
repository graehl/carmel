#ifndef GRAPH_H
#define GRAPH_H 1
#include "config.h"

#include <iostream>
#include <vector>

#include "2heap.h"
#include "list.h"
#include "weight.h"
#include <iterator>

struct GraphArc {
  int source;
  int dest;
  float weight;
  void *data;
};

std::ostream & operator << (std::ostream &out, const GraphArc &a);

struct GraphState {
  List<GraphArc> arcs;
};

struct Graph {
  GraphState *states;
  int nStates;
};


Graph reverseGraph(Graph g) ;

extern Graph dfsGraph;
extern bool *dfsVis;

void dfsRec(int state, int pred);

void depthFirstSearch(Graph graph, int startState, bool* visited, void (*func)(int state, int pred));

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
		done = new bool[g.nStates]; 
		begun = new bool[g.nStates];
		for (int i=0;i<g.nStates;++i) 
			done[i]=begun[i]=false;
	}
	void order_all() {
		order(true);
	}
	void order_crucial() {
		order(false);
	}
	void order(bool all=true) {
		for ( int i = 0 ; i < g.nStates ; ++i )
			if ( all || g.states[i].arcs.size() )
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
		for ( List<GraphArc>::const_iterator l=arcs.begin(),end=arcs.end() ; l !=end ; ++l ) {
			order_from(l->dest);
		}
		done[s]=true;

		// insert at beginning of sorted list (we must come before anything reachable by us!)
		*o++ = s;
	}
	~TopoSort() { delete[] done; delete[] begun; }
};


struct DistToState {
  int state;
  static DistToState **stateLocations;
  static float *weights;
  static float unreachable;
  operator float() const { return weights[state]; }
  void operator = (DistToState rhs) { 
    stateLocations[rhs.state] = this;
    state = rhs.state;
  }
};


inline bool operator < (DistToState lhs, DistToState rhs);

inline bool operator == (DistToState lhs, DistToState rhs);

inline bool operator == (DistToState lhs, float rhs);

Graph shortestPathTreeTo(Graph g, int dest, float *dist);
	// returns graph (need to delete[] ret.states yourself)
	// computes best paths from all states to single destination, storing tree of arcs taken in *pathTree, and distances to dest in *dist

void shortestDistancesFrom(Graph g, int source, float *dist,GraphArc **taken=NULL);
// computes best paths from single source to all other states
// if taken == NULL, only compute weights (stored in dist)
//  otherwise, store pointer to arc taken to get to state s in taken[s]


Graph removeStates(Graph g, bool marked[]); // not tested

void printGraph(Graph g, std::ostream &out);

inline std::ostream & operator << (std::ostream &out, const Graph &g) {
	printGraph(g,out);
	return out;
}

#endif
