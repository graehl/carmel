#include <cctype>
#include "fst.h"
#include "node.h"

const int WFST::perline_index = ios_base::xalloc();
const int WFST::arcformat_index = ios_base::xalloc();

Weight WFST::sumOfAllPaths(List<int> &inSeq, List<int> &outSeq)
{
  Assert(valid());

  int nIn = inSeq.count_length();
  int nOut = outSeq.count_length();
  int i, o;
  Weight ***w = forwardSumPaths(inSeq, outSeq);

  Weight fin = w[nIn][nOut][final];

  for ( i = 0 ; i <= nIn ; ++i ) {
    for ( o = 0 ; o <= nOut ; ++o )
      delete[] w[i][o];
    delete[] w[i];
  }
  delete[] w;
  return fin;
}

void WFST::pruneArcs(Weight thresh)
{
  for ( int s = 0 ; s < numStates() ; ++s )
    states[s].prune(thresh);
}

int WFST::generate(int *inSeq, int *outSeq, int minArcs, int bufLen)
{
  int i, o, s, nArcs;
  indexInput();
  int maxArcs=bufLen-1;
  i = o = s = nArcs = 0;
  for ( ; ; ) {
    if ( s == final && (states[s].arcs.isEmpty() || nArcs >= minArcs ) ) {
      inSeq[i] = outSeq[o] = 0;
      indexFlush();
      return 1;
    }
    int whichInput = (int)(states[s].index->count() * randomFloat());
    for ( HashIter<IntKey, List<HalfArc> > ha(*states[s].index) ; ha ; ++ha )
      if ( !whichInput-- ) {
        Weight which = randomFloat();
        Weight cum;

        List<HalfArc>::const_iterator a,begin=ha.val().const_begin(),end = ha.val().const_end();

        for(a = begin ; a != end; ++a) {
          cum+=(*a)->weight;
        }
        which *= cum;
        cum.setZero();
        for(a = begin ; a != end; ++a){
          if ( (cum += (*a)->weight) >= which ) {
            if ( (*a)->in )
              if ( i >= maxArcs )
                goto bad;
              else
                inSeq[i++] = (*a)->in;
            if ( (*a)->out )
              if ( o >= maxArcs )
                goto bad;
              else
                outSeq[o++] = (*a)->out;
            s = (*a)->dest;
            ++nArcs;
            break;
          }
        }
        if ( a == end ) {
        bad:
          indexFlush();
          return 0;
        }
        break;
      }
  }
}

namespace WFST_impl {

  class NormGroupIter {
    WFST &wfst;
    State *state;
    State *end;
    typedef HashIter<IntKey, List<HalfArc> > Cit;
    typedef List<HalfArc>::const_iterator Cit2;
    typedef List<Arc>::val_iterator Jit;
    Cit Ci;
    Cit2 Ci2,Cend;
    Jit Ji,Jend;
    const WFST::NormalizeMethod method;
    void beginState() {
      if(method==WFST::CONDITIONAL) {
        Ci.init(*state->index);
      }
    }
  public:
    NormGroupIter(WFST::NormalizeMethod meth,WFST &wfst_) : wfst(wfst_),state((State *)wfst_.states), end(state+wfst_.numStates()), method(meth) { beginState(); }
    bool moreGroups() { return state != end; }
    template <class charT, class Traits>
    std::ios_base::iostate print_on(std::basic_ostream<charT,Traits>& os) const {
      if(method==WFST::CONDITIONAL) {
        os << "(conditional normalization group for input=" << wfst.inLetter(Ci.key()) << " in ";
      } else {
        os << "(joint normalizaton group for ";
      }
      os << "state=" << wfst.stateName(wfst.states.index_of(state)) << ")";
      return std::ios_base::goodbit;
    }
    void beginArcs() {
      if(method==WFST::CONDITIONAL) {
        Ci2 = Ci.val().const_begin();
        Cend = Ci.val().const_end();
      } else {
        Ji = state->arcs.val_begin();
        Jend = state->arcs.val_end();
      }
    }
    bool moreArcs() {
      if(method==WFST::CONDITIONAL) {
        return Ci2 != Cend;
      } else {
        return Ji != Jend;
      }
    }
    Arc * operator *() {
      if(method==WFST::CONDITIONAL) {
        return *Ci2;
      } else {
        return &*Ji;
      }
    }
    void nextArc() {
      if(method==WFST::CONDITIONAL) {
        ++Ci2;
      } else {
        ++Ji;
      }
    }
    void nextGroup() {
      if(method==WFST::CONDITIONAL) {
        ++Ci;
        while (!Ci) {
          ++state;
          if (moreGroups())
            beginState();
          else
            break;
        }
      } else {
        ++state;
      }
    }

  };

};

