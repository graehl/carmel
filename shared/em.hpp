#ifndef _EM_HPP
#define _EM_HPP

#include "config.h"
#include "weight.h"
#include "debugprint.hpp"
#include <cmath>

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


/*struct ParamDelta : public std::pair<double,unsigned> {
  typedef std::pair<double,unsigned> P;
    ParamDelta() {}
    ParamDelta(double d,unsigned i) : P(d,i) {}
    ParamDelta(const P &o) : P(o) {}
};
*/
typedef std::pair<double,unsigned> ParamDelta;

ostream & operator << (ostream &out,const ParamDelta &p) {
    if (p.first > 0)
        return out << "delta_weight["<<p.second<<"]="<<p.first;
    else
        return out << "unchanged";

}

/// overrelaxed EM with random restarts.
/// RETURN: best perplexity
//  YOU SUPPLY (something that implements/extends):
struct EM_executor {
    // define =1 if you do anything with learning_rate
//    enum {LEARNING_RATE=0};
     // assigns random parameter values - not called unless random_restarts > 0
    void randomize() {}
     // returns (weighted) average log prob over all examples given current parameters, and collects counts (performs count initialization itself).  first_time flag intended to allow you to drop examples that have 0 probability (training can't continue if they're kept)
    double estimate(bool first_time) {return 0;}
    // assigns new parameters from counts collected by estimate; learning_rate may be ignored, but is intended to magnify the delta from the previous parameter set to the normalized new parameter set.  should return largest absolute change to any parameter, and the index of that parameter.  should also save the un-magnified (raw normalized counts) version for undo_maximize (if you only use learning_rate==1, then you don't need to do anything but normalize)
    ParamDelta maximize(double learning_rate) {return ParamDelta(0,0);}
    // for overrelaxed EM: if probability gets worse, reset learning rate to 1 and use the last improved weights.  or, you may wish to save a copy of the learning-rate-1 new weights that you extrapolate the overrelaxed ones from (compared to their previous value), and simply restore those, rather than backing off to the previous iteration and wasting another estimate.
    void undo_maximize() {}

     // if you're doing random restarts, transfer the current parameters to safekeeping
    void save_best() {}
    void restore_best() {}
};


// note that your initial parameters will be left alone for the first iteration (this means you should initialize them to something that gives nonzero probs

//    converge relative logprobability epsilon ... 0 = total convergence before stopping, .0001 = near exact
//    converge param delta ... 1 = converge if all changes less than 1, 0 = never stop as long as perplexity keeps improving
//    learning_rate_growth_factor ... exponent for overrelaxed EM.  1 = normal EM, 1.1 = reasonable guess (what rate helps speed convergence depends on problem).  if rate/exponent gets too high and perplexity goes down, it's reset to the base guaranteed-to-improve-or-converge EM, similar to TCP slow restart on congestion
//    ran_restarts = how many times to randomly initialize parameters (after doing one iteration with supplied parameters), keeping the best PP params of all runs
// return best (greatest) average log prob
template <class Exec>
double overrelaxed_em(Exec &exec,int max_iter=10000,double converge_relative_avg_logprob_epsilon=.0001,int ran_restarts=0,double converge_param_delta=0, double learning_rate_growth_factor=1, ostream &logs=Config::log(),unsigned log_level=1)
{
    double best_alp=-HUGE_VAL; // alp=average log prob = (logprob1 +...+ logprobn )/ n (negative means 0 probability, 0 = 1 probability)
    double &rel_eps=converge_relative_avg_logprob_epsilon;
    bool very_first_time=true;
    while(1) { // random restarts
        int train_iter = 0;
        ParamDelta max_delta_param;
        double last_alp;

        last_alp=-HUGE_VAL;
        double learning_rate=1;
        bool first_time=true;
        bool last_was_reset=false;
//        exec.maximize(1); // may not be desireable if you wanted just 1 iteration to compute counts = inside*outside but you should do that outside this framework
        for ( ; ; ) {
            ++train_iter;
            DBP(train_iter);DBPSCOPE;
            if ( train_iter > max_iter ) {
                logs << "Maximum number of iterations (" << max_iter << ") reached before convergence criteria was met - greatest param weight change was " << max_delta_param << "\n";
                break;
            }
//            if (log_level > 0) logs << "Starting iteration: " << train_iter << '\n';

            double new_alp = exec.estimate(very_first_time);

            logs << "i=" << train_iter << " (rate=" << learning_rate << "): model-avg-log-prob=" << new_alp;

            //FIXME: don't really need to do this so often, can move outside of for loop even ... but for sanity's sake (not much efficiency difference) leave it here
            if ( new_alp > best_alp || very_first_time ) {
                logs << " (new best)";
                best_alp=new_alp;
                exec.save_best();
            }

            if (very_first_time)
                very_first_time=false;

            double dpp=new_alp-last_alp; // should be increasing, so diff is positive
            double rel_dpp = dpp/fabs(last_alp); // COULD BE FASTER (last_alp always negative) but whatever :)
            if ( first_time ) {
                rel_dpp=HUGE_VAL;
                logs << std::endl;
                first_time = false;
//                pp_ratio_scaled.setZero();
            } else {
//                Weight pp_ratio=new_alp/last_alp;
//                pp_ratio_scaled = root(pp_ratio,new_alp.getLogImp()); // EM delta=(L'-L)/abs(L')
                logs << " (relative-d-avg-logprob=" << rel_dpp << "), max " << max_delta_param << std::endl;
            }
            if (!last_was_reset) {
                if ( rel_dpp < rel_eps ) {
                    if ( learning_rate > 1 ) {
                        logs << "Failed to improve (relaxation rate too high); starting again at learning rate 1" << std::endl;
                        learning_rate=1;
                        exec.undo_maximize(); // this is needed because you've just measured alp from corrupted (overscaled) parameters and collected corrupt counts
                        last_was_reset=true;
                        continue;
                    }
                    logs << "Converged - relative per-example avg-logprob change less than " << rel_eps << " after " << train_iter << " iterations.\n";
                    break;
                } else {
                    if (learning_rate < MAX_LEARNING_RATE_EXP)
                        learning_rate *= learning_rate_growth_factor;
                }
            } else
                last_was_reset=false;

            max_delta_param = exec.maximize(learning_rate);
            if (max_delta_param.first < converge_param_delta ) {
                logs << "Converged - all weights changed less than " << converge_param_delta << " after " << train_iter << " iterations.\n";
                break;
            }

            last_alp=new_alp;
        } // for
        if (ran_restarts > 0) {
            --ran_restarts;
            exec.randomize();
            logs << "\nRandom restart - " << ran_restarts << " remaining.\n";
        } else {
            break;
        }
    }
    logs << "Setting weights to model with lowest avg-prob=" << best_alp << std::endl;
    exec.restore_best();

    return best_alp;
}


#ifdef TEST
#include "test.hpp"

BOOST_AUTO_UNIT_TEST( TEST_EM )
{
}
#endif

#endif
