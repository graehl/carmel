#include "config.h"
#include "train.h"
#include "fst.h"
#include "weight.h"

//#define DEBUGTRAIN

void WFST::trainBegin(WFST::NormalizeMethod method,bool weight_is_prior_count, Weight smoothFloor) {
  //consolidateArcs();
  normalize(method);
  delete trn;
  trn = NEW trainInfo;
  //  trn->smoothFloor = smoothFloor;
  IOPair IO;
  DWPair DW;
  List<DWPair> *pLDW;
  HashTable<IOPair, List<DWPair> > *IOarcs =
    trn->forArcs = NEW HashTable<IOPair, List<DWPair> >[numStates()];
  HashTable<IOPair, List<DWPair> > *revIOarcs =
    trn->revArcs = NEW HashTable<IOPair, List<DWPair> >[numStates()];
  int s;
  for ( s = 0 ; s < numStates() ; ++s ){
    for ( List<Arc>::val_iterator aI=states[s].arcs.val_begin(),end=states[s].arcs.val_end(); aI != end; ++aI ) {
      IO.in = aI->in;
      IO.out = aI->out;
      int d = DW.dest = aI->dest;
      DW.arc = &(*aI);
      if ( !(pLDW = IOarcs[s].find(IO)) )
        pLDW = IOarcs[s].add(IO);
      DW.prior_counts = smoothFloor + weight_is_prior_count * aI->weight;
      // add in forward direction
      pLDW->push(DW);

      // reverse direction and add in reverse direction
      DW.dest = s;
      if ( !(pLDW = revIOarcs[d].find(IO)) )
        pLDW = revIOarcs[d].add(IO);
      pLDW->push(DW);
    }
  }

  Graph eGraph = makeEGraph();
  Graph revEGraph = reverseGraph(eGraph);
  trn->forETopo = NEW List<int>;
  {
    TopoSort t(eGraph,trn->forETopo);
    t.order_crucial();
    int b = t.get_n_back_edges();
    if ( b > 0 )
      Config::warn() << "Warning: empty-label subgraph has " << b << " cycles!  Training may not propogate counts properly" << std::endl;
    delete[] eGraph.states;
  }
  trn->revETopo = NEW List<int>;
  {
    TopoSort t(revEGraph,trn->revETopo);
    t.order_crucial();
    delete[] revEGraph.states;
  }

  trn->f = trn->b = NULL;
  trn->maxIn = trn->maxOut = 0;
  trn->totalEmpiricalWeight = 0;
#ifdef DEBUGTRAIN
  Config::debug() << "Just after training setup "<< *this ;
#endif
}

void WFST::trainExample(List<int> &inSeq, List<int> &outSeq, float weight)
{
  Assert(trn);
  Assert(weight > 0);
  IOSymSeq s;
  s.init(inSeq, outSeq, weight);
  trn->totalEmpiricalWeight += weight;
  trn->examples.push_front(s);
  //trn->examples.insert(trn->examples.end(),s);
  if ( s.i.n > trn->maxIn )
    trn->maxIn = s.i.n;
  if ( s.o.n > trn->maxOut )
    trn->maxOut = s.o.n;
}

#define EACHDW(a)   do {  for ( int s = 0 ; s < numStates() ; ++s ) \
      for ( HashIter<IOPair, List<DWPair> > ha(trn->forArcs[s]) ; ha ; ++ha ){ \
        List<DWPair>::val_iterator end = ha.val().val_end() ; \
                for ( List<DWPair>::val_iterator dw=ha.val().val_begin() ; dw !=end ; ++dw ) {\
                  a } } } while(0)


