#ifndef GRAEHL_SHARED_KBEST_H
#define GRAEHL_SHARED_KBEST_H

//FIXME: uses globals rather than explicit context struct; not threadsafe

#include <graehl/shared/graph.h>
#include <graehl/shared/myassert.h>
#include <graehl/shared/list.h>
#include <graehl/shared/2hash.h>

//#include "dynarray.h"

namespace graehl {

struct pGraphArc {
    GraphArc *p;
    GraphArc * operator ->() const { return p; }
    operator GraphArc *() const { return p; }
};

inline int operator < (const pGraphArc l, const pGraphArc r) {
    return l->weight > r->weight;
}


struct GraphHeap {
    GraphHeap *left, *right;      // for balanced heap
    int nDescend;
    //  GraphHeap *cross;
    // cross edge is implicitly determined by arc
    GraphArc *arc;                // data at each vertex
    pGraphArc *arcHeap;           // binary heap of sidetracks originating from a state
    int arcHeapSize;

    static GraphHeap *freeList;
    static const int newBlocksize;
    static List<GraphHeap *> usedBlocks;
    // custom new is mandatory, because of how freeAll works (and we do rely on freeAll)!
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

inline int operator < (const GraphHeap &l, const GraphHeap &r) {
    return l.arc->weight > r.arc->weight;
}

struct EdgePath {
    GraphHeap *node;
    int heapPos;                  // <0 if arc is node->arc - otherwise arc is node->arcHeap[heapPos]
    GraphArc *get_cut_arc()  const
    {
        if (heapPos<0)
            return node->arc;
        else
            return node->arcHeap[heapPos];
    }
    
    EdgePath *last;
    FLOAT_TYPE weight;
};

inline int operator < (const EdgePath &l, const EdgePath &r) {
    return l.weight > r.weight;
}


Graph sidetrackGraph(Graph lG, Graph rG, FLOAT_TYPE *dist);
void buildSidetracksHeap(unsigned state, unsigned pred); // call depthfirstsearch with this; see usage in kbest.cc
void freeAllSidetracks(); // must be called after you buildSidetracksHeap
void printTree(GraphHeap *t, int n) ;
void shortPrintTree(GraphHeap *t);

// you can inherit from this or just provide the same interface
struct best_paths_visitor {
    enum make_not_anon_16 { SIDETRACKS_ONLY=0 };
    void start_path(unsigned k,FLOAT_TYPE cost) {} // called with k=rank of path (1-best, 2-best, etc.) and cost=sum of arcs from start to finish.  AGAIN: k starts at 1, not 0
    void end_path() {}
    void visit_best_arc(const GraphArc &a) {} // won't be called if SIDETRACKS_ONLY != 0
    void visit_sidetrack_arc(const GraphArc &a) { visit_best_arc(a); }
};

// you can inherit from this or just provide the same interface
template <bool sidetracks_only=false>
struct BestPathsPrinter {
    std::ostream *pout;
    BestPathsPrinter(std::ostream &o) : pout(&o) {}
    enum make_not_anon_17 { SIDETRACKS_ONLY=sidetracks_only };
    void start_path(unsigned k,FLOAT_TYPE cost) { // called with k=rank of path (1-best, 2-best, etc.) and cost=sum of arcs from start to finish
        *pout<<cost;
    }
    
