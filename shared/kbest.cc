#include "config.h"
#include <vector>
#include "kbest.h"

Graph sidetracks;
GraphHeap **pathGraph;
GraphState *shortPathTree;
using namespace std;

int operator < (const pGraphArc l, const pGraphArc r) {
  return l->weight > r->weight;
}

List<GraphHeap *> GraphHeap::usedBlocks;
GraphHeap * GraphHeap::freeList = NULL;
const int GraphHeap::newBlocksize = 64;

int operator < (const GraphHeap &l, const GraphHeap &r) {
  return l.arc->weight > r.arc->weight;
}

int operator < (const EdgePath &l, const EdgePath &r) {
  return l.weight > r.weight;
}

vector<pGraphArc *> Repository ;

void freeAllSidetracks()
{
    for (unsigned int i = 0 ; i < Repository.size() ;i++)
        if (Repository[i])
            delete (pGraphArc *) Repository[i] ;
    Repository.clear();
}

void buildSidetracksHeap(int state, int pred)
{
  // IMPORTANT NOTE: Yaser 6-25-2001 This function create NEW memory
  // of type (pGraphArc *). This memory is not deleted inside the function
  // because it is used else where. For this reason addresses for
  // the memory created is kept in a global variable "Repsitory" so that
  // the caller function (e.g., bestPaths) deletes the memory when it is done.
  GraphHeap *prev;

  if ( pred == -1 )
    prev = NULL;
  else
    prev = pathGraph[pred];


  List<GraphArc> &arcs=sidetracks.states[state].arcs;
  List<GraphArc>::val_iterator s=arcs.val_begin(),end=arcs.val_end();
  if ( s != end ) {
    int heapSize = 0;
    GraphArc *min;
    min = &(*s);
    while ( ++s !=  end ) {
      if ( s->weight < min->weight )
        min = &(*s);
      ++heapSize;
    }
    pathGraph[state] = NEW GraphHeap;
    pathGraph[state]->arc = min;
    pathGraph[state]->arcHeapSize = heapSize;
    if ( heapSize ) {
      pGraphArc *heapStart = pathGraph[state]->arcHeap = NEW pGraphArc[heapSize];
      Repository.push_back(heapStart); // keep track of it so that we can delete it later
      pGraphArc *heapI = heapStart;
      //      List<GraphArc>::iterator end = sidetracks.states[state].arcs.end()  ;
      //    for ( List<GraphArc>::iterator gArc=sidetracks.states[state].arcs.begin() ; gArc !=end ; ++gArc )
      for ( List<GraphArc>::val_iterator gArc=arcs.val_begin(),end=arcs.val_end();gArc !=end ; ++gArc )
        if ( &(*gArc) != min )
          (heapI++)->p = &(*gArc);
      Assert(heapI == heapStart + heapSize);
      heapBuild(heapStart, heapStart + heapSize);
    } else
      pathGraph[state]->arcHeap = NULL;
    pathGraph[state] = newTreeHeapAdd(prev, pathGraph[state]);
  } else
    pathGraph[state] = prev;
} // end of buildSidetracksHeap()