void WFST::trainFinish(Weight converge_arc_delta, Weight converge_perplexity_ratio, int maxTrainIter, float learning_rate_growth_factor, NormalizeMethod method)
{
  Assert(trn);
  int i, o, nSt = numStates();
  int maxIn = trn->maxIn;
  int maxOut = trn->maxOut;
  Weight ***f = trn->f = NEW Weight **[maxIn+1];
  Weight ***b = trn->b = NEW Weight **[maxIn+1];
  trn->nStates = nSt ; // Yaser - added for supporting a copy constrcutor for trainInfo this 7-26-2000
  for ( i = 0 ; i <= maxIn ; ++i ) {
    f[i] = NEW Weight *[maxOut+1];
    b[i] = NEW Weight *[maxOut+1];
    for ( o = 0 ; o <= maxOut ; ++o ) {
      f[i][o] = NEW Weight [nSt];
      b[i][o] = NEW Weight [nSt];
    }
  }


  int giveUp = 0;
  Weight lastChange;
  Weight lastPerplexity;
  lastPerplexity.setInfinity();

  if ( trn->totalEmpiricalWeight > 0 ) {
    EACHDW(dw->prior_counts *= trn->totalEmpiricalWeight;);
  }

  float learning_rate=1;
  Weight pp_ratio;
  lastChange.setInfinity();
  for ( ; ; ) {
    Weight newPerplexity;
    ++giveUp;
    if ( giveUp > maxTrainIter ) {
      Config::log()  << "Maximum number of iterations (" << maxTrainIter << ") reached before convergence criteria of " << converge_arc_delta << " was met - greatest arc weight change was " << lastChange << "\n";
      break;
    }
#ifdef DEBUGTRAIN
  Config::debug() << "Starting iteration: " << giveUp << '\n';
#endif

    newPerplexity = train_estimate(); //lastPerplexity.isInfinity() // only delete no-path training the first time, in case we screw up with our learning rate

    Config::log() << "i=" << giveUp << " (scale=" << learning_rate << "): per-observation perplexity = " << newPerplexity ;
	
    if ( lastPerplexity.isInfinity() ) {
      Config::log() << std::endl;
	  pp_ratio.setZero();
    } else {
      pp_ratio = newPerplexity/lastPerplexity; //TODO: take 1/log(newPerplexity)th root? i.e. from EM delta=(L'-L)/abs(L')
														// we already scale for L' by dividing by #observations.  could go either way.
	  Config::log() << " (perplexity ratio = " << pp_ratio << "), max{d(weight)}=" << lastChange << std::endl;
	}

    if (  pp_ratio >= converge_perplexity_ratio ) {
	    if ( learning_rate > 1 )
			goto RESET_RATE;
		Config::log() << "Converged - per-example perplexity ratio exceeds " << converge_perplexity_ratio << " after " << giveUp << " iterations.\n";
        break;
	} else {
		#define MAX_LEARNING_RATE_EXP 20
		if (learning_rate < MAX_LEARNING_RATE_EXP)
			learning_rate *= learning_rate_growth_factor;
    }

	lastChange = train_maximize(method,learning_rate);

	if (lastChange <= converge_arc_delta ) {
			Config::log() << "Converged - maximum weight change less than " << converge_arc_delta << " after " << giveUp << " iterations.\n";
			break;
	}

    lastPerplexity=newPerplexity;

	continue;
RESET_RATE:
	Config::log() << "Failed to improve; reset learning rate to 1 and undid last iteration" << std::endl;
	learning_rate=1;
	undo_train_scale();
  }

  delete[] trn->forArcs;
  delete[] trn->revArcs;
  delete trn->forETopo;
  delete trn->revETopo;
#ifdef N_E_REPS
#endif
  for ( List<IOSymSeq>::val_iterator seq=trn->examples.val_begin(),end = trn->examples.val_end() ; seq !=end ; ++seq )
    seq->kill();

  for ( i = 0 ; i <= maxIn ; ++i ) {
    for ( o = 0 ; o <= maxOut ; ++o ) {
      delete[] f[i][o];
      delete[] b[i][o];
    }
    delete[] f[i];
    delete[] b[i];
  }
  delete[] f;
  delete[] b;
  delete trn;
  trn = NULL;
}

// possible speedups:
// * exclude states that are definitely not on any path start->finish matching i:o
// ** quick check: exclude if none of the outgoing transitions are (input|*e*):(output|*e*) for some input, output in sequence
/*
// ** complete check: integrate search and summation of paths - search state: (s,i,o)
 constraint: can't visit (dequeue) (s,i,o) until already for all t (t,i-1,o-1),(t,i,o-1),(t,i-1,o) and all u (u,i,o) s.t. u->*e*->s
 easy to meet first 3 using for i: for o: loop; add state to agenda for next (i|i+1,o|o+1)
 when you have an *e* leaving something on the agenda, you need to add the dest to the head of the agenda, and, you must preorder the agenda so that if a->*e*->b, a cannot occur after b in the agenda.  you could sort the agenda by reverse dfs finishing times (dfs on the whole e graph though)

*/
// ** in order for excluding states to be worthwhile in O(s*i*o) terms, have to take as input a 0-initialized w matrix and clear each non-0 entry after it is no longer in play.  ouch - that means all the lists (of nonzero values) need to be kept around until after people are done playing with the w