#include "genio.h"

template <class charT, class Traits>
std::basic_ostream<charT,Traits>&
operator <<
  (std::basic_ostream<charT,Traits>& os, const WFST_impl::NormGroupIter &arg)
{
  return gen_inserter(os,arg);
}



void WFST::normalize(NormalizeMethod method)
{
  if (method==NONE)
    return;
  if (method==CONDITIONAL)
    indexInput();

  // new plan:
  // step 1: compute sum of counts for non-locked arcs, and divide it by (1-(sum of locked arcs)) to reserve appropriate counts for the locked arcs
  // step 2: for tied arc groups, add these inferred counts to the group state counts total.  also sum group arc counts total.
  // step 3: assign tied arc weights; trouble: tied arcs sharing space with inflexible tied arcs.  under- or over- allocation can result ...
  //   ... alternative: give locked arcs implied counts in the tie group; norm-group having tie-group arcs, with highest locked arc sum R divides unscaled tie group state counts total by (1-R) instead of dividing individual state counts by (1-sum).  this ensures that tied arcs are kept small enough to make room for locked ones in ALL states and should leave some room for normal arcs as well
  // step 4: give normal arcs their share of what's left, if anything


  int pGroup;
  HashTable<IntKey, Weight> groupArcTotal;
  HashTable<IntKey, Weight> groupStateTotal;
  HashTable<IntKey, Weight> groupMaxLockedSum;
  // global pass 1: compute the sum of unnormalized weights for each normalization group.  sum for each arc in a tie group, its weight and its normalization group's weight.
  for (WFST_impl::NormGroupIter g(method,*this); g.moreGroups(); g.nextGroup()) {
    Weight sum,locked_sum; // sum of probability of all arcs that has this input
    for ( g.beginArcs(); g.moreArcs(); g.nextArc())
      if (isLocked((*g)->groupId)) //FIXME: how does training handle counts for locked arcs?
        locked_sum += (*g)->weight;
      else
        sum += (*g)->weight;
    for ( g.beginArcs(); g.moreArcs(); g.nextArc())
      if (!(*g)->weight.isZero())
        if ( isTied(pGroup = (*g)->groupId) ) {
          groupArcTotal[pGroup] += (*g)->weight; // default init is to 0
          groupStateTotal[pGroup] += sum;
          Weight &m=groupMaxLockedSum[pGroup];
          if (locked_sum > m)
            m = locked_sum;
        }
  }


  // global pass 2: assign weights
  for (WFST_impl::NormGroupIter g(method,*this); g.moreGroups(); g.nextGroup()) {
    Weight normal_sum;
    Weight reserved;

    // pass 2a: assign tied (and locked) arcs their weights, taking 'reserved' weight from the normal arcs in their group
    // tied arc weight = sum (over arcs in tie group) of weight / sum (over arcs in tie group) of norm-group-total-weight
    // also, compute sum of normal arcs
    for ( g.beginArcs(); g.moreArcs(); g.nextArc())
      if (!(*g)->weight.isZero())
        if ( isTiedOrLocked(pGroup = (*g)->groupId) ) { // tied or locked arc
          if ( isTied(pGroup) ) { // tied:
            Weight groupNorm = *groupStateTotal.find(pGroup); // can't be 0
            groupNorm /= (1. - groupMaxLockedSum[pGroup]); // as described in new plan above: ensure tied arcs leave room for the worst case competing locked arcs sum in any norm-group
            reserved += (*g)->weight = (*groupArcTotal.find(pGroup) / groupNorm);
          } else // locked:
            reserved += (*g)->weight;
        }  else // normal arc
          normal_sum += (*g)->weight;
#ifdef DEBUG_NORMALIZE
    if ( reserved > 1.001 )
      std::cerr << "Warning: sum of reserved arcs for " << g << " = " << reserved << " - should not exceed 1.0\n";
#endif

    // pass 2b: give normal arcs their share of however much is left
    Weight fraction_remain = 1.;
    fraction_remain -= reserved;
    if (!fraction_remain.isZero()) { // something left
      normal_sum /= fraction_remain; // total counts that would be needed to fairly give reserved weights out
      for ( g.beginArcs(); g.moreArcs(); g.nextArc())
        if (isNormal((*g)->groupId))
          (*g)->weight /= normal_sum;
    } else // nothing left, sorry
      for ( g.beginArcs(); g.moreArcs(); g.nextArc())
        if (isNormal((*g)->groupId))
          (*g)->weight.setZero();
  }

#ifdef DEBUG_NORMALIZE
  for (WFST_impl::NormGroupIter g(method,*this); g.moreGroups(); g.nextGroup()) {
    Weight sum;
    for ( g.beginArcs(); g.moreArcs(); g.nextArc())
      sum += (*g)->weight;
    if ( sum > 1.001 )
      std::cerr << "Warning: sum of normalized arcs for " << g << " = " << sum << " - should not exceed 1.0\n";
  }
#endif


  if (method == CONDITIONAL)
    indexFlush();
}

