#ifndef _EM_HPP
#define _EM_HPP

#include "config.h"
#include "weight.h"

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

/// overrelaxed EM with random restarts.
/// RETURN: best perplexity
//  YOU SUPPLY:
//            perplexity = estimate();
//            lastChange = maximize(learning_rate); // returns maximum absolute change to any parameter
//            normalize();
//    converge perplexity ratio ... 1 = total convergence before stopping, .999 = near exact
//    converge param delta ... 1 = converge if all changes less than 1, 0 = never stop as long as perplexity keeps improving
//    learning_rate_growth_factor ... exponent for overrelaxed EM.  1 = normal EM, 1.1 = reasonable guess (what rate helps speed convergence depends on problem).  if rate/exponent gets too high and perplexity goes down, it's reset to the base guaranteed-to-improve-or-converge EM, similar to TCP slow restart on congestion
//    ran_restarts = how many times to randomly initialize parameters (after doing one iteration with supplied parameters), keeping the best PP params of all runs
//FIXME: EACHDW visitor or iterator?
template <class Estimate,class Maximize,class Normalize>
Weight overrelaxed_em(Estimate &estimate,Maximize &maximize, Normalize &normalize,Weight converge_perplexity_ratio=.999,Weight converge_param_delta=0,int max_iter=10000, FLOAT_TYPE learning_rate_growth_factor=1, int ran_restarts=0,ostream &log=Config::log())
{
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
            if ( train_iter > max_iter ) {
                log  << "Maximum number of iterations (" << max_iter << ") reached before convergence criteria was met - greatest param weight change was " << lastChange << "\n";
                break;
            }
#ifdef DEBUGTRAIN
            log << "Starting iteration: " << train_iter << '\n';
#endif
//#ifdef DEBUG
#define DWSTAT(a)                                                       \
            do {log << a;                                   \
                WeightAccum a_w;                                        \
                WeightAccum a_c;                                        \
                EACHDW(                                                 \
                    a_w(&(dw->weight()));                               \
                    a_c(&(dw->counts));                                 \
                    );                                                  \
                log << "(sum,n,nonzero): weights=("<<a_w.sum<<","<<a_w.n<<","<<a_w.n_nonzero<<")" << " counts=("<<a_c.sum<<","<<a_c.n<<","<<a_c.n_nonzero<<")\n"; \
            } while(0)
//#else
//#define DWSTAT
//#endif

            DWSTAT("Before estimate");
            Weight newPerplexity = train_estimate(); //lastPerplexity.isInfinity() // only delete no-path training the first time, in case we screw up with our learning rate
            DWSTAT("After estimate");
            log << "i=" << train_iter << " (rate=" << learning_rate << "): model-perplexity=" << newPerplexity;
            if ( newPerplexity < bestPerplexity ) {
                log << " (new best)";
                bestPerplexity=newPerplexity;
                EACHDW(dw->best_weight=dw->weight(););
            }

            Weight pp_ratio_scaled;
            if ( first_time ) {
                log << std::endl;
                first_time = false;
                pp_ratio_scaled.setZero();
            } else {
                Weight pp_ratio=newPerplexity/lastPerplexity;
                pp_ratio_scaled = root(pp_ratio,newPerplexity.getLogImp()); // EM delta=(L'-L)/abs(L')
                log << " (relative-perplexity-ratio=" << pp_ratio_scaled << "), max{d(weight)}=" << lastChange;
#ifdef DEBUG_ADAPTIVE_EM
                log  << " last-perplexity="<<lastPerplexity<<' ';
                if ( learning_rate > 1) {
                    EACHDW(Weight t=dw->em_weight;dw->em_weight=dw->weight();dw->weight()=t;);        // swap EM/scaled
                    Weight em_pp=train_estimate();

                    log << "unscaled-EM-perplexity=" << em_pp;
                    EACHDW(Weight t=dw->em_weight;dw->em_weight=dw->weight();dw->weight()=t;);        // swap back
                    if (em_pp > lastPerplexity)
                        log << " - last EM worsened perplexity, from " << lastPerplexity << " to " << em_pp << ", which is theoretically impossible." << std::endl;
                }
#endif
                log << std::endl;

            }
            if (!last_was_reset) {
                if (  pp_ratio_scaled >= converge_perplexity_ratio ) {
                    if ( learning_rate > 1 ) {
                        log << "Failed to improve (relaxation rate too high); starting again at learning rate 1" << std::endl;
                        learning_rate=1;
                        EACHDW(dw->weight() = dw->em_weight;);
                        last_was_reset=true;
                        continue;
                    }
                    log << "Converged - per-example perplexity ratio exceeds " << converge_perplexity_ratio << " after " << train_iter << " iterations.\n";
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
//                        normalize(method);

            lastChange = maximize(learning_rate);
            DWSTAT("After maximize");
            if (lastChange <= converge_param_delta ) {
                log << "Converged - maximum weight change less than " << converge_param_delta << " after " << train_iter << " iterations.\n";
                break;
            }

            lastPerplexity=newPerplexity;
        }
        if (ran_restarts > 0) {
            --ran_restarts;
            randomSet();
            normalize(method);
            log << "\nRandom restart - " << ran_restarts << " remaining.\n";
        } else {
            break;
        }
    }
    log << "Setting weights to model with lowest perplexity=" << bestPerplexity << std::endl;
    EACHDW(dw->weight()=dw->best_weight;);

    return bestPerplexity;
}

#if 0

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
                                                NANCHECK(d->scratch);   NANCHECK(d->weight());

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
              if ( (pLDW = find_second(*IOarcs,io)) ){
                for ( List<DWPair>::val_iterator dw=pLDW->val_begin(),end = pLDW->val_end() ; dw !=end ; ++dw )
                  dw->scratch += f[i][o][s] * dw->weight() * b[i+1][o+1][dw->dest];
              }
            }
            io.out = epsilon_index; // output is epsilon, input is not
            if ( (pLDW = find_second(*IOarcs,io)) ){
              for ( List<DWPair>::val_iterator dw=pLDW->val_begin(),end = pLDW->val_end() ; dw !=end ; ++dw )
                dw->scratch += f[i][o][s] * dw->weight() * b[i+1][o][dw->dest];
            }
          }
          io.in = epsilon_index; // input is epsilon
          if ( o < nOut ) { // input is epsilon, output is not
            io.out = letOut[o];
            if ( (pLDW = find_second(*IOarcs,io)) ){
              for ( List<DWPair>::val_iterator dw=pLDW->val_begin(),end = pLDW->val_end() ; dw !=end ; ++dw )
                dw->scratch += f[i][o][s] * dw->weight() * b[i][o+1][dw->dest];
            }
          }
          io.out = epsilon_index; // input and output are both epsilon
          if ( (pLDW = find_second(*IOarcs,io)) ){
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

#endif

#ifdef TEST
#include "test.hpp"

BOOST_AUTO_UNIT_TEST( TEST_EM )
{
}
#endif

#endif