void sumPaths(int nSt, int start, Weight ***w, HashTable<IOPair, List<DWPair> > *IOarcs, List<int> * eTopo, int nIn, int *inLet, int nOut, int *outLet)
{
  int i, o, s;
#ifdef N_E_REPS
  Weight *wNew = NEW Weight [nSt];
  Weight *wOld = NEW Weight [nSt];
#endif
  for ( i = 0 ; i <= nIn ; ++i )
    for ( o = 0 ; o <= nOut ; ++o )
      for ( s = 0 ; s < nSt ; ++s )
        w[i][o][s].setZero();

  w[0][0][start] = 1;

  IOPair IO;
  List<DWPair> *pLDW;

  for ( i = 0 ; i <= nIn ; ++i )
    for ( o = 0 ; o <= nOut ; ++o ) {
#ifdef DEBUGFB
      Config::debug() <<"("<<i<<","<<o<<")\n";
#endif
      IO.in = 0;
      IO.out = 0;
#ifdef N_E_REPS
      for ( s = 0 ; s < nSt; ++s )
        wNew[s] = w[i][o][s];
#endif
      for ( List<int>::const_iterator topI=eTopo->const_begin(),end=eTopo->const_end() ; topI != end; ++topI ) {
        s = *topI;
        if ( (pLDW = IOarcs[s].find(IO)) ){
          for ( List<DWPair>::const_iterator dw=pLDW->const_begin(),end2 = pLDW->const_end() ; dw !=end2; ++dw ){
#ifdef DEBUGFB
            Config::debug() << "w["<<i<<"]["<<o<<"]["<<dw->dest<<"] += " <<  "w["<<i<<"]["<<o<<"]["<<s<<"] * weight("<< *dw<<") ="<< w[i][o][dw->dest] <<" + " << w[i][o][s] <<" * "<< dw->weight() <<" = "<< w[i][o][dw->dest] <<" + " << w[i][o][s] * dw->weight() <<" = ";
#endif
            w[i][o][dw->dest] += w[i][o][s] * dw->weight();
#ifdef DEBUGFB
            Config::debug() << w[i][o][dw->dest] << '\n';
#endif
          }
        }
      }
#ifdef N_E_REPS
      // caveat: this method is wrong, although it will converge as N_E_REPS -> inf assuming the null transitions are normalized per source state, it can converge higher than it should by counting the same paths multiple times.  thus, N_E_REPS is not enabled =D
      for ( int rep = 0 ; rep < N_E_REPS ; ++rep ) {
        for ( s = 0 ; s < nSt; ++s )
          wOld[s] = wNew[s];
        for ( s = 0 ; s < nSt; ++s )
          wNew[s] = w[i][o][s];
        for ( s = 0 ; s < nSt; ++s ) {
          if ( (pLDW = IOarcs[s].find(IO)) ){
            for ( List<DWPair>::const_iterator dw=pLDW->const_begin(),end = pLDW->const_end() ; dw != end; ++dw )
              wNew[dw->dest] += wOld[s] * dw->weight();
          }
        }
      }
#endif
      for ( s = 0 ; s < nSt; ++s ) {
#ifdef N_E_REPS
        w[i][o][s] = wNew[s];
#endif
        if ( w[i][o][s].isZero() )
          continue;
        if ( o < nOut ) {
          IO.in = 0;
          IO.out = outLet[o];
          if ( (pLDW = IOarcs[s].find(IO)) ){
            for ( List<DWPair>::const_iterator dw=pLDW->const_begin(),end = pLDW->const_end() ; dw != end; ++dw ){
#ifdef DEBUGFB
              Config::debug() << "w["<<i<<"]["<<o+1<<"]["<<dw->dest<<"] += " <<  "w["<<i<<"]["<<o<<"]["<<s<<"] * weight ("<< *dw<<") ="<< w[i][o+1][dw->dest] <<" + " << w[i][o][s] <<" * "<< dw->weight() <<" = "<< w[i][o+1][dw->dest] <<" + " << w[i][o][s] * dw->weight() <<" = ";
#endif
              w[i][o+1][dw->dest] += w[i][o][s] * dw->weight();
#ifdef DEBUGFB
              Config::debug() << w[i][o+1][dw->dest] << '\n';
#endif
            }
          }
          if ( i < nIn ) {
            IO.in = inLet[i];
            IO.out = outLet[o];
            if ( (pLDW = IOarcs[s].find(IO)) ){
              for ( List<DWPair>::const_iterator dw=pLDW->const_begin(),end = pLDW->const_end() ; dw != end; ++dw ){
#ifdef DEBUGFB
                Config::debug() << "w["<<i+1<<"]["<<o+1<<"]["<<dw->dest<<"] += " <<  "w["<<i<<"]["<<o<<"]["<<s<<"] * weight ("<< *dw<<") ="<< w[i+1][o+1][dw->dest] <<" + " << w[i][o][s] <<" * "<< dw->weight() <<" = "<< w[i+1][o+1][dw->dest] <<" + " << w[i][o][s] * dw->weight() <<" = ";
#endif
                w[i+1][o+1][dw->dest] += w[i][o][s] * dw->weight();
#ifdef DEBUGFB
                Config::debug() << w[i+1][o+1][dw->dest] << '\n';
#endif
              }
            }
          }
        }
        if ( i < nIn ) {
          IO.in = inLet[i];
          IO.out = 0;
          if ( (pLDW = IOarcs[s].find(IO)) ){
            for ( List<DWPair>::const_iterator dw=pLDW->const_begin(),end = pLDW->const_end() ; dw != end; ++dw ){
#ifdef DEBUGFB
              Config::debug() << "w["<<i+1<<"]["<<o<<"]["<<dw->dest<<"] += " <<  "w["<<i<<"]["<<o<<"]["<<s<<"] * weight ("<< *dw <<") ="<< w[i+1][o][dw->dest] <<" + " << w[i][o][s] <<" * "<< dw->weight() <<" = "<< w[i+1][o][dw->dest] <<" + " << w[i][o][s] * dw->weight() <<" = ";
#endif
              w[i+1][o][dw->dest] += w[i][o][s] * dw->weight();
#ifdef DEBUGFB
              Config::debug() << w[i+1][o][dw->dest] << '\n';
#endif
            }
          }
        }
      }
    }
#ifdef N_E_REPS
  delete[] wNew;
  delete[] wOld;
#endif
}