void WFST::assignWeights(const WFST &source)
{
  HashTable<IntKey, Weight> groupWeight;
  int s, pGroup;
  for ( s = 0 ; s < source.numStates() ; ++s ){
    const List<Arc> &arcs = source.states[s].arcs;
    for ( List<Arc>::const_iterator a=arcs.const_begin(),end=arcs.const_end() ; a !=end ; ++a )
      if ( isTied(pGroup = a->groupId) )
        groupWeight[pGroup] = a->weight;
  }
  Weight *pWeight;
  for ( s = 0 ; s < numStates() ; ++s) {
    List<Arc> &arcs = source.states[s].arcs;
    for ( List<Arc>::erase_iterator a=arcs.erase_begin(),end=arcs.erase_end(); a !=end ; ) {
      if ( isTied(pGroup = a->groupId) )
        if ( (pWeight = groupWeight.find(pGroup)) )
          a->weight = *pWeight;
        else {
          a=states[s].arcs.erase(a);
          continue;
        }
      ++a;
    }
  }
}

void WFST::unTieGroups() {
  for ( int s = 0 ; s < numStates() ; ++s ){
    for ( List<Arc>::val_iterator a=states[s].arcs.val_begin(),end = states[s].arcs.val_end(); a != end ; ++a )
      a->groupId = NOGROUP ;
  }
}

void WFST::lockArcs() {
  for ( int s = 0 ; s < numStates() ; ++s ){
    for ( List<Arc>::val_iterator a=states[s].arcs.val_begin(),end = states[s].arcs.val_end(); a != end ; ++a )
      a->groupId = 0 ;
  }
}

void WFST::numberArcsFrom(int label) {
  Assert ( label > 0 );
  for ( int s = 0 ; s < numStates() ; ++s ){
    for ( List<Arc>::val_iterator a=states[s].arcs.val_begin(),end = states[s].arcs.val_end(); a != end ; ++a )
      a->groupId = label++;
  }
}