    void end_path() {
        *pout<<'\n';
    }
    void visit_best_arc(const GraphArc &a) { // won't be called if SIDETRACKS_ONLY != 0
        *pout<<' '<<a;
    }
    void visit_sidetrack_arc(const GraphArc &a) {
        visit_best_arc(a);
        *pout<<'!';
    }
};

static inline void telescope_cost(GraphArc &w,FLOAT_TYPE *dist)
{    
    w.weight = w.weight - (dist[w.src] - dist[w.dest]);
}

static inline void untelescope_cost(GraphArc &w,FLOAT_TYPE *dist)
{    
    w.weight = w.weight + (dist[w.src] - dist[w.dest]);
}

#ifdef GRAEHL__SINGLE_MAIN
# include "kbest.cc"
#else
extern Graph sidetracks;
extern GraphHeap **pathGraph;
extern GraphState *shortPathTree;
#endif


typedef HashTable<GraphArc *,bool> taken_arc_type;

struct best_path_has_cycle : public std::runtime_error
{
    best_path_has_cycle() : std::runtime_error("Best path has a cycle; visiting best arcs (not just sidetracks) in kbest results would give an infinite loop.") {}
};

template <class Visitor>
void insertShortPath(int src, int dest, Visitor &v, taken_arc_type *cycle_detect=NULL)
{
    if (!v.SIDETRACKS_ONLY) {
        if (cycle_detect)
            cycle_detect->clear();
        GraphArc *taken;
        for ( int iState = src ; iState != dest; iState = taken->dest ) {
            taken = &shortPathTree[iState].arcs.top();
            if (cycle_detect && !was_inserted(insert(*cycle_detect,taken,true)))
                throw best_path_has_cycle();
            v.visit_best_arc(*taken);
        }
    }
}


    
// throw_on_cycle, if false, will avoid checking for cycles but could infinitely loop
template <class Visitor>
void bestPaths(Graph graph,unsigned src, unsigned dest,unsigned k,Visitor &v,bool throw_on_cycle=true) {
    unsigned nStates=graph.nStates;
    Assert(nStates > 0 && graph.states);
    Assert(src >= 0 && src < nStates);
    Assert(dest >= 0 && dest < nStates);

    taken_arc_type cycle_detect;
    taken_arc_type * p_cycle_hash=throw_on_cycle ? &cycle_detect : 0;
    
#ifdef DEBUGKBEST
    Config::debug() << "Calling KBest with k: "<<k<<'\n' << graph;
#endif

    FLOAT_TYPE *dist = NEW FLOAT_TYPE[nStates];
    unsigned path_no=1;
    Graph shortPathGraph = shortestPathTreeTo(graph, dest,dist);
    FLOAT_TYPE path_cost;
#ifdef DEBUGKBEST
    Config::debug() << "Shortest path graph ("<<src<<"->"<<dest<<"): "<<k<<'\n' << shortPathGraph;
#endif
    shortPathTree = shortPathGraph.states;
    if ( shortPathTree[src].arcs.notEmpty() || dest == src ) {

        FLOAT_TYPE base_path_cost=dist[src];
        v.start_path(path_no,base_path_cost);
        insertShortPath(src, dest, v, p_cycle_hash);
        v.end_path();

        if ( k > 1 ) {
            GraphHeap::freeAll();
            pathGraph = NEW GraphHeap *[nStates];
            for ( unsigned i = 0 ; i < nStates ; ++i ) pathGraph[i] = 0; // necessary because we may not have reduced (removed states that aren't start->state->finish reachable
            sidetracks = sidetrackGraph(graph, shortPathGraph, dist);
            bool *visited = NEW bool[nStates];
            for ( unsigned i = 0 ; i < nStates ; ++i ) visited[i] = false;
//      freeAllSidetracks();
            Graph revPathTree = reverseGraph(shortPathGraph);
            depthFirstSearch(revPathTree, dest, visited, buildSidetracksHeap); // depthFirstSearch recursively calls the function passed as the last argument (in this  case "buildSidetracksHeap")
            delete[] visited;

            if ( pathGraph[src] ) {
#ifdef DEBUGKBEST
                Config::debug() << "printing trees\n";
                for ( unsigned i = 0 ; i < nStates ; ++i )
                    printTree(pathGraph[i], 0);
                Config::debug() << "done printing trees\n\n";
#endif
                EdgePath *pathQueue = NEW EdgePath[4 * (k+1)];  // out-degree is at most 4
                EdgePath *endQueue = pathQueue;
                EdgePath *retired = NEW EdgePath[k+1];
                EdgePath *endRetired = retired;
                EdgePath newPath;
                newPath.weight = pathGraph[src]->arc->weight;
                newPath.heapPos = -1;
                newPath.node = pathGraph[src];
                newPath.last = NULL;
                heap_add(pathQueue, ++endQueue, newPath);
                while ( heapSize(pathQueue, endQueue) && ++path_no <= k ) {
                    EdgePath *top = pathQueue;
                    GraphArc *cutArc = top->get_cut_arc();
                    typedef List<GraphArc *> Sidetracks;
                    Sidetracks shortPath;
#ifdef DEBUGKBEST
                    Config::debug() << top->weight;
#endif
                    path_cost=base_path_cost;
                    shortPath.push(cutArc);
                    path_cost += cutArc->weight;
#ifdef DEBUGKBEST
                    Config::debug() << ' ' << *cutArc;
#endif
                    EdgePath *last;
                    while ( (last = top->last) ) {
                        if ( !((last->heapPos == -1 && (top->heapPos == 0 || top->node == last->node->left || top->node == last->node->right )) || (last->heapPos >= 0 && top->heapPos != -1 )) ) { // got to p on a cross edge
                            cutArc=last->get_cut_arc();
                            shortPath.push( cutArc);
                            path_cost += cutArc->weight;
#ifdef DEBUGKBEST
                            Config::debug() << ' ' << *cutArc;
#endif
                        }
                        top = last;
                    }
#ifdef DEBUGKBEST
                    Config::debug() << "\n\n";
#endif

                    int srcState = src; // pretend beginning state is end of last sidetrack

                    v.start_path(path_no,path_cost);
                    for ( Sidetracks::const_iterator cut=shortPath.const_begin(),end=shortPath.const_end(); cut != end; ++cut ) {
                        GraphArc *cutarc=*cut;
                        insertShortPath(srcState, cutarc->src, v, p_cycle_hash); // stitch end of last sidetrack to beginning of this one
                        srcState = cutarc->dest;
                        if (!v.SIDETRACKS_ONLY)
                            untelescope_cost(*cutarc,dist);
                        v.visit_sidetrack_arc(*cutarc);
                        if (!v.SIDETRACKS_ONLY)
                            telescope_cost(*cutarc,dist);                        
                    }

                    insertShortPath(srcState, dest, v, p_cycle_hash); // connect end of last sidetrack to dest state

                    v.end_path();

                    *endRetired = pathQueue[0];
                    newPath.last = endRetired++;
                    heapPop(pathQueue, endQueue--);
                    int lastHeapPos = newPath.last->heapPos;
                    GraphArc *spawnVertex;
                    GraphHeap *from = newPath.last->node;
                    FLOAT_TYPE lastWeight = newPath.last->weight;
                    if ( lastHeapPos == -1 ) {
                        spawnVertex = from->arc;
                        newPath.heapPos = -1;
                        if ( from->left ) {
                            newPath.node = from->left;
                            newPath.weight = lastWeight + (newPath.node->arc->weight - spawnVertex->weight);
                            heap_add(pathQueue, ++endQueue, newPath);
                        }
                        if ( from->right ) {
                            newPath.node = from->right;
                            newPath.weight = lastWeight + (newPath.node->arc->weight - spawnVertex->weight);
                            heap_add(pathQueue, ++endQueue, newPath);
                        }
                        if ( from->arcHeapSize ) {
                            newPath.heapPos = 0;
                            newPath.node = from;
                            newPath.weight = lastWeight + (newPath.node->arcHeap[0]->weight - spawnVertex->weight);
                            heap_add(pathQueue, ++endQueue, newPath);
                        }
                    } else {
                        spawnVertex = from->arcHeap[lastHeapPos];
                        newPath.node = from;
                        int iChild = 2 * lastHeapPos + 1;
                        if ( from->arcHeapSize > iChild  ) {
                            newPath.heapPos = iChild;
                            newPath.weight = lastWeight + (newPath.node->arcHeap[iChild]->weight - spawnVertex->weight);
                            heap_add(pathQueue, ++endQueue, newPath);
                            if ( from->arcHeapSize > ++iChild ) {
                                newPath.heapPos = iChild;
                                newPath.weight = lastWeight + (newPath.node->arcHeap[iChild]->weight - spawnVertex->weight);
                                heap_add(pathQueue, ++endQueue, newPath);
                            }
                        }
                    }
                    if ( pathGraph[spawnVertex->dest] ) {
                        newPath.heapPos = -1;
                        newPath.node = pathGraph[spawnVertex->dest];
                        newPath.heapPos = -1;
                        newPath.weight = lastWeight + newPath.node->arc->weight;
                        heap_add(pathQueue, ++endQueue, newPath);
                    }
                } // end of while
                delete[] pathQueue;
                delete[] retired;
            } else {
//                Config::log() << "no more best paths exist.\n";
            }// end of if (pathGraph[0])
            GraphHeap::freeAll(); // FIXME: global
            freeAllSidetracks(); // FIXME: global

            delete[] pathGraph;
            freeGraph(revPathTree);
            freeGraph(sidetracks);
        } // end of if (k > 1)
    }

    freeGraph(shortPathGraph);
    delete[] dist;
    
}

}

#endif