// similar to transposition but not quite: instead of replacing w.ij with w.ji, replace w.ij with w.(I-i)(J-j) ... matrix has the same dimensions.  it's a 180 degree rotation, not a reflection about the identity line
void reverseMatrix(Weight ***w,int nIn, int nOut) {
  int i;
  for ( i = 0 ; i <= nIn/2 ; ++i ) {
    Weight **temp = w[i];
    w[i] = w[nIn - i];
    w[nIn - i] = temp;
  }
  for ( i = 0 ; i <= nIn ; ++i )
    for ( int o = 0 ; o <= nOut/2 ; ++o ) {
      Weight *temp = w[i][o];
      w[i][o] = w[i][nOut - o];
      w[i][nOut - o] = temp;
    }
}

void forwardBackward(IOSymSeq &s, trainInfo *trn, int nSt, int final)
{
  Assert(trn);
#ifdef DEBUGFB
  Config::debug() << "training example: \n"<<s << "\nForward\n" ;
#endif
  sumPaths(nSt, 0, trn->f, trn->forArcs, trn->forETopo, s.i.n, s.i.let, s.o.n, s.o.let);
#ifdef DEBUGFB
  Config::debug() << "\nBackward\n";
#endif
  sumPaths(nSt, final, trn->b, trn->revArcs, trn->revETopo, s.i.n, s.i.rLet, s.o.n, s.o.rLet);
  
  Weight ***w = trn->b;
  int nIn = s.i.n;
  int nOut = s.o.n;
  // since the backward paths were obtained on the reverseed input/output, reverse them back
  reverseMatrix(w,nIn,nOut);
#ifdef DEBUGTRAINDETAIL // Yaser 7-20-2000
  Config::debug() << "\nForwardProb/BackwardProb:\n";
  for (i = 0 ;i<= nIn ; ++i){
    for (int o = 0 ; o <= nOut ; ++o){
      Config::debug() << i << ':' << o << " (" ;
      for (int s = 0 ; s < nSt ; ++s){
        Config::debug() << trn->f[i][o][s] <<'/'<<trn->b[i][o][s];
        if (s < nSt-1)
          Config::debug() << ' ' ;
      }
      Config::debug() <<')'<<std::endl;
      if(o < nOut-1)
        Config::debug() <<' ' ;
    }
    Config::debug() <<std::endl;
  }
#endif
}