void WFST::invert()
{
  Assert(valid());
  int temp;
  in->swap(*out);
  for ( int s = 0 ; s < states.count(); ++s ) {
    for ( List<Arc>::val_iterator a=states[s].arcs.val_begin(),end = states[s].arcs.val_end(); a != end ; ++a ) {
      //XXX should use SWAP here instead?
      temp = a->in;
      a->in = a->out;
      a->out = temp;
    }
    states[s].flush();
  }
}

Graph WFST::makeGraph() const
  // Comment by Yaser: This function creates new GraphState[] and because the
  // return Graph points to this newly created Graph, it is NOT deleted. Therefore
  // whatever the caller function is responsible for deleting this data.
  // It is not a good programming practice but it will be messy to clean it up.
  //
{
  Assert(valid());
  GraphState *g = new GraphState[numStates()];
  GraphArc gArc;
  for ( int i = 0 ; i < numStates() ; ++i ){
    for ( List<Arc>::val_iterator l=states[i].arcs.val_begin(),end = states[i].arcs.val_end(); l != end; ++l ) {
      gArc.source = i;
      gArc.dest = l->dest;
      gArc.weight = - l->weight.getLogImp(); // - log
      gArc.data = &(*l);
      Assert(gArc.dest < numStates() && gArc.source < numStates());
      g[i].arcs.push(gArc);
    }
  }
  Graph ret;
  ret.states = g;
  ret.nStates = numStates();
  return ret;
}

Graph WFST::makeEGraph() const
  // Comment by Yaser: This function creates new GraphState[] and because the
  // return Graph points to this newly created Graph, it is NOT deleted. Therefore
  // whatever the caller function is responsible for deleting this data.
  // It is not a good programming practice but it will be messy to clean it up.
  //

{
  Assert(valid());
  GraphState *g = new GraphState[numStates()];
  GraphArc gArc;
  for ( int i = 0 ; i < numStates() ; ++i )
    for ( List<Arc>::val_iterator l=states[i].arcs.val_begin(),end = states[i].arcs.val_end(); l != end; ++l )
      if ( l->in == 0 && l->out == 0 ) {
        gArc.source = i;
        gArc.dest = l->dest;
        gArc.weight = - l->weight.weight; // - log
        gArc.data = &(*l);
        Assert(gArc.dest < numStates() && gArc.source < numStates());
        g[i].arcs.push(gArc);
      }

  Graph ret;
  ret.states = g;
  ret.nStates = numStates();
  return ret;
}

#include <algorithm>
// for std::sort

typedef pair<float,int> PFI;

// isn't there some projectfirst template?
inline bool lesscost(const PFI &l, const PFI &r) {
  return l.first < r.first;
}


