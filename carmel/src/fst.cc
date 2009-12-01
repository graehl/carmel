#include <graehl/shared/config.h>
#include <cctype>
#include <graehl/carmel/src/fst.h>
#include <graehl/shared/kbest.h>
#include <graehl/shared/array.hpp>
#include <graehl/shared/genio.h>

namespace graehl {

const int WFST::per_line_index = ios_base::xalloc();
const int WFST::arc_format_index = ios_base::xalloc();
THREADLOCAL int WFST::default_per_line = WFST::STATE;
THREADLOCAL int WFST::default_arc_format = WFST::BRIEF;

#define DP_SLIDE_36
// slide 36 gives what seems like a bad norm method: scale(c)/scale(sum{c_i}).  slide 38 looks better: scale(c)/sum{scale(c_i)}.  update: slide 38 was wrong, and is revised in http://www.cs.berkeley.edu/~pliang/papers/tutorial-acl2007.pdf

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

template <class charT, class Traits>
std::basic_ostream<charT,Traits>&
operator <<
              (std::basic_ostream<charT,Traits>& os, const NormGroupIter &arg)
{
    return gen_inserter(os,arg);
}

static void NaNCheck(const Weight *w) {
    w->NaNCheck();
}

void WFST::normalize(NormalizeMethod const& method,bool uniform_zero_normgroups)
{
    norm_group_by group=method.group;

    if (group==NONE)
        return;
    if (group==CONDITIONAL)
        indexInput();

    graehl::mean_field_scale const& scale=method.scale;

    // NEW plan:
    // step 1: compute sum of counts for non-locked arcs, and divide it by (1-(sum of locked arcs)) to reserve appropriate counts for the locked arcs
    // step 2: for tied arc groups, add these inferred counts to the group state counts total.  also sum group arc counts total.
    // step 3: assign tied arc weights; trouble: tied arcs sharing space with inflexible tied arcs.  under- or over- allocation can result ...
    //   ... alternative: give locked arcs implied counts in the tie group; norm-group having tie-group arcs, with highest locked arc sum R divides unscaled tie group state counts total by (1-R) instead of dividing individual state counts by (1-sum).  this ensures that tied arcs are kept small enough to make room for locked ones in ALL states and should leave some room for normal arcs as well
    // step 4: give normal arcs their share of what's left, if anything

    Weight addc=method.add_count;
    int pGroup;
    HashTable<IntKey, Weight> groupArcTotal;
    HashTable<IntKey, Weight> groupStateTotal;
    HashTable<IntKey, Weight> groupMaxLockedSum;
    // global pass 1: compute the sum of unnormalized weights for each normalization group.  sum for each arc in a tie group, its weight and its normalization group's weight.
    for (NormGroupIter g(group,*this); g.moreGroups(); g.nextGroup()) {
#ifdef DEBUGNORMALIZE
        Config::debug() << "Normgroup=" << g;
#endif
        Weight sum,locked_sum; // =0, sum of probability of all arcs that has this input
        for ( g.beginArcs(); g.moreArcs(); g.nextArc()) {
            FSTArc & a=**g;
            Weight &w=a.weight;
            w+=addc;
            if (isLocked(a.groupId)) //note: training does not set any counts for locked arcs.  so this is the original weight
                locked_sum += w;
            else {
                sum += w;
            }
        }
#ifdef DEBUGNORMALIZE
        Config::debug() << " locked_sum=" << locked_sum << " sum=" << sum << std::endl;
#endif
        for ( g.beginArcs(); g.moreArcs(); g.nextArc()) {
            FSTArc const& a=**g;
            if ( isTied(pGroup = a.groupId) ) {
                groupArcTotal[pGroup] += a.weight; // default init is to 0
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
    for (NormGroupIter g(group,*this); g.moreGroups(); g.nextGroup()) {
        Weight normal_sum;//=0
        Weight reserved;// =0
        Assert(reserved.isZero()&&normal_sum.isZero());
        // pass 2a: assign tied (and locked) arcs their weights, taking 'reserved' weight from the normal arcs in their group
        // tied arc weight = sum (over arcs in tie group) of weight / sum (over arcs in tie group) of norm-group-total-weight
        // also, compute sum of normal arcs
        for ( g.beginArcs(); g.moreArcs(); g.nextArc()) {
            FSTArc & a=**g;
            if ( isTied(pGroup=a.groupId) ) { // tied:
                Weight groupNorm = *find_second(groupStateTotal,(IntKey)pGroup); // can be 0 if no counts at all for any states of group
                Weight gmax=*find_second(groupMaxLockedSum,(IntKey)pGroup);
                NANCHECK(gmax);
                Weight one(1.);
                if ( gmax > one) {
                    a.weight.setZero();
                } else {
                    if ( !gmax.isZero() )
                        groupNorm /= (one - gmax); // as described in NEW plan above: ensure tied arcs leave room for the worst case competing locked arcs sum in any norm-group
                    NANCHECK(groupNorm);

                    Weight groupTotal=*find_second(groupArcTotal,(IntKey)pGroup);
                    NANCHECK(groupTotal);
                    if (!groupTotal.isZero()) { // then groupNorm non0 also
                        a.weight =
                            scale(groupTotal)/scale(groupNorm)
                            ;

                        reserved += a.weight;
                    } else
                        a.weight.setZero();
                    NANCHECK(reserved);
                }
            } else if (isLocked(pGroup)) { // locked:
                reserved += a.weight;
                NANCHECK(reserved);
            } else { //normal
                normal_sum+=a.weight;
            }
        }

#ifdef DEBUGNORMALIZE
        if ( reserved > 1.001 )
            Config::warn() << "Warning: sum of reserved arcs for " << g << " = " << reserved << " - should not exceed 1.0\n";
#endif

        // pass 2b: give normal arcs their share of however much is left
        Weight fraction_remain = 1.;
        fraction_remain -= reserved;
        NANCHECK(fraction_remain);
        bool something_left_for_normal=!fraction_remain.isZero();
        if (something_left_for_normal  && (uniform_zero_normgroups || !normal_sum.isZero())) {
            NANCHECK(normal_sum);
            Weight scaled_sum=scale(normal_sum);
            for ( g.beginArcs(); g.moreArcs(); g.nextArc()) {
                FSTArc & a=**g;
                if (isNormal(a.groupId)) {
                    a.weight = fraction_remain *
                        scale(a.weight)/scaled_sum
                        ;
                    NANCHECK(a.weight);
                }
            }
        } else // nothing left, sorry
            for ( g.beginArcs(); g.moreArcs(); g.nextArc()) {
                FSTArc & a=**g;
                if (isNormal(a.groupId))
                    a.weight.setZero();
            }
    }

#ifdef CHECKNORMALIZE
    for (NormGroupIter g(group,*this); g.moreGroups(); g.nextGroup()) {
        Weight sum;
        for ( g.beginArcs(); g.moreArcs(); g.nextArc())
            sum += (*g)->weight;
#define NORM_EPSILON .01
        if ( sum > 1+NORM_EPSILON || sum < 1-NORM_EPSILON)
            Config::warn() << "Warning: sum of normalized arcs for " << g << " = " << sum << " - should equal 1.0\n";
    }
#endif

    if (group == CONDITIONAL)
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
        states[s].flush();
        List<FSTArc> &arcs = states[s].arcs;
        for ( List<FSTArc>::erase_iterator a=arcs.erase_begin(),end=arcs.erase_end(); a !=end ; ) {
            if ( isTied(pGroup = a->groupId) ) {
                if ( (pWeight = find_second(groupWeight,(IntKey)pGroup)) ) {
                    a->weight = *pWeight;
                    ++a;
                } else {
                    a=arcs.erase(a);
                }
            } else {
                ++a;
            }
        }
    }
}

void WFST::unTieGroups() {
    for ( int s = 0 ; s < numStates() ; ++s ){
        for ( List<FSTArc>::val_iterator a=states[s].arcs.val_begin(),end = states[s].arcs.val_end(); a != end ; ++a )
            a->groupId = no_group ;
    }
}

void WFST::lockArcs() {
    for ( int s = 0 ; s < numStates() ; ++s ){
        for ( List<FSTArc>::val_iterator a=states[s].arcs.val_begin(),end = states[s].arcs.val_end(); a != end ; ++a )
            a->groupId = 0 ;
    }
}

unsigned WFST::numberArcsFrom(unsigned label) {
    Assert ( label > 0 );
    for ( int s = 0 ; s < numStates() ; ++s ){
        for ( List<FSTArc>::val_iterator a=states[s].arcs.val_begin(),end = states[s].arcs.val_end(); a != end ; ++a )
            a->groupId = label++;
    }
    return label;
}


void WFST::invert()
{
    Assert(valid());
    int temp;
    in_alph().swap(out_alph());
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

Graph WFST::makeGraph()
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
            gArc.src = i;
            gArc.dest = l->dest;
            gArc.weight = l->weight.getCost();
            gArc.data_as<FSTArc *>() = &*l;
            Assert(gArc.dest < numStates() && gArc.src < numStates());
            g[i].arcs.push(gArc);
        }
    }
    Graph ret;
    ret.states = g;
    ret.nStates = numStates();
    return ret;
}

Graph WFST::makeEGraph()
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
                gArc.src = i;
                gArc.dest = l->dest;
                gArc.weight = l->weight.getCost();
                gArc.data_as<FSTArc *>() = &*l;
                Assert(gArc.dest < numStates() && gArc.src < numStates());
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
       a->groupId = no_group ;
       }
       }
       }
       // end of Yaser's additions - Oct. 12 2000
       */
    removeMarkedStates(visitedForward);

    for ( i = 0 ; i < numStates() ; ++i ) {
        states[i].remove_epsilons_to(i);
    }

    delete[] visitedBackward;
    delete[] visitedForward;
    // delete[] discard;
    delete[] revGraph;
    delete[] graph;
}

void WFST::consolidateArcs(bool sum,bool clamp)
{
    for ( int i = 0 ; i < numStates() ; ++i )
        states[i].reduce(sum,clamp);
}

void WFST::removeMarkedStates(bool marked[])
{
    Assert(valid());
    int *oldToNew = NEW int[numStates()];
    unsigned n_pre=numStates();

    if ( n_pre != graehl::indices_after_remove_marked(oldToNew,marked,n_pre) ) { // something removed
        stateNames.removeMarked(marked, oldToNew,n_pre);
        remove_marked_swap(states,marked); //states.removeMarked(marked);
        for ( unsigned i = 0 ; i < states.size() ; ++i ) {
            states[i].renumberDestinations(oldToNew);
        }
        final = oldToNew[final]==-1 ? invalid_state : oldToNew[final];
    }

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
            if ( randomPath(paths->front().back_inserter(),max_len) == -1 ) {
                paths->pop_front();
            } else {
                ++i;
            }

        }
    }
    return paths;
}


}

#include <graehl/carmel/src/wfstio.cc>

#include <graehl/carmel/src/compose.cc>

