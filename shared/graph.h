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

typedef List<GraphArc>::iterator GAIT;

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
	    GAIT end = dfsGraph.states[s].arcs.end();
		for ( GAIT l=dfsGraph.states[s].arcs.begin() ; l !=end ; ++l ) {
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

void shortestPathTree(Graph g, GraphState *pathTree,int dest, float *dist);
// computes best paths from each state to a single destination
// if pathTree == NULL, only compute weights (stored in dist)
//  otherwise, store arc used for state s in pathTree[s]

Graph removeStates(Graph g, bool marked[]); // not tested

void printGraph(Graph g, std::ostream &out);

#endif