void WFST::prunePaths(int max_states,Weight keep_paths_within_ratio)
{
  Assert(valid());
#ifdef DEBUGPRUNE
  Config::debug() << "Prune - keep up to " << max_states << " states, and paths within " << keep_paths_within_ratio << std::endl;
#endif
  int i;
  bool all_paths = keep_paths_within_ratio.isInfinity();
  if (max_states == UNLIMITED && all_paths)
    return;
  int n_states = numStates();

  Graph for_graph = makeGraph();
  Graph rev_graph = reverseGraph(for_graph);

  bool *remove = new bool[n_states];
  float worst_d_dist = keep_paths_within_ratio.getLogImp();
  float *for_dist = new float[n_states];
  float *rev_dist = new float[n_states];
  // todo: efficiency: could use indirected compare on array of integers, instead of moving around float+integer
  PFI *best_path_cost = new PFI[n_states];
  shortestDistancesFrom(for_graph,0, for_dist,NULL);
  shortestDistancesFrom(rev_graph,final, rev_dist,NULL);
  float best_path = for_dist[final];
  float worst_path = best_path + worst_d_dist;
  Assert(best_path == rev_dist[0]);
  for (i=0;i<n_states;++i) {
    best_path_cost[i].first = for_dist[i] + rev_dist[i];
    best_path_cost[i].second = i;
  }

  std::sort(best_path_cost,best_path_cost+n_states,lesscost);
  // now we have a list of states in order of increasing cost (poorness)
#ifdef DEBUGPRUNE
  Config::debug() << "Best path cost = " << best_path << "; worst path allowed = " << worst_path << std::endl;
  Config::debug() << "BEST PATH THROUGH STATE:" << std::endl;
  for (i=0;i<n_states;++i) {
    Config::debug() << best_path_cost[i].first << ' ' << stateName(best_path_cost[i].second) << std::endl;
  }
#endif

  int allowed = max_states;
  if ( max_states == UNLIMITED || max_states > n_states )
    allowed = n_states;

  for (i=0;i<allowed;++i) {
    int st=best_path_cost[i].second;
    if (all_paths)
      remove[st] = false;
    else {
      if (best_path_cost[i].first > worst_path)
        remove[st] = true;
      else {
        remove[st] = false;
        State &s=states[st];
        for ( List<Arc>::erase_iterator a(s.arcs.erase_begin()), end = s.arcs.erase_end() ; a !=end  ;  ) {
          float best_path_this_arc = (-a->weight.getLogImp())+for_dist[st]+rev_dist[a->dest];
#ifdef DEBUGPRUNE
          Config::debug() << "Arc " << st << ": ";
          printArc(*a,st,Config::debug()) << " best path cost = " << best_path_this_arc << std::endl;
#endif
          if (best_path_this_arc > worst_path)
            a=s.remove(a);
          else
            ++a;
        }
      }
    }
  }
  for (;i<n_states;++i) { // over allotted state limit
    int st=best_path_cost[i].second;
    remove[st] = true;
  }

  removeMarkedStates(remove);
  //n_states = numStates();

  delete[] for_dist;
  delete[] rev_dist;
  delete[] remove;
  delete[] rev_graph.states;
  delete[] for_graph.states;
}

void WFST::reduce()
{
  int nStates = numStates();

  if (!valid()) {
    clear();
    return;
  }

  Graph g = makeGraph();
  GraphState *graph = g.states;
  Graph revG = reverseGraph(g);
  GraphState *revGraph = revG.states;
  Assert(nStates == g.nStates && g.nStates == revG.nStates);

  bool *visitedForward = new bool[nStates];
  bool *visitedBackward = new bool[nStates];
  //bool *discard = new bool[nStates];
  int i;
  for ( i = 0 ; i < nStates ; ++i ){
    visitedForward[i] = false;
    visitedBackward[i] = false;
  }

  depthFirstSearch(g, 0, visitedForward, NULL);
  depthFirstSearch(revG, final, visitedBackward, NULL);


  //bool *discard = visitedForward;
  for ( i = 0 ; i < nStates ; ++i )
    visitedForward[i] = !(visitedForward[i] && visitedBackward[i]);

  // Begin additions by Yaser to fix a bug where when a state is discarded, the tieGroup is not updated
  // when  states are discarded, their repsective arcs must be removed from tieGroup
  /* jon: the below by yaser makes no sense.  tie groups are not explicit lists
     for ( i = 0 ; i < nStates ; ++i ){
     discard[i] = !(visitedForward[i] && visitedBackward[i]);
     for ( List<Arc>::iterator a(states[i].arcs.begin()), end = states[i].arcs.end() ; a !=end  ; ++a ){
     if ((discard[i])
     || !(visitedForward[a->dest] && visitedBackward[(a->dest)])){ // if a state should be discarded remove its arcs from tie group, also an Arc must be removed if its destination state is discarded.
     a->groupId = NOGROUP ;
     }
     }
     }
     // end of Yaser's additions - Oct. 12 2000
     */
  removeMarkedStates(visitedForward);

  for ( i = 0 ; i < numStates() ; ++i ) {
    states[i].flush();
    for ( List<Arc>::erase_iterator a=states[i].arcs.erase_begin(),end = states[i].arcs.erase_end() ; a != end; )
      if ( a->in == 0 && a->out == 0 && a->dest == i ) // erase empty loops
        a=states[i].arcs.erase(a);
      else
        ++a;
  }

  delete[] visitedBackward;
  delete[] visitedForward;
  // delete[] discard;
  delete[] revGraph;
  delete[] graph;
}