void WFST::undo_train_scale(void)
{
	EACHDW(dw->weight() = dw->em_weight;);
}

Weight WFST::train_estimate(bool delete_bad_training)
{
  Assert(trn);
  int i, o, s, nIn, nOut, *letIn, *letOut;

  // for perplexity
  Weight prodModProb = 1;

  Weight ***f = trn->f;
  Weight ***b = trn->b;
  HashTable <IOPair, List<DWPair> > *IOarcs;
  List<DWPair> * pLDW;
  IOPair io;
  EACHDW(dw->counts.setZero(););

  List<IOSymSeq>::erase_iterator seq=trn->examples.erase_begin(),lastExample=trn->examples.erase_end();


  //#ifdef DEBUGTRAIN
  int train_example_no = 0 ; // Yaser 7-13-2000
  //#endif
  while (seq != lastExample) { // loop over all training examples

    //#ifdef DEBUGTRAIN // Yaser 13-7-2000 - Debugging messages ..
    ++train_example_no ;
#define EXAMPLES_PER_DOT 10
#define EXAMPLES_PER_NUMBER (70*EXAMPLES_PER_DOT)
    if (train_example_no % EXAMPLES_PER_DOT == 0)
      Config::log() << '.' ;
    if (train_example_no % EXAMPLES_PER_NUMBER == 0)
      Config::debug() << train_example_no << '\n' ;
    //#endif
    nIn = seq->i.n;
    nOut = seq->o.n;
    forwardBackward(*seq, trn, numStates(), final);
#ifdef DEBUGTRAIN
    Config::debug() << '\n';
    for ( i = 0 ; i < nIn ; ++i ) {
      Config::debug() << (*in)[seq->i.let[i]] << ' ' ;
      for ( o = 0 ; o < nOut ; ++o ) {
        Config::debug() << (*out)[seq->o.let[o]] << ' ' ;
        Config::debug() << '(' << f[i][o][0];
        for ( s = 1 ; s < numStates() ; ++s ) {
          Config::debug() << ' ' << f[i][o][s];
        }
        Config::debug() << ") ";
      }
      Config::debug() << "\t\t";
      for ( o = 0 ; o < nOut ; ++o ) {
        Config::debug() << (*out)[seq->o.let[o]] << ' ' ;
        Config::debug() << '(' << b[i][o][0];
        for ( s = 1 ; s < numStates() ; ++s ) {
          Config::debug() << ' ' << b[i][o][s];
        }
        Config::debug() << ") ";
      }
      Config::debug() << '\n';
    }
#endif
    Weight fin = f[nIn][nOut][final];
#ifdef DEBUGTRAIN
    Config::debug()<<"Forward prob = " << fin << std::endl;
#endif
	if (delete_bad_training)
		if ( !(fin.isPositive()) ) {
		Config::warn() << "No accepting path in transducer for input/output:\n";
		for ( i = 0 ; i < nIn ; ++i )
			Config::warn() << (*in)[seq->i.let[i]] << ' ';
		Config::warn() << '\n';
		for ( o = 0 ; o < nOut ; ++o )
			Config::warn() << (*out)[seq->o.let[o]] << ' ';
		Config::warn() << '\n';
		seq=trn->examples.erase(seq);
		continue;
		}
#ifdef ALLOWED_FORWARD_OVER_BACKWARD_EPSILON
    Weight fin2 = b[0][0][0];
#ifdef DEBUGTRAIN
    Config::debug()<<"Backward prob = " << fin2 << std::endl;
#endif
    double ratio = (fin > fin2 ? fin/fin2 : fin2/fin).getReal();
    double e = ratio - 1;
    if ( e > ALLOWED_FORWARD_OVER_BACKWARD_EPSILON )
      Config::warn() << "Warning: forward prob vs backward prob relative difference of " << e << " exceeded " << ALLOWED_FORWARD_OVER_BACKWARD_EPSILON << " (with infinite precision, it should be 0).\n";
#endif

    letIn = seq->i.let;
    letOut = seq->o.let;

    EACHDW(dw->scratch.setZero(););

    for ( i = 0 ; i <= nIn ; ++i ) // go over all symbols in input in the training pair
      for ( o = 0 ; o <= nOut ; ++o ) // go over all symbols in the output pair
        for ( s = 0 ; s < numStates() ; ++s ) {
          IOarcs = trn->forArcs + s;
          if ( i < nIn ) { // input is not epsilon
            io.in = letIn[i];
            if ( o < nOut ) { // output is also not epsilon
              io.out = letOut[o];
              if ( (pLDW = IOarcs->find(io)) ){
                for ( List<DWPair>::val_iterator dw=pLDW->val_begin(),end = pLDW->val_end() ; dw !=end ; ++dw )
                  dw->scratch += f[i][o][s] * dw->weight() * b[i+1][o+1][dw->dest];
              }
            }
            io.out = 0; // output is epsilon, input is not
            if ( (pLDW = IOarcs->find(io)) ){
              for ( List<DWPair>::val_iterator dw=pLDW->val_begin(),end = pLDW->val_end() ; dw !=end ; ++dw )
                dw->scratch += f[i][o][s] * dw->weight() * b[i+1][o][dw->dest];
            }
          }
          io.in = 0; // input is epsilon
          if ( o < nOut ) { // input is epsilon, output is not
            io.out = letOut[o];
            if ( (pLDW = IOarcs->find(io)) ){
              for ( List<DWPair>::val_iterator dw=pLDW->val_begin(),end = pLDW->val_end() ; dw !=end ; ++dw )
                dw->scratch += f[i][o][s] * dw->weight() * b[i][o+1][dw->dest];
            }
          }
          io.out = 0; // input and output are both epsilon
          if ( (pLDW = IOarcs->find(io)) ){
            for ( List<DWPair>::val_iterator dw=pLDW->val_begin(),end = pLDW->val_end() ; dw !=end ; ++dw )
              dw->scratch += f[i][o][s] * dw->weight() * b[i][o][dw->dest];
          }
        }
    EACHDW(if (!dw->scratch.isZero()) dw->counts += (dw->scratch / fin) * (Weight)seq->weight;);
    prodModProb *= (fin^seq->weight); // since perplexity = 2^((-1/n)*sum(log2 prob)), we can take prod(prob)^(1/n) instead
    ++seq;
  } // end of while(training examples)
  
    return root(prodModProb ^ (-1 / trn->totalEmpiricalWeight),trn->totalEmpiricalWeight); // return per-symbol perplexity.  simplify to root(prodModProb,-trn->totalEmpiricalWeight * trn->totalEmpiricalWeight) ?
	
}

