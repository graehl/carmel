#include "config.h"
#include <cctype>
#include "fst.h"
#include "kbest.h"

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
    int whichInput = (int)(states[s].index->size() * randomFloat());
        const HashTable<IntKey, List<HalfArc> > &hat=*states[s].index;
    for ( HashTable<IntKey, List<HalfArc> >::const_iterator ha = hat.begin() ; ha != hat.end(); ++ha )
      if ( !whichInput-- ) {
        Weight which = randomFloat();
        Weight cum;

        List<HalfArc>::const_iterator a,begin=ha->second.const_begin(),end = ha->second.const_end();

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
    typedef HashTable<IntKey, List<HalfArc> >::iterator Cit;
    typedef List<HalfArc>::const_iterator Cit2;
    typedef List<FSTArc>::val_iterator Jit;
    Cit Ci;
    Cit2 Ci2,Cend;
    Jit Ji,Jend;
    const WFST::NormalizeMethod method;
    bool empty_state() { return state->size == 0; }
    void beginState() {
      if(method==WFST::CONDITIONAL)
        if (!empty_state())
          Ci = state->index->begin();
    }
  public:
    NormGroupIter(WFST::NormalizeMethod meth,WFST &wfst_) : wfst(wfst_),state(wfst_.states.begin()), end(state+wfst_.numStates()), method(meth) { beginState(); } // initializer order = same as declaration (state before end)
    bool moreGroups() { return state != end; }
    template <class charT, class Traits>
    std::ios_base::iostate print(std::basic_ostream<charT,Traits>& os) const {
      if(method==WFST::CONDITIONAL) {
        os << "(conditional normalization group for input=" << wfst.inLetter(Ci->first) << " in ";
      } else {
        os << "(joint normalizaton group for ";
      }
      os << "state=" << wfst.stateName(wfst.states.index_of(state)) << ")";
      return std::ios_base::goodbit;
    }
    void beginArcs() {
      if(method==WFST::CONDITIONAL) {
        if (empty_state())
          return;
        Ci2 = Ci->second.const_begin();
        Cend = Ci->second.const_end();
      } else {
        Ji = state->arcs.val_begin();
        Jend = state->arcs.val_end();
      }
    }
    bool moreArcs() {
      if(method==WFST::CONDITIONAL) {
        if (empty_state())
          return false;
        return Ci2 != Cend;
      } else {
        return Ji != Jend;
      }
    }
    FSTArc * operator *() {
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
        if ( !empty_state() )
          ++Ci;
        while (empty_state() || Ci == state->index->end()) {
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



static void NaNCheck(const Weight *w) {
        w->NaNCheck();
}


void WFST::normalize(NormalizeMethod method)
{
  if (method==NONE)
    return;
  if (method==CONDITIONAL)
    indexInput();

  // NEW plan:
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
    Weight sum,locked_sum; // =0, sum of probability of all arcs that has this input
                Assert(sum.isZero() && locked_sum.isZero());
    for ( g.beginArcs(); g.moreArcs(); g.nextArc()) {
      const Weight w=(*g)->weight;
      if (isLocked((*g)->groupId)) //FIXME: how does training handle counts for locked arcs?
        locked_sum += w;
      else
        sum += w;
    }
#ifdef DEBUGNAN
                                readEachParameter(NaNCheck);
#endif
#ifdef DEBUGNORMALIZE
    Config::debug() << "Normgroup=" << g << " locked_sum=" << locked_sum << " sum=" << sum << std::endl;
#endif
    for ( g.beginArcs(); g.moreArcs(); g.nextArc()) {


        if ( isTied(pGroup = (*g)->groupId) ) {
                        groupArcTotal[pGroup] += (*g)->weight; // default init is to 0          
                      groupStateTotal[pGroup] += sum;
          Weight &m=groupMaxLockedSum[pGroup];
          if (locked_sum > m)
            m = locked_sum;
                                        NANCHECK(groupStateTotal[pGroup]);
                                        NANCHECK(groupStateTotal[pGroup]);
                                        NANCHECK(groupMaxLockedSum[pGroup]);
#ifdef DEBUGNORMALIZE
          Config::debug() << "Tiegroup=" << pGroup << " Normgroup=" << g << " tie_weight=" << groupArcTotal[pGroup] << " sum_state_weight=" << groupStateTotal[pGroup] << " max_locked=" << m << std::endl;
#endif

        }
    }
  }


  // global pass 2: assign weights
  for (WFST_impl::NormGroupIter g(method,*this); g.moreGroups(); g.nextGroup()) {
    Weight normal_sum;//=0
    Weight reserved;// =0
                Assert(reserved.isZero()&&normal_sum.isZero());
    // pass 2a: assign tied (and locked) arcs their weights, taking 'reserved' weight from the normal arcs in their group
    // tied arc weight = sum (over arcs in tie group) of weight / sum (over arcs in tie group) of norm-group-total-weight
    // also, compute sum of normal arcs
    for ( g.beginArcs(); g.moreArcs(); g.nextArc())

        if ( isTiedOrLocked(pGroup = (*g)->groupId) ) { // tied or locked arc
          if ( isTied(pGroup) ) { // tied:
            Weight groupNorm = *find_second(groupStateTotal,(IntKey)pGroup); // can be 0 if no counts at all for any states of group
                                                Weight gmax=*find_second(groupMaxLockedSum,(IntKey)pGroup);
                                                NANCHECK(gmax);
                                                Weight one(1.);
                                                if ( gmax > one) {
                                                        (*g)->weight.setZero();
                                                } else {
                                                        if ( !gmax.isZero() )
                                                                groupNorm /= (one - gmax); // as described in NEW plan above: ensure tied arcs leave room for the worst case competing locked arcs sum in any norm-group
                                                        NANCHECK(groupNorm);

                                                        Weight groupTotal=*find_second(groupArcTotal,(IntKey)pGroup);
                                                        NANCHECK(groupTotal);
                                                        if (!groupTotal.isZero()) // then groupNorm non0 also
                                                                reserved += (*g)->weight = ( groupTotal/ groupNorm);
                                                        else
                                                                (*g)->weight.setZero();
                                                        NANCHECK(reserved);
                                                }
                                        } else { // locked:
            reserved += (*g)->weight;
                                                NANCHECK(reserved);
                                        }
        }  else if (!(*g)->weight.isZero()) // normal arc
          normal_sum += (*g)->weight;
#ifdef DEBUGNORMALIZE
    if ( reserved > 1.001 )
      Config::warn() << "Warning: sum of reserved arcs for " << g << " = " << reserved << " - should not exceed 1.0\n";
#endif

    // pass 2b: give normal arcs their share of however much is left
    Weight fraction_remain = 1.;
    fraction_remain -= reserved;
        NANCHECK(fraction_remain);
    if (!fraction_remain.isZero()) { // something left
      normal_sum /= fraction_remain; // total counts that would be needed to fairly give reserved weights out
                        NANCHECK(normal_sum);
      for ( g.beginArcs(); g.moreArcs(); g.nextArc())
                                if (isNormal((*g)->groupId)) {
          (*g)->weight /= normal_sum;
                                        NANCHECK((*g)->weight);
                                }
    } else // nothing left, sorry
      for ( g.beginArcs(); g.moreArcs(); g.nextArc())
        if (isNormal((*g)->groupId))
          (*g)->weight.setZero();
  }

#ifdef CHECKNORMALIZE
  for (WFST_impl::NormGroupIter g(method,*this); g.moreGroups(); g.nextGroup()) {

    Weight sum;
    for ( g.beginArcs(); g.moreArcs(); g.nextArc())
      sum += (*g)->weight;
#define NORM_EPSILON .01
    if ( sum > 1+NORM_EPSILON || sum < 1-NORM_EPSILON)
      Config::warn() << "Warning: sum of normalized arcs for " << g << " = " << sum << " - should equal 1.0\n";
  }
#endif

#ifdef DEBUGNAN
        readEachParameter(NaNCheck);
#endif

  if (method == CONDITIONAL)
    indexFlush(); // free up by-input index we created at start
}

void WFST::assignWeights(const WFST &source)
{
  HashTable<IntKey, Weight> groupWeight;
  int s, pGroup;
  for ( s = 0 ; s < source.numStates() ; ++s ){
    const List<FSTArc> &arcs = source.states[s].arcs;
    for ( List<FSTArc>::const_iterator a=arcs.const_begin(),end=arcs.const_end() ; a !=end ; ++a )
      if ( isTied(pGroup = a->groupId) )
        groupWeight[pGroup] = a->weight;
  }
  Weight *pWeight;
  for ( s = 0 ; s < numStates() ; ++s) {
        source.states[s].flush();
    List<FSTArc> &arcs = source.states[s].arcs;
    for ( List<FSTArc>::erase_iterator a=arcs.erase_begin(),end=arcs.erase_end(); a !=end ; ) {
      if ( isTied(pGroup = a->groupId) )
        if ( (pWeight = find_second(groupWeight,(IntKey)pGroup)) )
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
    for ( List<FSTArc>::val_iterator a=states[s].arcs.val_begin(),end = states[s].arcs.val_end(); a != end ; ++a )
      a->groupId = NOGROUP ;
  }
}

void WFST::lockArcs() {
  for ( int s = 0 ; s < numStates() ; ++s ){
    for ( List<FSTArc>::val_iterator a=states[s].arcs.val_begin(),end = states[s].arcs.val_end(); a != end ; ++a )
      a->groupId = 0 ;
  }
}

void WFST::numberArcsFrom(int label) {
  Assert ( label > 0 );
  for ( int s = 0 ; s < numStates() ; ++s ){
    for ( List<FSTArc>::val_iterator a=states[s].arcs.val_begin(),end = states[s].arcs.val_end(); a != end ; ++a )
      a->groupId = label++;
  }
}


void WFST::invert()
{
  Assert(valid());
  int temp;
  in->swap(*out);
  for ( unsigned int s = 0 ; s < states.size(); ++s ) {
    for ( List<FSTArc>::val_iterator a=states[s].arcs.val_begin(),end = states[s].arcs.val_end(); a != end ; ++a ) {
      //XXX should use SWAP here instead?
      temp = a->in;
      a->in = a->out;
      a->out = temp;
    }
    states[s].flush();
  }
}

Graph WFST::makeGraph() const
  // Comment by Yaser: This function creates NEW GraphState[] and because the
  // return Graph points to this newly created Graph, it is NOT deleted. Therefore
  // whatever the caller function is responsible for deleting this data.
  //
{
  Assert(valid());
  GraphState *g = NEW GraphState[numStates()];
  GraphArc gArc;
  for ( int i = 0 ; i < numStates() ; ++i ){
    for ( List<FSTArc>::val_iterator l=states[i].arcs.val_begin(),end = states[i].arcs.val_end(); l != end; ++l ) {
      gArc.source = i;
      gArc.dest = l->dest;
      gArc.weight = l->weight.getCost();
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
  // Comment by Yaser: This function creates NEW GraphState[] and because the
  // return Graph points to this newly created Graph, it is NOT deleted. Therefore
  // whatever the caller function is responsible for deleting this data.
  //

{
  Assert(valid());
  GraphState *g = NEW GraphState[numStates()];
  GraphArc gArc;
  for ( int i = 0 ; i < numStates() ; ++i )
    for ( List<FSTArc>::val_iterator l=states[i].arcs.val_begin(),end = states[i].arcs.val_end(); l != end; ++l )
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

typedef pair<FLOAT_TYPE,int> PFI;

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

  bool *remove = NEW bool[n_states];
  FLOAT_TYPE worst_d_dist = keep_paths_within_ratio.getLogImp();
  FLOAT_TYPE *for_dist = NEW FLOAT_TYPE[n_states];
  FLOAT_TYPE *rev_dist = NEW FLOAT_TYPE[n_states];
  // todo: efficiency: could use indirected compare on array of integers, instead of moving around FLOAT_TYPE+integer
  PFI *best_path_cost = NEW PFI[n_states];
  shortestDistancesFrom(for_graph,0, for_dist,NULL);
  shortestDistancesFrom(rev_graph,final, rev_dist,NULL);
  FLOAT_TYPE best_path = for_dist[final];
  FLOAT_TYPE worst_path = best_path + worst_d_dist;
  Assert(fabs(best_path - rev_dist[0]) < 1e-5);

  for (i=0;i<n_states;++i) {
    best_path_cost[i].first = for_dist[i] + rev_dist[i];
    best_path_cost[i].second = i;
  }

  std::sort(best_path_cost,best_path_cost+n_states,lesscost);
  // now we have a list of states in order of increasing cost (poorness)
#ifdef DEBUGPRUNE
  Config::debug() << "Best path cost = " << best_path << "(reverse best path = " << rev_dist[0] << "); worst path allowed = " << worst_path << std::endl;
  /*  Config::debug() << "BEST PATH THROUGH STATE:" << std::endl;
      for (i=0;i<n_states;++i) {
      Config::debug() << best_path_cost[i].first << ' ' << stateName(best_path_cost[i].second) << std::endl;
      }*/
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
        for ( List<FSTArc>::erase_iterator a(s.arcs.erase_begin()), end = s.arcs.erase_end() ; a !=end  ;  ) {
          FLOAT_TYPE best_path_this_arc = (-a->weight.getLogImp())+for_dist[st]+rev_dist[a->dest];
#ifdef DEBUGPRUNE
          Config::debug() << "FSTArc " << st << ": ";
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
  delete[] best_path_cost;

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

  bool *visitedForward = NEW bool[nStates];
  bool *visitedBackward = NEW bool[nStates];
  //bool *discard = NEW bool[nStates];
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
     for ( List<FSTArc>::iterator a(states[i].arcs.begin()), end = states[i].arcs.end() ; a !=end  ; ++a ){
     if ((discard[i])
     || !(visitedForward[a->dest] && visitedBackward[(a->dest)])){ // if a state should be discarded remove its arcs from tie group, also an FSTArc must be removed if its destination state is discarded.
     a->groupId = NOGROUP ;
     }
     }
     }
     // end of Yaser's additions - Oct. 12 2000
     */
  removeMarkedStates(visitedForward);

  for ( i = 0 ; i < numStates() ; ++i ) {
    states[i].flush();
    for ( List<FSTArc>::erase_iterator a=states[i].arcs.erase_begin(),end = states[i].arcs.erase_end() ; a != end; )
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
  int *oldToNew = NEW int[numStates()];
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
  for ( unsigned i = 0 ; i < states.size() ; ++i ) {
    states[i].flush();
    states[i].renumberDestinations(oldToNew);
  }
  final = oldToNew[final];
  delete[] oldToNew;
}

int WFST::abort() {
  return 0;
}

ostream & operator << (ostream & o, const PathArc &p)
{
  const WFST *w=p.wfst;
  o << "(" << w->inLetter(p.in) << " : " << w->outLetter(p.out) << " / " << p.weight << " -> " << w->stateName(p.destState) << ")";
  return o;
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



List<List<PathArc> > * WFST::randomPaths(int k,int max_len)
{
  Assert(valid());
  List<List<PathArc> > *paths = NEW List<List<PathArc> >;
  if (!valid()) {
    //List<List<PathArc> >::iterator insertHere=paths->begin();
    for (int i=0;i<k;) {
      paths->push_front(List<PathArc>());
      if ( randomPath(insert_iterator<List<PathArc> >(paths->front(),paths->front().erase_begin()),max_len) == -1 ) {
        paths->pop_front();
      } else {
        ++i;
      }

    }
  }
  return paths;
}

/*
List<List<PathArc> > *WFST::bestPaths(int k)
{
  int nStates = numStates();
  Assert(valid());

  typedef List<List<PathArc> > LLP;
  LLP *paths = NEW List<List<PathArc> >;
  insert_iterator<LLP> path_adder(*paths,paths->erase_begin());
  //List<List<PathArc> >::iterator insertHere=paths->begin();

  Graph graph = makeGraph();
#ifdef DEBUGKBEST
  Config::debug() << "Calling KBest on WFST with k: "<<k<<'\n' << graph;
#endif

  FLOAT_TYPE *dist = NEW FLOAT_TYPE[nStates];
  Graph shortPathGraph = shortestPathTreeTo(graph, final,dist);
#ifdef DEBUGKBEST
  Config::debug() << "Shortest path graph: "<<k<<'\n' << shortPathGraph;
#endif
  shortPathTree = shortPathGraph.states;

  if ( shortPathTree[0].arcs.notEmpty() || final == 0 ) {

    List<PathArc> temp;
    //List<PathArc>::iterator path=temp.begin();
    //paths->push_back(temp);
    insert_iterator<List<PathArc> > here(temp,temp.erase_begin());
    insertShortPath(shortPathTree, 0, final, here);
    *path_adder++ = temp; //XXX unnecessary copy because of output iterator not giving reference to added item =(



    if ( k > 1 ) {
      GraphHeap::freeAll();
      Graph revPathTree = reverseGraph(shortPathGraph);
      pathGraph = NEW GraphHeap *[nStates];
      sidetracks = sidetrackGraph(graph, shortPathGraph, dist);
      bool *visited = NEW bool[nStates];
      for ( int i = 0 ; i < nStates ; ++i ) visited[i] = false;
      // IMPORTANT NOTE: depthFirstSearch recursively calls the function
      // passed as the last argument (in this  case "buildSidetracksHeap")
      //
      freeAllSidetracks();
      depthFirstSearch(revPathTree, final, visited, buildSidetracksHeap);
      if ( pathGraph[0] ) {
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
  void WFST::insertPathArc(GraphArc *gArc,List<PathArc>* l)
  {
  PathArc pArc;
  FSTArc *taken = (FSTArc *)gArc->data;
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
*/

#include "wfstio.cc"

#include "compose.cc"

