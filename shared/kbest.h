#ifndef KBEST_H
#define KBEST_H 1

#include "graph.h"
#include "myassert.h"
#include "list.h"
#include "dynarray.h"
//#include "arc.h"

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
    // custom new is mandatory, because of how freeAll works!
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
    int heapPos;                  // -1 if arc is GraphHeap.arc
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

extern Graph sidetracks;
extern GraphHeap **pathGraph;
extern GraphState *shortPathTree;


template <class Visitor>
void insertShortPath(int source, int dest, Visitor &v)
{
    GraphArc *taken;
    for ( int iState = source ; iState != dest; iState = taken->dest ) {
        taken = &shortPathTree[iState].arcs.top();
        v.visit_best_arc(*taken);
    }
}

// you can inherit from this or just provide the same interface
struct BestPathsVisitor {
    enum { SIDETRACKS_ONLY=0 };
    void start_path(unsigned k,FLOAT_TYPE cost) {} // called with k=rank of path (1-best, 2-best, etc.) and cost=sum of arcs from start to finish
    void end_path() {}
    void visit_best_arc(const GraphArc &a) {} // won't be called if SIDETRACKS_ONLY != 0
    void visit_sidetrack_arc(const GraphArc &a) { visit_best_arc(a); }
};


template <class Visitor>
void bestPaths(Graph graph,unsigned source, unsigned dest,unsigned k,Visitor &v) {
    unsigned nStates=graph.nStates;
    Assert(nStates > 0 && graph.states);
    Assert(source >= 0 && source < nStates);
    Assert(dest >= 0 && dest < nStates);

#ifdef DEBUGKBEST
    Config::debug() << "Calling KBest with k: "<<k<<'\n' << graph;
#endif

    FLOAT_TYPE *dist = NEW FLOAT_TYPE[nStates];
    unsigned path_no=1;
    Graph shortPathGraph = shortestPathTreeTo(graph, dest,dist);
    FLOAT_TYPE path_cost;
#ifdef DEBUGKBEST
    Config::debug() << "Shortest path graph: "<<k<<'\n' << shortPathGraph;
#endif
    shortPathTree = shortPathGraph.states;
    if ( shortPathTree[source].arcs.notEmpty() || dest == source ) {

        FLOAT_TYPE base_path_cost=dist[source];
        v.start_path(path_no,base_path_cost);
        insertShortPath(source, dest, v);
        v.end_path();

        if ( k > 1 ) {
            GraphHeap::freeAll();
            Graph revPathTree = reverseGraph(shortPathGraph);
            pathGraph = NEW GraphHeap *[nStates];
            sidetracks = sidetrackGraph(graph, shortPathGraph, dist);
            bool *visited = NEW bool[nStates];
            for ( unsigned i = 0 ; i < nStates ; ++i ) visited[i] = false;
            // IMPORTANT NOTE: depthFirstSearch recursively calls the function
            // passed as the last argument (in this  case "buildSidetracksHeap")
            //
//      freeAllSidetracks();
            depthFirstSearch(revPathTree, dest, visited, buildSidetracksHeap);
            if ( pathGraph[source] ) {
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
                newPath.weight = pathGraph[source]->arc->weight;
                newPath.heapPos = -1;
                newPath.node = pathGraph[source];
                newPath.last = NULL;
                heapAdd(pathQueue, endQueue++, newPath);
                while ( heapSize(pathQueue, endQueue) && ++path_no <= k ) {
                    EdgePath *top = pathQueue;
                    GraphArc *cutArc;
                    typedef List<GraphArc *> Sidetracks;
                    Sidetracks shortPath;
#ifdef DEBUGKBEST
                    Config::debug() << top->weight;
#endif
                    if ( top->heapPos == -1 )
                        cutArc = top->node->arc;
                    else
                        cutArc = top->node->arcHeap[top->heapPos];
                    path_cost=base_path_cost;
                    shortPath.push( cutArc);
                    path_cost += cutArc->weight;
#ifdef DEBUGKBEST
                    Config::debug() << ' ' << *cutArc;
#endif
                    EdgePath *last;
                    while ( (last = top->last) ) {
                        if ( !((last->heapPos == -1 && (top->heapPos == 0 || top->node == last->node->left || top->node == last->node->right )) || (last->heapPos >= 0 && top->heapPos != -1 )) ) { // got to p on a cross edge
                            if ( last->heapPos == -1 )
                                cutArc = last->node->arc;
                            else
                                cutArc = last->node->arcHeap[last->heapPos];
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

                    int sourceState = source; // pretend beginning state is end of last sidetrack

                    v.start_path(path_no,path_cost);
                    for ( Sidetracks::const_iterator cut=shortPath.const_begin(),end=shortPath.const_end(); cut != end; ++cut ) {
                        GraphArc *cutarc=*cut;
                        insertShortPath(sourceState, cutarc->source, v); // stitch end of last sidetrack to beginning of this one
                        sourceState = cutarc->dest;
                        v.visit_sidetrack_arc(*cutarc);
                    }

                    insertShortPath(sourceState, dest, v); // connect end of last sidetrack to dest state

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
                            heapAdd(pathQueue, endQueue++, newPath);
                        }
                        if ( from->right ) {
                            newPath.node = from->right;
                            newPath.weight = lastWeight + (newPath.node->arc->weight - spawnVertex->weight);
                            heapAdd(pathQueue, endQueue++, newPath);
                        }
                        if ( from->arcHeapSize ) {
                            newPath.heapPos = 0;
                            newPath.node = from;
                            newPath.weight = lastWeight + (newPath.node->arcHeap[0]->weight - spawnVertex->weight);
                            heapAdd(pathQueue, endQueue++, newPath);
                        }
                    } else {
                        spawnVertex = from->arcHeap[lastHeapPos];
                        newPath.node = from;
                        int iChild = 2 * lastHeapPos + 1;
                        if ( from->arcHeapSize > iChild  ) {
                            newPath.heapPos = iChild;
                            newPath.weight = lastWeight + (newPath.node->arcHeap[iChild]->weight - spawnVertex->weight);
                            heapAdd(pathQueue, endQueue++, newPath);
                            if ( from->arcHeapSize > ++iChild ) {
                                newPath.heapPos = iChild;
                                newPath.weight = lastWeight + (newPath.node->arcHeap[iChild]->weight - spawnVertex->weight);
                                heapAdd(pathQueue, endQueue++, newPath);
                            }
                        }
                    }
                    if ( pathGraph[spawnVertex->dest] ) {
                        newPath.heapPos = -1;
                        newPath.node = pathGraph[spawnVertex->dest];
                        newPath.heapPos = -1;
                        newPath.weight = lastWeight + newPath.node->arc->weight;
                        heapAdd(pathQueue, endQueue++, newPath);
                    }
                } // end of while
                delete[] pathQueue;
                delete[] retired;
            } // end of if (pathGraph[0])
            GraphHeap::freeAll(); // FIXME: global
            freeAllSidetracks(); // FIXME: global

            delete[] pathGraph;
            delete[] visited;
            freeGraph(revPathTree);
            freeGraph(sidetracks);
        } // end of if (k > 1)
    }

    freeGraph(shortPathGraph);
    delete[] dist;


}
#endif