Weight WFST::train_maximize(WFST::NormalizeMethod method,Weight delta_scale)
{
 Assert(trn);

  int pGroup;

#define DUMPDW  do { for ( s = 0 ; s < numStates() ; ++s ) \
    for ( HashIter<IOPair, List<DWPair> > ha(trn->forArcs[s]) ; ha ; ++ha ){ \
      List<DWPair>::const_iterator end = ha.val().const_end() ; \
      for ( List<DWPair>::const_iterator dw=ha.val().const_begin() ; dw !=end; ++dw ){ \
        if ( isTiedOrLocked(pGroup = (dw->arc)->groupId) ) \
          Config::debug() << pGroup << ' ' ; \
        Config::debug() << s << "->" << *dw->arc <<  " weight " << dw->weight() << " scratch: "<< dw->scratch  <<" counts " <<dw->counts  << '\n'; \
      } \
        } } while(0)
  
#ifdef DEBUGTRAINDETAIL
  Config::debug() << "\nWeights before prior smoothing\n";
  DUMPDW;
#endif
EACHDW (
  if ( !isLocked(pGroup = (dw->arc)->groupId) ) { // if the group is tied, and the group number is zero, then the old weight doe not change. Otherwise update as follows
          //Weight &w=dw->weight();
          dw->scratch = dw->weight();   // old weight - Yaser: this is needed only to calculate change in weight later on ..
          //Weight &counts = dw->counts;
          dw->weight() = dw->counts + dw->prior_counts; // new (unnormalized weight)
        }
 );
#ifdef DEBUGTRAINDETAIL
  Config::debug() << "\nWeights before normalization\n";
  DUMPDW;
#endif
  normalize(method);
#ifdef DEBUGTRAINDETAIL
  Config::debug() << "\nWeights after normalization\n";
  DUMPDW;
