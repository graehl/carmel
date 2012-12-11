// simple adjacency graph with path-additive weight/cost (lower is better, 0 is no cost, +INF is infinite cost)

#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>
#include <vector>
#include <iterator>

#include <graehl/shared/dynarray.h>
#include <graehl/shared/config.h>
#include <graehl/shared/weight.h>
#include <graehl/shared/2heap.h>
#include <graehl/shared/list.h>
#include <graehl/shared/push_backer.hpp>

//#include <boost/serialization/access.hpp>

namespace graehl {

static const unsigned DFS_NO_PREDECESSOR=(unsigned)-1;

struct GraphArc {
  int src;
  int dest;
    WEIGHT_FLOAT_TYPE weight;
    Weight & wt() const
    {
        return *(Weight *)&weight;
    }
    /*
    Weight const& wt() const
    {
        return *(Weight const*)&weight;
        }*/

  void *data;
    template <class T>
    T &data_as()
    {
        return *(T*)(&data);
    }
    /// note: not safe to use with integral types if you construct as void * data ... only safe if you set with data_as()
    template <class T>
    T const&data_as() const
    {
        return *(T const*)(&data);
    }

    GraphArc() {}
    GraphArc(int src,int dest,FLOAT_TYPE weight,void *data) :
        src(src),dest(dest),weight(weight),data(data)
    {}
    GraphArc(int src,int dest,FLOAT_TYPE weight) :
        src(src),dest(dest),weight(weight)
    {}
// private:
// friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive & ar, const unsigned int version=0)
    { ar & src & dest & weight & data; }
};

std::ostream & operator << (std::ostream &out, const GraphArc &a);

struct GraphState {
    typedef  List<GraphArc> arcs_type;
    arcs_type arcs;

    template <class Archive>
    void serialize(Archive & ar, const unsigned int version=0)
    { ar & arcs; }

    void add(GraphArc const& a)
    {
        arcs.push(a);
    }
    std::size_t outdegree() const
    {
        return arcs.size();
    }

    template <class T>
    void add_data_as(int src,int dest,FLOAT_TYPE weight, T const& data)
    {
        arcs.push_front(src,dest,weight);
        arcs.front().template data_as<T>()=data;
    }

    void add(int src,int dest,FLOAT_TYPE weight, void *data)
    {
        arcs.push_front(src,dest,weight,data);
    }

    void add(int src,int dest,FLOAT_TYPE weight)
    {
        arcs.push_front(src,dest,weight);
    }
    template <class W>
    void setwt(W const& w)
    {
        for ( List<GraphArc>::val_iterator i=arcs.val_begin(),end=arcs.val_end() ; i !=end ; ++i ) {
            GraphArc &a=*i;
            a.wt()=w(a);
        }
    }

    typedef dynamic_array<WEIGHT_FLOAT_TYPE> saved_weights_t;
    void save_weights(saved_weights_t &s) const
    {
        for ( List<GraphArc>::const_iterator i=arcs.const_begin(),end=arcs.const_end() ; i !=end ; ++i )
            s.push_back(i->weight);
    }
    unsigned restore_weights(saved_weights_t const& s,unsigned start=0)
    {
        for ( List<GraphArc>::val_iterator i=arcs.val_begin(),end=arcs.val_end() ; i !=end ; ++i ) {
            GraphArc &a=*i;
            a.weight=s[start++];
        }
        return start;
    }
};

inline void swap(GraphState &a,GraphState &b)
{
    a.arcs.swap(b.arcs);
}


struct Graph {
    GraphState *states;
    unsigned nStates;
    template <class W>
    void setwt(W const& w)
    {
        for (unsigned i=0;i<nStates;++i)
            states[i].setwt(w);
    }
    typedef GraphState::saved_weights_t saved_weights_t;
    void save_weights(saved_weights_t &s) const
    {
        for (unsigned i=0;i<nStates;++i)
            states[i].save_weights(s);

    }
    unsigned restore_weights(saved_weights_t const& s,unsigned start=0)
    {
        for (unsigned i=0;i<nStates;++i)
            start=states[i].restore_weights(s,start);
        return start;
    }
};

// take the arcs in graph (src,n_states) and add them, reversing src<->dest, to destination graph (destination,n_states).  copy data field unless data_point_to_forward, in which case point to original (forward) arc
void add_reversed_arcs(GraphState *destination,GraphState const*src,unsigned n_states,bool data_point_to_forward=false);

Graph reverseGraph(Graph g,bool data_point_to_forward=true) ;

extern Graph dfsGraph;
extern bool *dfsVis;

void dfsRec(unsigned state, unsigned pred);

void depthFirstSearch(Graph graph, unsigned startState, bool* visited, void (*func)(unsigned state, unsigned pred));

template <class Weight>
void countNoCyclePaths(Graph g, Weight *nPaths, int src);


struct backref
{
    unsigned uses; // if >1, then assign id
    unsigned id;
    backref() : uses(),id()
    {}
    bool use(unsigned &nextid)
    {
        if (uses++>0) {
            id=nextid++;
            return false;
        }
        return true;
    }
};

struct backrefs
{
    fixed_array<backref> ids;
    Graph g;
    unsigned nextid;
    backrefs(Graph g,unsigned root,unsigned startid=1) : g(g),ids(g.nStates),nextid(startid)
    {
        use(root);
    }
    void use(unsigned s)
    {
        if (ids[s].use(nextid))
            order_from(s);
    }
    void order_from(unsigned s)
    {
        const List<GraphArc> &arcs = g.states[s].arcs;
        for ( List<GraphArc>::const_iterator l=arcs.const_begin(),end=arcs.const_end() ; l !=end ; ++l ) {
            use(l->dest);
        }
    }
};


class TopoSort {
  typedef std::front_insert_iterator<List<int> > IntPusher;
  Graph g;
  bool *done;
  bool *begun;
  IntPusher o;
  int n_back_edges;
 public:
    bool has_cycle() const
    {
        return n_back_edges;
    }
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

struct reverse_topo_order {
  Graph g;
  bool *done;
  bool *begun;
  unsigned n_back_edges;
 public:

  reverse_topo_order(Graph g_) : g(g_), n_back_edges(0) {
    done = NEW bool[g.nStates];
    begun = NEW bool[g.nStates];
    for (unsigned i=0;i<g.nStates;++i)
      done[i]=begun[i]=false;
  }
    template <class O>
  void order_all(O o) {
        order(o,true);
  }
    template <class O>
  void order_crucial(O o) {
        order(o,false);
  }
    template <class O>
    void order(O o,bool all=true) {
    for ( unsigned i = 0 ; i < g.nStates ; ++i )
      if ( all || !g.states[i].arcs.empty() )
          order_from(o,i);
  }
  unsigned get_n_back_edges() const { return n_back_edges; }
    template <class O>
    void order_from(O o,int s) {
        if (done[s]) return;
        if (begun[s]) { // this state previously started but not finished already = back edge/cycle!
            ++n_back_edges;
            return;
        }
        begun[s]=true;
        const List<GraphArc> &arcs = g.states[s].arcs;
        for ( List<GraphArc>::const_iterator l=arcs.const_begin(),end=arcs.const_end() ; l !=end ; ++l ) {
            order_from(o,l->dest);
        }
        done[s]=true;

        // insert at beginning of sorted list (we must come before anything reachable by us!)
        o(s);
  }
  ~reverse_topo_order() { delete[] done; delete[] begun; }
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

void shortestDistancesFrom(Graph g, unsigned src, FLOAT_TYPE *dist,GraphArc **taken=NULL);
// computes best paths from single src to all other states
// if taken == NULL, only compute weights (stored in dist)
//  otherwise, store pointer to arc taken to get to state s in taken[s]


// if marked[i], remove state i and arcs leading to it
Graph removeStates(Graph g, bool marked[]); // not tested

struct rewrite_GraphState
{
    unsigned n_kept;
    rewrite_GraphState() : n_kept(0)
    {}

    template <class OldToNew>
    void rewrite_arc(GraphArc &a,OldToNew const& m) const
    {
        a.src=m[a.src];
        a.dest=m[a.dest];
    }

    // OldToNew[i] = i -> this new index if keeping i, (unsigned)-1 if removing i
    template <class OldToNew>
    void operator()(GraphState &s,OldToNew const& t)
    {
        typedef GraphState::arcs_type A;

        for (A::erase_iterator i=s.arcs.begin(),e=s.arcs.end();i!=e;) {
            if (t[i->dest]==(unsigned)-1) {
                i=s.arcs.erase(i);
            } else {
                ++n_kept;
                i->src=t[i->src];
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
void countNoCyclePaths(Graph g, Weight *nPaths, int src,unsigned *p_n_back_edges=0) {
  List<int> topo;

  TopoSort sort(g,&topo);
  sort.order_from(src);
  if (p_n_back_edges) *p_n_back_edges=sort.get_n_back_edges();
  for ( unsigned i = 0 ; i < g.nStates; ++i )
    nPaths[i]=0;
  nPaths[src] = 1;
  for ( List<int>::const_iterator t=topo.const_begin(),end = topo.const_end() ; t != end; ++t ){
      unsigned src=*t;
      const List<GraphArc> &arcs = g.states[src].arcs;
      for ( List<GraphArc>::const_iterator a=arcs.const_begin(),end=arcs.const_end() ; a !=end ; ++a )
          nPaths[a->dest] += nPaths[src];
  }
}

template <class Weight>
Weight countNoCyclePathsTo(Graph g, int src, int dest,unsigned *p_n_back_edges=0)
{
    Weight *w=new Weight[g.nStates];
    countNoCyclePaths(g,w,src,p_n_back_edges);
    Weight wd=w[dest];
    delete[] w;
    return wd;
}

// w is an array with as many entries as g has states.   w[start] is nonzero

template <class Weight_get,class Weight_array,class Order>
void propagate_paths_in_order(Graph g,Order t,Order const& t_order_end,Weight_get const& getwt,Weight_array& w)
{
  for (  ; t != t_order_end; ++t ){
      unsigned src=*t;
      const List<GraphArc> &arcs = g.states[src].arcs;
      for ( List<GraphArc>::const_iterator i=arcs.const_begin(),end=arcs.const_end() ; i !=end ; ++i ) {
          GraphArc const& a=*i;
          w[a.dest] += w[src] * getwt(a);
      }
  }
}

struct get_wt
{
    Weight const& operator()(GraphArc const& a) const
    {
        return a.wt();
    }
};

template <class Weight_array,class Order>
void propagate_paths_in_order_wt(Graph g,Order t,Order const& t_order_end,Weight_array& w)
{
    propagate_paths_in_order(g,t,t_order_end,get_wt(),w);
}

template <class Weight_get,class Weight_array>
void propagate_paths(Graph g,Weight_get const& getwt,Weight_array& w,unsigned start)
{
    dynamic_array<unsigned> rev;
    reverse_topo_order r(g);
    r.order_from(make_push_backer(rev),start);
    propagate_paths_in_order(g,rev.rbegin(),rev.rend(),getwt,w);
/*
  List<int> topo;
  TopoSort sort(g,&topo);
  sort.order_from(start);
  propagate_paths_in_order(g,topo.begin(),topo.end(),getwt,w);
*/
}


}


#ifdef GRAEHL__SINGLE_MAIN
# include "graph.cc"
#endif
#endif
