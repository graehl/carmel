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
      if ( !(pLDW = IOarcs[s].find_second(IO)) )
        pLDW = IOarcs[s].add(IO);
      DW.prior_counts = smoothFloor + weight_is_prior_count * aI->weight;
      // add in forward direction
      pLDW->push(DW);

      // reverse direction and add in reverse direction
      DW.dest = s;
      if ( !(pLDW = revIOarcs[d].find_second(IO)) )
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

void WFST::trainExample(List<int> &inSeq, List<int> &outSeq, FLOAT_TYPE weight)
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
  for ( HashTable<IOPair, List<DWPair> >::const_iterator ha=trn->forArcs[s].begin(); ha!=trn->forArcs[s].end() ; ++ha ){ \
                for ( List<DWPair>::val_iterator dw=ha->second.val_begin(),dend = ha->second.val_end() ; dw !=dend ; ++dw ) {\
                  a } } } while(0)


struct WeightAccum {
	Weight sum;
	int n_nonzero;
	int n;
	void operator ()(Weight *w) {
		w->NaNCheck();
		sum += *w;
		++n;
		if (w->isPositive())
			++n_nonzero;
	}
	WeightAccum() {
		reset();
	}
	void reset() {
		n=n_nonzero=0;
		sum.setZero();
	}
};

Weight WFST::trainFinish(Weight converge_arc_delta, Weight converge_perplexity_ratio, int maxTrainIter, FLOAT_TYPE learning_rate_growth_factor, NormalizeMethod method, int ran_restarts)
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



  if ( trn->totalEmpiricalWeight > 0 ) {
    EACHDW(dw->prior_counts *= trn->totalEmpiricalWeight;);
  }

  Weight bestPerplexity;
  bestPerplexity.setInfinity();