#endif
  // find maximum change for convergence (and scale change by delta_scale)
  Weight change, maxChange;
  EACHDW(
	   dw->em_weight = dw->weight();
       if ( !isLocked((dw->arc)->groupId) )
   	      dw->weight() = dw->scratch * (
				(dw->weight() / dw->scratch) ^ delta_scale.getReal());
  );
    
  if (delta_scale != 1)
	  normalize(method);

  EACHDW(
	  if (!isLocked(pGroup = (dw->arc)->groupId) ) {
  	  	    if (dw->weight() > dw->scratch) {
				change = dw->weight() - dw->scratch;
			//	dw->weight() = dw->scratch + delta_scale*change;
			} else {
				change = dw->scratch - dw->weight();
				//dw->weight() = dw->scratch - delta_scale*change;
			}
			if ( change > maxChange )
				maxChange = change;			
	  }
  );
  return maxChange; // TODO: recompute change after delta_scale/normalize?
}

Weight ***WFST::forwardSumPaths(List<int> &inSeq, List<int> &outSeq)
{
  int i, o, s;
  int nIn = inSeq.count_length();
  int nOut = outSeq.count_length();
  int *inLet = NEW int[nIn];
  int *outLet = NEW int[nOut];
  int *pi;

  pi = inLet;
  for ( List<int>::const_iterator inL=inSeq.const_begin(), end = inSeq.const_end() ; inL != end; ++inL )
    *pi++ = *inL;

  pi = outLet;
  for ( List<int>::const_iterator outL=outSeq.const_begin(),end = outSeq.const_end() ; outL != end; ++outL )
    *pi++ = *outL;

  HashTable<IOPair, List<DWPair> > *IOarcs =
    NEW HashTable<IOPair, List<DWPair> >[numStates()];

  IOPair IO;
  DWPair DW;
  List<DWPair> *pLDW;

  for ( s = 0 ; s < numStates() ; ++s ){
   for ( List<Arc>::val_iterator a=states[s].arcs.val_begin(),end = states[s].arcs.val_end(); a != end ; ++a ) {
      IO.in = a->in;
    IO.out = a->out;
    DW.dest = a->dest;
    DW.arc = &(*a);
    if ( !(pLDW = IOarcs[s].find(IO)) )
      pLDW = IOarcs[s].add(IO);
    pLDW->push(DW);
  }
}

List<int> eTopo;
{
  Graph eGraph = makeEGraph();
  TopoSort t(eGraph,&eTopo);
  t.order_crucial();
  int b = t.get_n_back_edges();
  if ( b > 0 )
    Config::warn() << "Warning: empty-label subgraph has " << b << " cycles!  May not add paths with those cycles properly" << std::endl;
  delete[] eGraph.states;
}

Weight ***w = NEW Weight **[nIn+1];
for ( i = 0 ; i <= nIn ; ++i ) {
  w[i] = NEW Weight *[nOut+1];
  for ( o = 0 ; o <= nOut ; ++o )
    w[i][o] = NEW Weight [numStates()];
}
sumPaths(numStates(), 0, w, IOarcs, &eTopo, nIn, inLet, nOut, outLet);

delete[] IOarcs;
delete[] inLet;
delete[] outLet;
return w;
}


ostream & operator << (ostream &out, const trainInfo &t){ // Yaser 7-20-2000

  out << "Forward Edges Topologically Sorted\n" ;
  if (t.forETopo)
    out << *(t.forETopo)<<'\n';
  else out << "not set yet!";
  return(out);
}



ostream& operator << (ostream &out, struct State &s){ // Yaser 7-20-2000
  out << s.arcs << '\n';
  return(out);
}

ostream & operator << (ostream &o, IOPair p)
{
  return o << '(' << p.in << ' ' << p.out << ')';
}

int operator == (IOPair l, IOPair r) { return l.in == r.in && l.out == r.out; }

ostream & operator << (ostream &o, DWPair p)
{
  return o << *(p.arc)  ;
}


ostream & hashPrint(HashTable<IOPair, List<DWPair> > &h, ostream &o) {
  HashIter<IOPair,List<DWPair> > i(h);
  if ( !i ) return o;
  o << '(' << i.key() << ' ' << i.val() << ')';
  while ( ++i )
    o << " (" << i.key() << ' ' << i.val() << ')';
  return o;
}


ostream & operator << (ostream & out , const symSeq & s){   // Yaser 7-21-2000
  for (int i=0 ; i < s.n ; ++i)
    out << s.let[i] ;
  out << '\n';
  return(out);
}

ostream & operator << (ostream & out , const IOSymSeq & s){   // Yaser 7-21-2000
  out << s.i << s.o ;
  return(out);
}