/*
void insertShortPath(int source, int dest, ListIter<GraphArc *> &path)
{
  GraphArc *taken;
  for ( int iState = source ; iState != dest; iState = taken->dest ) {
    taken = &shortPathTree[iState].arcs.top();
    path.insert((GraphArc *)taken->data);
  }
}
List<List<GraphArc *> > *bestPaths(Graph graph, int source, int dest, int k)
{
  int nStates = graph.nStates;
  assert(nStates > 0 && graph.states);
  assert(source >= 0 && source < nStates);
  assert(dest >= 0 && dest < nStates);

  List<List<GraphArc *> > *paths = NEW List<List<GraphArc *> >;
  insert_iterator<LLP> path_adder(*paths,paths->erase_begin());

  ListIter<List<GraphArc *> > insertHere(*paths); // append rather than push so best path comes first in list

  FLOAT_TYPE *dist = NEW FLOAT_TYPE[nStates];
  Graph shortPathGraph = shortestPathTree(graph, dest, dist);
  shortPathTree = shortPathGraph.states;

  if ( shortPathTree[source].arcs.notEmpty() || source == dest ) {

    ListIter<GraphArc *> path(insertHere.insert(List<GraphArc *>()));
    insertShortPath(source, dest, path);

    if ( k > 1 ) {
      GraphHeap::freeAll();
      List<List<GraphArc *> > graphPaths;
      Graph revPathTree = reverseGraph(shortPathGraph);
      pathGraph = new GraphHeap *[nStates];
      sidetracks = sidetrackGraph(graph, shortPathGraph, dist);
      bool *visited = new bool[nStates];
      for ( int i = 0 ; i < nStates ; ++i ) visited[i] = 0;
      depthFirstSearch(revPathTree, dest, visited, buildSidetracksHeap);
      if ( pathGraph[source] ) {
#ifdef DEBUGKBEST
        cout << "printing trees\n";
        for ( int i = 0 ; i < nStates ; ++i )
          printTree(pathGraph[i], 0);
        cout << "done printing trees\n\n";
#endif
        EdgePath *pathQueue = NEW EdgePath[4 * (k+1)];  // out-degree is at most 4
        EdgePath *endQueue = pathQueue;
        EdgePath *retired = NEW EdgePath[k+1];
        EdgePath *endRetired = retired;
        EdgePath newPath;
        newPath.weight = pathGraph[0]->arc->weight;
        newPath.heapPos = -1;
        newPath.node = pathGraph[0];
        newPath.last = NULL;
        heapAdd(pathQueue, endQueue++, newPath);
        while ( heapSize(pathQueue, endQueue) && --k ) {
          EdgePath *top = pathQueue;
          GraphArc *cutArc;
          List<GraphArc *> shortPath;
#ifdef DEBUGKBEST
          cout << top->weight;
#endif
          if ( top->heapPos == -1 )
            cutArc = top->node->arc;
          else
            cutArc = top->node->arcHeap[top->heapPos];
          shortPath.push( cutArc);
#ifdef DEBUGKBEST
          cout << ' ' << *cutArc;
#endif
          EdgePath *last;
          while ( (last = top->last) ) {
            if ( !((last->heapPos == -1 && (top->heapPos == 0 || top->node == last->node->left || top->node == last->node->right )) || (last->heapPos >= 0 && top->heapPos != -1 )) ) { // got to p on a cross edge
              if ( last->heapPos == -1 )
                cutArc = last->node->arc;
              else
                cutArc = last->node->arcHeap[last->heapPos];
              shortPath.push(cutArc);
#ifdef DEBUGKBEST
              cout << ' ' << *cutArc;
#endif
            }
            top = last;
          }
#ifdef DEBUGKBEST
          cout << "\n\n";
#endif
          List<PathArc> temp;
          //List<PathArc>::iterator fullPath=temp.begin();
          insert_iterator<List<PathArc> > path_cursor(temp,temp.erase_begin());

          int sourceState = 0; // pretend beginning state is end of last sidetrack

          for ( List<GraphArc *>::const_iterator cut=shortPath.const_begin(),end=shortPath.const_end(); cut != end; ++cut ) {
            insertShortPath(shortPathTree, sourceState, (*cut)->source, path_cursor); // stitch end of last sidetrack to beginning of this one
            sourceState = (*cut)->dest;
            insertPathArc(*cut, path_cursor); // append this sidetrack
          }
          insertShortPath(shortPathTree, sourceState, final, path_cursor); // connect end of last sidetrack to final state

          //paths->push_back(temp);
          *path_adder++ = temp;
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
      GraphHeap::freeAll();

      //Yaser 6-26-2001:  The following repository was filled using the
      // "buildSidetracksHeap" method which is called recursively by the
      // "depthFirstSearch()" method and it was never deleted because
      // we needed it here in bestPaths(). Hence we have to delete it here
      // since we are done with it.
      freeAllSidetracks();

      delete[] pathGraph;
      delete[] visited;
      delete[] revPathTree.states;
      delete[] sidetracks.states;
    } // end of if (k > 1)
  }

  delete[] graph.states;
  delete[] shortPathGraph.states;
  delete[] dist;

  return paths;
}
*/

Graph sidetrackGraph(Graph lG, Graph rG, FLOAT_TYPE *dist)
  // Comment by Yaser: This function creates new GraphState[] and because the
  // return Graph points to this newly created Graph, it is NOT deleted. Therefore
  //  the caller function is responsible for deleting this data.
  // It is not a good programming practice but it will be messy to clean it up.
  //

{
  Assert(lG.nStates == rG.nStates);
  int nStates = lG.nStates;
  GraphState *sub = NEW GraphState[nStates];
  for ( int i = 0 ; i < nStates ; ++i )
    if ( dist[i] != HUGE_FLOAT ){

      const List<GraphArc> &la=lG.states[i].arcs;
      for ( List<GraphArc>::const_iterator l=la.const_begin(),end=la.const_end() ; l != end; ++l ) {
        Assert(i == l->source);
        int isShort = 0;

        const List<GraphArc> &ra=rG.states[i].arcs;
        for ( List<GraphArc>::const_iterator r=ra.const_begin(),end=ra.const_end() ; r !=end ; ++r )
          if ( r->data == l->data ) {
            isShort = 1;
            break;
          }
        if ( !isShort )
          if ( dist[l->dest] != HUGE_FLOAT ) {
            GraphArc w = *l;
            w.weight = w.weight - (dist[i] - dist[w.dest]);
            sub[i].arcs.push(w);
          }
      }
    }
  Graph ret;
  ret.nStates = lG.nStates;
  ret.states = sub;
  return ret;
}


void printTree(GraphHeap *t, int n)
{
  int i;
  for ( i = 0 ; i < n ; ++i ) cout << ' ';
  if ( !t ) {
    cout << "-\n";
    return;
  }
  cout << (*t->arc);
  cout << " [";
  pGraphArc *heap = t->arcHeap;
  for ( i = 0 ; i < t->arcHeapSize ; ++i ) {
    cout << *heap[i].p;
  }
  cout << "]\n";
  if ( !t->left && !t->right )
    return;
  printTree(t->left, n+1);
  printTree(t->right, n+1);
}

void shortPrintTree(GraphHeap *t)
{
  cout << *t->arc;
  if ( !t->left && !t->right )
    return;
  cout << " (";
  if ( t->left) {
    shortPrintTree(t->left);
    if ( t->right ) {
      cout << ' ';
      shortPrintTree(t->right);
    }
  } else
    if ( t->right )
      shortPrintTree(t->right);
  cout << ')';
}