bool very_first_time=true;
while(1) { // random restarts
	int train_iter = 0;
  Weight lastChange;
  Weight lastPerplexity;
  lastPerplexity.setInfinity();
  FLOAT_TYPE learning_rate=1;
  bool first_time=true;
  bool last_was_reset=false;
  for ( ; ; ) {
    ++train_iter;
    if ( train_iter > maxTrainIter ) {
      Config::log()  << "Maximum number of iterations (" << maxTrainIter << ") reached before convergence criteria was met - greatest arc weight change was " << lastChange << "\n";
      break;
    }
#ifdef DEBUGTRAIN
    Config::debug() << "Starting iteration: " << train_iter << '\n';
#endif
//#ifdef DEBUG
#define DWSTAT(a) \
	do {Config::debug() << a; \
		    WeightAccum a_w;\
				 WeightAccum a_c;\
				 EACHDW(\
					a_w(&(dw->weight()));\
				  a_c(&(dw->counts));\
					);\
					Config::debug() << "(sum,n,nonzero): weights=("<<a_w.sum<<","<<a_w.n<<","<<a_w.n_nonzero<<")" << " counts=("<<a_c.sum<<","<<a_c.n<<","<<a_c.n_nonzero<<")\n";\
				} while(0)
//#else
//#define DWSTAT
//#endif

		DWSTAT("Before estimate");
    Weight newPerplexity = train_estimate(); //lastPerplexity.isInfinity() // only delete no-path training the first time, in case we screw up with our learning rate
		DWSTAT("After estimate");
    Config::log() << "i=" << train_iter << " (rate=" << learning_rate << "): model-perplexity=" << newPerplexity;
    if ( newPerplexity < bestPerplexity ) {
      Config::log() << " (new best)";
      bestPerplexity=newPerplexity;
      EACHDW(dw->best_weight=dw->weight(););
    }

    Weight pp_ratio_scaled;
    if ( first_time ) {
      Config::log() << std::endl;
      first_time = false;
      pp_ratio_scaled.setZero();
    } else {
      Weight pp_ratio=newPerplexity/lastPerplexity;
      pp_ratio_scaled = root(pp_ratio,newPerplexity.getLogImp()); // EM delta=(L'-L)/abs(L')      
      Config::log() << " (relative-perplexity-ratio=" << pp_ratio_scaled << "), max{d(weight)}=" << lastChange;
#ifdef DEBUG_ADAPTIVE_EM
      Config::log()  << " last-perplexity="<<lastPerplexity<<' ';
      if ( learning_rate > 1) {
        EACHDW(Weight t=dw->em_weight;dw->em_weight=dw->weight();dw->weight()=t;);        // swap EM/scaled
				Weight em_pp=train_estimate();

        Config::log() << "unscaled-EM-perplexity=" << em_pp;
        EACHDW(Weight t=dw->em_weight;dw->em_weight=dw->weight();dw->weight()=t;);        // swap back
        if (em_pp > lastPerplexity)
          Config::warn() << " - last EM worsened perplexity, from " << lastPerplexity << " to " << em_pp << ", which is theoretically impossible." << std::endl;
      }
#endif
      Config::log() << std::endl;

    }
    if (!last_was_reset) {
      if (  pp_ratio_scaled >= converge_perplexity_ratio ) {
        if ( learning_rate > 1 ) {
          Config::log() << "Failed to improve (relaxation rate too high); starting again at learning rate 1" << std::endl;
          learning_rate=1;
          EACHDW(dw->weight() = dw->em_weight;);
          last_was_reset=true;
          continue;
        }
        Config::log() << "Converged - per-example perplexity ratio exceeds " << converge_perplexity_ratio << " after " << train_iter << " iterations.\n";
        break;
      } else {
        if (learning_rate < MAX_LEARNING_RATE_EXP)
          learning_rate *= learning_rate_growth_factor;
      }
    } else
      last_was_reset=false;

		if (very_first_time) {
			train_prune();
			very_first_time=false;
		}
		DWSTAT("Before maximize");
    lastChange = train_maximize(method,learning_rate);
		DWSTAT("After maximize");
    if (lastChange <= converge_arc_delta ) {
      Config::log() << "Converged - maximum weight change less than " << converge_arc_delta << " after " << train_iter << " iterations.\n";
      break;
    }

    lastPerplexity=newPerplexity;
  }
	if (ran_restarts > 0) {
		--ran_restarts;
		randomSet();
		normalize(method);
		Config::log() << "\nRandom restart - " << ran_restarts << " remaining.\n";
	} else {
		break;
	}
}
  Config::log() << "Setting weights to model with lowest perplexity=" << bestPerplexity << std::endl;
  EACHDW(dw->weight()=dw->best_weight;);

	return bestPerplexity;
  delete[] trn->forArcs;
  delete[] trn->revArcs;
  delete trn->forETopo;
  delete trn->revETopo;

	
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
        if ( (pLDW = IOarcs[s].find_second(IO)) ){
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
          if ( (pLDW = IOarcs[s].find_second(IO)) ){
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
          if ( (pLDW = IOarcs[s].find_second(IO)) ){
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
            if ( (pLDW = IOarcs[s].find_second(IO)) ){
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
          if ( (pLDW = IOarcs[s].find_second(IO)) ){
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
  // since the backward paths were obtained on the reversed input/output, reverse them back
  reverseMatrix(w,nIn,nOut);
#ifdef DEBUGTRAINDETAIL // Yaser 7-20-2000
  Config::debug() << "\nForwardProb/BackwardProb:\n";
  for (int i = 0 ;i<= nIn ; ++i){
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
  EACHDW(
         dw->counts.setZero();
         );

  List<IOSymSeq>::erase_iterator seq=trn->examples.erase_begin(),lastExample=trn->examples.erase_end();
#ifdef DEBUG
	EACHDW(NANCHECK(dw->counts););

#endif
  //#ifdef DEBUGTRAIN
  int train_example_no = 0 ; // Yaser 7-13-2000
  //#endif

#ifdef DEBUG_ESTIMATE_PP
  Config::debug() << " Exampleprobs:";
#endif

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
#ifdef DEBUG_ESTIMATE_PP
    Config::debug() << ',' << fin;
#endif
    prodModProb *= (fin^seq->weight); // since perplexity = 2^(- avg log likelihood)=2^((-1/n)*sum(log2 prob)) = (2^sum(log2 prob))^(-1/n) , we can take prod(prob)^(1/n) instead; prod(prob) = prodModProb, of course.  raising ^N does the multiplication N times for an example that is weighted N

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

    EACHDW(
           dw->scratch.setZero();
           );

    // accumulate counts for each arc's contribution throughout all uses it has in explaining the training
    for ( i = 0 ; i <= nIn ; ++i ) // go over all symbols in input in the training pair
      for ( o = 0 ; o <= nOut ; ++o ) // go over all symbols in the output pair
        for ( s = 0 ; s < numStates() ; ++s ) {
          IOarcs = trn->forArcs + s;
          if ( i < nIn ) { // input is not epsilon
            io.in = letIn[i];
            if ( o < nOut ) { // output is also not epsilon
              io.out = letOut[o];
              if ( (pLDW = IOarcs->find_second(io)) ){
                for ( List<DWPair>::val_iterator dw=pLDW->val_begin(),end = pLDW->val_end() ; dw !=end ; ++dw )
                  dw->scratch += f[i][o][s] * dw->weight() * b[i+1][o+1][dw->dest];
              }
            }
            io.out = epsilon_index; // output is epsilon, input is not
            if ( (pLDW = IOarcs->find_second(io)) ){
              for ( List<DWPair>::val_iterator dw=pLDW->val_begin(),end = pLDW->val_end() ; dw !=end ; ++dw )
                dw->scratch += f[i][o][s] * dw->weight() * b[i+1][o][dw->dest];
            }
          }
          io.in = epsilon_index; // input is epsilon
          if ( o < nOut ) { // input is epsilon, output is not
            io.out = letOut[o];
            if ( (pLDW = IOarcs->find_second(io)) ){
              for ( List<DWPair>::val_iterator dw=pLDW->val_begin(),end = pLDW->val_end() ; dw !=end ; ++dw )
                dw->scratch += f[i][o][s] * dw->weight() * b[i][o+1][dw->dest];
            }
          }
          io.out = epsilon_index; // input and output are both epsilon
          if ( (pLDW = IOarcs->find_second(io)) ){
            for ( List<DWPair>::val_iterator dw=pLDW->val_begin(),end = pLDW->val_end() ; dw !=end ; ++dw )
              dw->scratch += f[i][o][s] * dw->weight() * b[i][o][dw->dest];
          }
        }
    EACHDW(
           if (!dw->scratch.isZero())
           dw->counts += (dw->scratch / fin) * (Weight)seq->weight;
           );

    ++seq;
#ifdef DEBUG
		EACHDW(NANCHECK(dw->counts);NANCHECK(dw->scratch););
	
#endif

  } // end of while(training examples)

  return root(prodModProb,trn->totalEmpiricalWeight).invert(); // ,trn->totalEmpiricalWeight); // return model perplexity = 1/Nth root of 2^entropy

}

void WFST::train_prune() {
			Assert(trn);
			/*
			int n_states=numStates();
			bool *dead_states=NEW bool[n_states]; // blah: won't really work unless we also delete stuff from trn, so postponing
			for (int i=0;i<n_states;++i) {
				Weight sum=0;
				for ( HashIter<IOPair, List<DWPair> > ha(trn->forArcs[s]) ; ha ; ++ha ){ \
        List<DWPair>::val_iterator end = ha.val().val_end() ; \
                for ( List<DWPair>::val_iterator dw=ha.val().val_begin() ; dw !=end ; ++dw ) {\
				dead_states[i]=false;
				dead_states[i]=true;

			}
			delete[] dead_states;
			*/

}

Weight WFST::train_maximize(WFST::NormalizeMethod method,FLOAT_TYPE delta_scale)
{
  Assert(trn);

#ifdef DEBUGTRAINDETAIL
#define DUMPDW  do { for ( int s = 0 ; s < numStates() ; ++s ) \
  for ( HashTable<IOPair, List<DWPair> >::const_iterator ha(trn->forArcs[s]) ; ha ; ++ha ){ \
      List<DWPair>::const_iterator end = ha.val().const_end() ; \
      for ( List<DWPair>::const_iterator dw=ha.val().const_begin() ; dw !=end; ++dw ){ \
        if ( isTiedOrLocked(pGroup = (dw->arc)->groupId) ) \
          Config::debug() << pGroup << ' ' ; \
        Config::debug() << s << "->" << *dw->arc <<  " weight " << dw->weight() << " scratch: "<< dw->scratch  <<" counts " <<dw->counts  << '\n'; \
      } \
        } } while(0)
int pGroup;
	Config::debug() << "\nWeights before prior smoothing\n";
  DUMPDW;
#endif
  EACHDW (
          if ( !isLocked((dw->arc)->groupId) ) { // if the group is tied, and the group number is zero, then the old weight does not change. Otherwise update as follows
            //Weight &w=dw->weight();
            dw->scratch = dw->weight();   // old weight - Yaser: this is needed only to calculate change in weight later on ..
            //Weight &counts = dw->counts;
						NANCHECK(dw->counts);
						NANCHECK(dw->prior_counts);
            dw->weight() = dw->counts + dw->prior_counts; // new (unnormalized weight)
						NANCHECK(dw->weight());
						NANCHECK(dw->scratch);
          }
          );
#ifdef DEBUGTRAINDETAIL
  Config::debug() << "\nWeights before normalization\n";
  DUMPDW;
#endif
	DWSTAT("Before normalize");
  normalize(method);
	DWSTAT("After normalize");
#ifdef DEBUG_ADAPTIVE_EM
  //normalize(method);
#endif
#ifdef DEBUGTRAINDETAIL
  Config::debug() << "\nWeights after normalization\n";
  DUMPDW;
#endif

  // overrelax weight() and store raw EM weight in em_weight
  EACHDW(
         DWPair *d=&*dw;
         d->em_weight = d->weight();
				 NANCHECK(d->em_weight); 
         if (delta_scale > 1.)
         if ( !isLocked((d->arc)->groupId) )
					 if ( d->scratch.isPositive() ) {
						d->weight() = d->scratch * ((d->em_weight / d->scratch) ^ delta_scale);
						NANCHECK(d->scratch);	NANCHECK(d->weight());

					 }
         );

  Weight change, maxChange;

  // find maximum change for convergence
  //maxChange.setZero(); // default constructor

  if (delta_scale > 1.)
    normalize(method);

  EACHDW(
         if (!isLocked((dw->arc)->groupId)) {
           change = absdiff(dw->weight(),dw->scratch);           
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
  int *inLet = (nIn > 0 ? NEW int[nIn] : NULL);
  int *outLet = (nOut > 0 ? NEW int[nOut] : NULL);
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
      DW.arc = &*a;
      if ( !(pLDW = IOarcs[s].find_second(IO)) )
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
  HashTable<IOPair,List<DWPair> >::const_iterator i=h.begin();
  if ( i==h.end() ) return o;
goto FIRST;
  while ( ++i,i!=h.end() ) {
	o << ' ';
FIRST:
    o << '(' << i->first << ' ' << i->second << ')';
  }
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