void WFST::consolidateArcs()
{
  for ( int i = 0 ; i < numStates() ; ++i )
    states[i].reduce();
}

void WFST::removeMarkedStates(bool marked[])
{
  Assert(valid());
  int *oldToNew = new int[numStates()];
  int i = 0, f = 0;
  while ( i < numStates() )
    if (marked[i])
      oldToNew[i++] = -1;
    else
      oldToNew[i++] = f++;
  if ( i == f ) { // none to be removed
    delete[] oldToNew;
    return;
  }
  stateNames.removeMarked(marked, oldToNew);
  states.removeMarked(marked);
  for ( i = 0 ; i < states.count() ; ++i ) {
    states[i].flush();
    states[i].renumberDestinations(oldToNew);
  }
  final = oldToNew[final];
  delete[] oldToNew;
}

int WFST::abort() {
  return 0;
}

ostream & operator << (ostream &o, List<PathArc> &l) {
  for (  List<PathArc>::const_iterator li=l.const_begin(),end = l.const_end(); li != end; ++li )
    o << *li << " ";
  return o;
}

ostream & operator << (ostream &o, WFST &w) {
  w.writeLegible(o); //- Yaser  07-20-2000
  return o;
}


int WFST::indexThreshold = 128;
int TrioKey::aMax = 0;
int TrioKey::bMax = 0;

const int Entry<StringKey, int>::newBlocksize = 64;
Entry<StringKey, int> *Entry<StringKey, int>::freeList = NULL;

const int Entry<TrioKey,int>::newBlocksize = 64;
Entry<TrioKey,int> *Entry<TrioKey,int>::freeList = NULL;

const int Entry<UnArc,Weight *>::newBlocksize = 64;
Entry<UnArc,Weight *> *Entry<UnArc,Weight *>::freeList = NULL;

Node<HalfArc> *Node<HalfArc>::freeList = NULL;
const int Node<HalfArc>::newBlocksize = 64;

Node<int> *Node<int>::freeList = NULL;
const int Node<int>::newBlocksize = 64;

const int Entry<IntKey,List<HalfArc> >::newBlocksize = 64;
Entry<IntKey,List<HalfArc> > *Entry<IntKey,List<HalfArc> >::freeList = NULL;

const int Entry<IntKey, int >::newBlocksize = 64;
Entry<IntKey,int > *Entry<IntKey,int>::freeList = NULL;

const int Entry<HalfArcState, int >::newBlocksize = 64;
Entry<HalfArcState,int > *Entry<HalfArcState,int>::freeList = NULL;

const int Entry<IntKey, Weight >::newBlocksize = 64;
Entry<IntKey, Weight > *Entry<IntKey, Weight>::freeList = NULL;

Node<PathArc> *Node<PathArc>::freeList = NULL;
const int Node<PathArc>::newBlocksize = 64;

Node<TrioID> *Node<TrioID>::freeList = NULL;
const int Node<TrioID>::newBlocksize = 64;

Node<Arc> *Node<Arc>::freeList = NULL;
const int Node<Arc>::newBlocksize = 64;

Node<List<PathArc> > *Node<List<PathArc> >::freeList = NULL;
const int Node<List<PathArc> >::newBlocksize = 64;

Node<List<GraphArc *> > *Node<List<GraphArc *> >::freeList = NULL;
const int Node<List<GraphArc *> >::newBlocksize = 64;

Node<GraphArc *> *Node<GraphArc *>::freeList = NULL;
const int Node<GraphArc *>::newBlocksize = 64;
