#include <vector>
#include "kbest.h"
#include "node.h"

Graph sidetracks;
GraphHeap **pathGraph;
GraphState *shortPathTree;
using namespace std;

int operator < (const pGraphArc l, const pGraphArc r) {
  return l->weight > r->weight;
}

Node<GraphHeap *> *Node<GraphHeap *>::freeList = NULL;
const int Node<GraphHeap *>::newBlocksize = 64;

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

void buildSidetracksHeap(int state, int pred)
{
  // IMPORTANT NOTE: Yaser 6-25-2001 This function create new memory
  // of type (pGraphArc *). This memory is not deleted inside the function
  // because it is used else where. For this reason addresses for
  // the memory created is kept in a global variable "Repsitory" so that
  // the caller function (e.g., bestPaths) deletes the memory when it is done.
  GraphHeap *prev;

  if ( pred == -1 )
    prev = NULL;
  else
    prev = pathGraph[pred];


  List<GraphArc>::iterator s=sidetracks.states[state].arcs.begin();
  List<GraphArc>::iterator end = sidetracks.states[state].arcs.end();
  if ( s != end ) {
    int heapSize = 0;
    GraphArc *min;
    min = &(*s);
    while ( ++s !=  end ) {
      if ( s->weight < min->weight )
        min = &(*s);
      ++heapSize;
    }
    pathGraph[state] = new GraphHeap;
    pathGraph[state]->arc = min;
    pathGraph[state]->arcHeapSize = heapSize;
    if ( heapSize ) {
      pGraphArc *heapStart = pathGraph[state]->arcHeap = new pGraphArc[heapSize];
      Repository.push_back(heapStart); // keep track of it so that we can delete it later
      pGraphArc *heapI = heapStart;
      List<GraphArc>::iterator end = sidetracks.states[state].arcs.end()  ;
      for ( List<GraphArc>::iterator gArc=sidetracks.states[state].arcs.begin() ; gArc !=end ; ++gArc )
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

List<List<PathArc> > * WFST::randomPaths(int k,int max_len)
{
  Assert(valid());
  List<List<PathArc> > *paths = new List<List<PathArc> >;
  //List<List<PathArc> >::iterator insertHere=paths->begin();
  for (int i=0;i<k;) {
	  paths->push_front(List<PathArc>());
	  if ( randomPath(back_insert_iterator<List<PathArc> > (paths->front()),max_len) == -1 ) {
		  paths->pop_front();
	  } else {
		  ++i;
	  }

  }
  return paths;
}


List<List<PathArc> > *WFST::bestPaths(int k)
{
  int nStates = numStates();
  Assert(valid());

#ifdef DEBUGKBEST
  cout << "Calling KBest on WFST with k: "<<k<<'\n';
#endif

  List<List<PathArc> > *paths = new List<List<PathArc> >;
  //List<List<PathArc> >::iterator insertHere=paths->begin();

  Graph graph = makeGraph();
  float *dist = new float[nStates];
  Graph shortPathGraph = shortestPathTree(graph, final, dist);
  shortPathTree = shortPathGraph.states;

  if ( shortPathTree[0].arcs.notEmpty() || final == 0 ) {

    List<PathArc> temp;
    //List<PathArc>::iterator path=temp.begin();
    insertShortPath(0, final, &temp);
    paths->push_back(temp);

    if ( k > 1 ) {
      GraphHeap::freeAll();
      Graph revPathTree = reverseGraph(shortPathGraph);
      pathGraph = new GraphHeap *[nStates];
      sidetracks = sidetrackGraph(graph, shortPathGraph, dist);
      bool *visited = new bool[nStates];
      for ( int i = 0 ; i < nStates ; ++i ) visited[i] = false;
      // IMPORTANT NOTE: depthFirstSearch recursively calls the function
      // passed as the last argument (in this  case "buildSidetracksHeap")
      //
      depthFirstSearch(revPathTree, final, visited, buildSidetracksHeap);
      if ( pathGraph[0] ) {
#ifdef DEBUGKBEST
        cout << "printing trees\n";
        for ( int i = 0 ; i < nStates ; ++i )
          printTree(pathGraph[i], 0);
        cout << "done printing trees\n";
#endif
        EdgePath *pathQueue = new EdgePath[4 * (k+1)];  // out-degree is at most 4
        EdgePath *endQueue = pathQueue;
        EdgePath *retired = new EdgePath[k+1];
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
          cout << '\n';
#endif
          List<PathArc> temp;
          //List<PathArc>::iterator fullPath=temp.begin();
          int sourceState = 0;
          List<GraphArc *>::const_iterator end = shortPath.end();
          for ( List<GraphArc *>::const_iterator cut=shortPath.begin() ; cut != end; ++cut ) {
            insertShortPath(sourceState, (*cut)->source, &temp);
            sourceState = (*cut)->dest;
            insertPathArc(*cut, &temp);
          }
          insertShortPath(sourceState, final, &temp);
          paths->push_back(temp);
          *endRetired = pathQueue[0];
          newPath.last = endRetired++;
          heapPop(pathQueue, endQueue--);
          int lastHeapPos = newPath.last->heapPos;
          GraphArc *spawnVertex;
          GraphHeap *from = newPath.last->node;
          float lastWeight = newPath.last->weight;
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
      for (unsigned int i = 0 ; i < Repository.size() ;i++)
        if (Repository[i])
          delete (pGraphArc *) Repository[i] ;
      Repository.clear();

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

void WFST::insertPathArc(GraphArc *gArc,List<PathArc>* l)
{
  PathArc pArc;
  Arc *taken = (Arc *)gArc->data;
  setPathArc(&pArc,*taken);
  l->push_back(pArc);
}

void WFST::insertShortPath(int source, int dest, List<PathArc>*l)
{
  GraphArc *taken;
  for ( int iState = source ; iState != dest; iState = taken->dest ) {
    taken = &shortPathTree[iState].arcs.top();
    insertPathArc(taken,l);
  }
}

Graph sidetrackGraph(Graph lG, Graph rG, float *dist)
// Comment by Yaser: This function creates new GraphState[] and because the
// return Graph points to this newly created Graph, it is NOT deleted. Therefore
//  the caller function is responsible for deleting this data.
// It is not a good programming practice but it will be messy to clean it up.
//

{
  Assert(lG.nStates == rG.nStates);
  int nStates = lG.nStates;
  GraphState *sub = new GraphState[nStates];
  for ( int i = 0 ; i < nStates ; ++i )
   if ( dist[i] != Weight::HUGE_FLOAT ){
       List<GraphArc>::const_iterator end = lG.states[i].arcs.end();
      for ( List<GraphArc>::const_iterator l=lG.states[i].arcs.begin() ; l != end; ++l ) {
        Assert(i == l->source);
        int isShort = 0;
        List<GraphArc>::const_iterator end2 = rG.states[i].arcs.end()  ;
        for ( List<GraphArc>::const_iterator r=rG.states[i].arcs.begin() ; r !=end2  ; ++r )
          if ( r->data == l->data ) {
            isShort = 1;
            break;
          }
        if ( !isShort )
         if ( dist[l->dest] != Weight::HUGE_FLOAT ) {
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
