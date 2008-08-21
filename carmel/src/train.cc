#include <boost/utility.hpp>
#include <graehl/shared/config.h>
#include <graehl/carmel/src/train.h>
#include <graehl/carmel/src/fst.h>
#include <graehl/shared/weight.h>
#include <graehl/carmel/src/derivations.h>

//#define DEBUGTRAIN

namespace graehl {

derivations::statistics derivations::global_stats;

void check_fb_agree(Weight fin,Weight fin2)
{
#ifdef DEBUGTRAIN
    Config::debug()<<"Forward prob = " << fin << std::endl;
    Config::debug()<<"Backward prob = " << fin2 << std::endl;
#endif
#ifdef ALLOWED_FORWARD_OVER_BACKWARD_EPSILON        
    double ratio = (fin > fin2 ? fin/fin2 : fin2/fin).getReal();
    double e = ratio - 1;
    if ( e > ALLOWED_FORWARD_OVER_BACKWARD_EPSILON )
        Config::warn() << "Warning: forward prob vs backward prob relative difference of " << e << " exceeded " << ALLOWED_FORWARD_OVER_BACKWARD_EPSILON << " (with infinite precision, it should be 0).\n";
#endif
}

struct WeightAccum {
    Weight sum;
    int n_nonzero;
    int n;
    void operator ()(Weight const&w) {
        w.NaNCheck();
        sum += w;
        ++n;
        if (w.isPositive())
            ++n_nonzero;
    }
    WeightAccum() {
        reset();
    }
    void reset() {
        n=n_nonzero=0;
        sum.setZero();
    }
    friend std::ostream & operator <<(std::ostream &o,WeightAccum const& a) 
    {
        return o << "("<<a.sum<<","<<a.n<<","<<a.n_nonzero<<")";
    }    
};

void print_stats(arcs_table const& t,char const* header) 
{
    
    Config::debug() << header;
    WeightAccum a_w;
    WeightAccum a_c;
    for (arcs_table::const_iterator i=t.begin(),e=t.end();i!=e;++i) {
        a_w(i->weight());
        a_c(i->counts);
    }
    Config::debug() << "(sum,n,nonzero): weights="<<a_w<<" counts="<<a_c<<"\n";
}

struct matrix_io_index : boost::noncopyable
{
    typedef dynamic_array<DWPair> for_io;
    typedef HashTable<IOPair,for_io> for_state;
    typedef fixed_array<for_state> states_t;

    arcs_table &t;
    states_t forward,backward;

    matrix_io_index(arcs_table &t) : t(t), forward(t.n_states), backward(t.n_states)
    {
    }

    void populate(bool include_backward) 
    {
        for (unsigned i=0,N=t.size();i!=N;++i) {
            arc_counts const& ac=t[i];
            IOPair io(ac.in(),ac.out());
            unsigned s=ac.src,d=ac.dest();
            forward[s][io].push_back(DWPair(d,i));
            if (include_backward)
                backward[d][io].push_back(DWPair(s,i));
        }
    }
};
    

// similar to transposition but not quite: instead of replacing w.ij with w.ji, replace w.ij with w.(I-i)(J-j) ... matrix has the same dimensions.  it's a 180 degree rotation, not a reflection about the identity line
void matrix_reverse_io(Weight ***w,int max_i, int max_o) {
    int i;
    for ( i = 0 ; i <= max_i/2 ; ++i ) {
        Weight **temp = w[i];
        w[i] = w[max_i - i];
        w[max_i - i] = temp;
    }
    for ( i = 0 ; i <= max_i ; ++i )
        for ( int o = 0 ; o <= max_o/2 ; ++o ) {
            Weight *temp = w[i][o];
            w[i][o] = w[i][max_o - o];
            w[i][max_o - o] = temp;
        }
}

struct forward_backward 
{
    WFST &x;
    unsigned n_in,n_out,n_st;
    typedef arcs_table arcs_t;
    training_corpus *trn;
    
    arcs_t arcs;

    /// stuff for old matrix (non derivation-structure-caching) based forward/backward
    bool use_matrix;
    bool remove_bad_training;    
    matrix_io_index mio;
    Weight ***f,***b;
    List<int> e_forward_topo,e_backward_topo; // epsilon edges that don't make cycles are handled by propogating forward/backward in these orders (state = int because of graph.h)    

    /// stuff for new derivation caching EM:
    typedef List<derivations> cached_derivs_t;
    cached_derivs_t cached_derivs;

    
    inline void matrix_compute(IOSymSeq const& s,bool backward=false) 
    {
        if (backward) {
            matrix_compute(s.i.n,s.i.rLet,s.o.n,s.o.rLet,x.final,b,mio.backward,e_backward_topo);
            // since the backward paths were obtained on the reversed input/output, reverse them back
            matrix_reverse_io(b,s.i.n,s.o.n);
        } else
            matrix_compute(s.i.n,s.i.let,s.o.n,s.o.let,0,f,mio.forward,e_forward_topo);
    }
    
    void matrix_compute(int nIn,int *inLet,int nOut,int *outLet,int start,Weight ***w,matrix_io_index::states_t &io,List<int> const& eTopo);
    
    inline void matrix_forward_prop(Weight ***m,matrix_io_index::for_io const* fio,unsigned s,unsigned i,unsigned o,unsigned d_i,unsigned d_o) 
    {
        if (!fio) return;
        for (matrix_io_index::for_io::const_iterator dw=fio->begin(),e=fio->end();dw!=e;++dw) {
            arc_counts &a=arcs[dw->id];
            unsigned d=dw->dest;
            assert(a.dest()==d||a.src==d); // first: forward, second: reverse
            Weight &to=m[i+d_i][o+d_o][d];
            Weight from=m[i][o][s];
            Weight w=a.weight();
#ifdef DEBUGFB
            Config::debug() << "w["<<i+d_i<<"]["<<o+d_o<<"]["<<d<<"] += " <<  "w["<<i<<"]["<<o<<"]["<<s<<"] * weight("<< *dw<<") ="<< to <<" + " << from <<" * "<< w <<" = "<< to <<" + " << from*w <<" = "<< to+(from*w)<<"\n";
#endif 
            to += from * w;            
        }
    }
    
    // accumulate counts for this example into scratch (so they can be weighted later all at once.  saves a few mults to weighting as you go?)
    inline void matrix_count(matrix_io_index::for_io const* fio,unsigned s,unsigned i,unsigned o,unsigned d_i,unsigned d_o)
    {
        if (!fio) return;
        for (matrix_io_index::for_io::const_iterator dw=fio->begin(),e=fio->end();dw!=e;++dw) {
            arc_counts &a=arcs[dw->id];
            assert(a.dest()==dw->dest);
            a.scratch += f[i][o][s] *a.weight() * b[i+d_i][o+d_o][dw->dest];
        }
    }
    
    template <class Examples>
    void compute_derivations(Examples const &ex) 
    {
        wfst_io_index io(arcs);
        unsigned n=1;
        for (typename Examples::const_iterator i=ex.begin(),end=ex.end();
             i!=end ; ++i,++n) {
            cached_derivs.push_front(x,i->i,i->o,i->weight,n,cache_backward);
            derivations &d=cached_derivs.front();
            if (!d.compute(io)) {
                warn_no_derivations(x,*i,n);
                cached_derivs.pop();
            } else {
#ifdef DEBUGDERIVATIONS
                Config::debug() << "Derivations in transducer for input/output #"<<n<<" (final="<<d.final()<<"):\n";
                i->print(Config::debug(),x,"\n");
                printGraph(d.graph(),Config::debug());
#endif 
            }
        }
        Config::log() << derivations::global_stats;
    }


            
    //     newPerplexity = train_estimate();
    //	lastChange = train_maximize(method);
    //    Weight train_estimate(Weight &unweighted_corpus_prob,bool remove_bad_training=true); // accumulates counts, returns perplexity of training set = 2^(- avg log likelihood) = 1/(Nth root of product of model probabilities of N-weight training examples)  - optionally deletes training examples that have no accepting path
    //    Weight train_maximize(NormalizeMethod const& method,FLOAT_TYPE delta_scale=1); // normalize then exaggerate (then normalize again), returning maximum change

// return per-example perplexity = 2^entropy=p(corpus)^(-1/N)
// unweighted_corpus_prob: ignore per-example weight, product over corpus of p(example)
    Weight estimate(Weight &unweighted_corpus_prob);

 private:
    // these take an initialize unweighted_corpus_prob and counts, and accumulate over the training corpus
    Weight estimate_cached(Weight &unweighted_corpus_prob_accum);
    Weight estimate_matrix(Weight &unweighted_corpus_prob_accum);

    // uses derivs.weight to scale counts for that example.  returns unweighted prob, however
    Weight estimate_cached(derivations & derivs);
 public:

    // return max change
    Weight maximize(WFST::NormalizeMethod const& method,FLOAT_TYPE delta_scale);

    void matrix_fb(IOSymSeq &s);

    
    void e_topo_populate(bool include_backward) 
    {
        assert(use_matrix);
        e_forward_topo.clear();
        e_backward_topo.clear();
        {
            Graph eGraph = x.makeEGraph();
            TopoSort t(eGraph,&e_forward_topo);
            t.order_crucial();
            int b = t.get_n_back_edges();
            if ( b > 0 )
                Config::warn() << "Warning: empty-label subgraph has " << b << " cycles!  Training may not propogate counts properly" << std::endl;
            if (include_backward) {
                Graph revEGraph = reverseGraph(eGraph);
                TopoSort t(revEGraph,&e_backward_topo);
                t.order_crucial();
                freeGraph(revEGraph);
            }
            freeGraph(eGraph);
        }
    }

    bool cache_backward;
    
    forward_backward(WFST &x,bool per_arc_prior,Weight global_prior,unsigned cache_derivations_level,bool include_backward=true)
        : x(x),arcs(x,per_arc_prior,global_prior),mio(arcs)
    {
        trn=NULL;
        f=b=NULL;
        remove_bad_training=true;
        if (cache_derivations_level!=WFST::cache_nothing) {
            use_matrix=false;
            cache_backward=(cache_derivations_level==WFST::cache_forward_backward);
        } else {            
            use_matrix=true;
            mio.populate(include_backward);
            e_topo_populate(include_backward);
        }        
        n_st=x.numStates();
    }

    void matrix_dump(unsigned m_i,unsigned m_o) 
    {
        assert (use_matrix && f && b);
        Config::debug() << "\nForwardProb/BackwardProb:\n";
        for (int i = 0 ;i<=m_i ; ++i){
            for (int o = 0 ; o <=m_o ; ++o){
                Config::debug() << i << ':' << o << " (" ;
                for (int s = 0 ; s < n_st ; ++s){
                    Config::debug() << f[i][o][s] <<'/'<<b[i][o][s];
                    if (s < n_st-1)
                        Config::debug() << ' ' ;
                }
                Config::debug() <<')'<<std::endl;
                if(o < m_o)
                    Config::debug() <<' ' ;
            }
            Config::debug() <<std::endl;
        }
    }    

    training_corpus &corpus() const 
    {
        return *trn;
    }
    
    // call before using corpus
    void prepare(training_corpus & corpus,bool include_backward=true)  // nonconst ref because we may remove examples with no derivation
    {
        trn=&corpus;

        if (use_matrix) {    
            n_in=corpus.maxIn+1; // because position 0->1 is first symbol, there are n+1 boundary markers
            n_out=corpus.maxOut+1;
            f = NEW Weight **[n_in];
            if (include_backward)
                b = NEW Weight **[n_in];
            else
                b=NULL;
            for ( unsigned i = 0 ; i < n_in ; ++i ) {
                f[i] = NEW Weight *[n_out];
                if (b)
                    b[i] = NEW Weight *[n_out];
                for ( unsigned o = 0 ; o < n_out ; ++o ) {
                    f[i][o] = NEW Weight [n_st];
                    if (b)
                        b[i][o] = NEW Weight [n_st];
                }
            }
        } else {
            compute_derivations(corpus.examples);
        }
    }

    // call after done using f,b matrix for a corpus
    void cleanup() 
    {
        if (use_matrix && f) {
            for ( unsigned i = 0 ; i < n_in ; ++i ) {
                for ( unsigned o = 0 ; o < n_out ; ++o ) {
                    delete[] f[i][o];
                    if (b)
                        delete[] b[i][o];
                }
                delete[] f[i];
                if (b)
                    delete[] b[i];
            }
            delete[] f;
            if (b)
                delete[] b;   
        }
        f=b=NULL;
    }
    
    ~forward_backward() 
    {
        cleanup();
    }

    
    
};

namespace for_arcs {


// scratch gets previous weight
struct prep_new_weights
{
    Weight scale_prior;
    prep_new_weights(Weight scale_prior) : scale_prior(scale_prior) {}
    void operator()(arc_counts &a) const 
    {
        if ( !WFST::isLocked((a.arc)->groupId) ) { // if the group is tied, then the group number is zero, then the old weight does not change. Otherwise update as follows
            a.scratch = a.weight();   // old weight - Yaser: this is needed only to calculate change in weight later on ..
            a.weight() = a.counts + a.prior_counts*scale_prior; // new (unnormalized weight)
            NANCHECK(a.counts);
            NANCHECK(a.prior_counts);
            NANCHECK(a.weight());
            NANCHECK(a.scratch);
        }
    }
};

// overrelax weight() and store raw EM weight in em_weight.  after pre_norm_counts, scratch has old .  also after WFST::normalize.  POST: need normalization again.
struct overrelax
{
    FLOAT_TYPE delta_scale;
    overrelax(FLOAT_TYPE delta_scale) : delta_scale(delta_scale) {}
    void operator()(arc_counts &a) const
    {
        a.em_weight = a.weight();
        NANCHECK(a.em_weight);
        if (delta_scale > 1.)
            if ( !WFST::isLocked(a.groupId()) )
                if ( a.scratch.isPositive() ) {
                    a.weight() = a.scratch * ((a.em_weight / a.scratch).pow(delta_scale));
                    NANCHECK(a.scratch);
                    NANCHECK(a.weight());
                }
    }        
};

struct max_change
{
    Weight maxChange;
    void operator()(arc_counts &a) 
    {
        if (!WFST::isLocked(a.groupId())) {
            Weight change = absdiff(a.weight(),a.scratch);
            if ( change > maxChange )
                maxChange = change;
        }
    }
    Weight get() const 
    {
        return maxChange;
    }
};

struct save_best 
{
    void operator()(arc_counts &a) const
    {
        a.best_weight=a.weight();
    }
};

struct swap_em_scaled
{
    void operator()(arc_counts &a) const
    {
        std::swap(a.em_weight,a.weight());
    }
};  

struct keep_em_weight
{
    void operator()(arc_counts &a) const
    {
        a.weight()=a.em_weight;
    }
};

struct use_best_weight
{
    void operator()(arc_counts &a) const
    {
        a.weight()=a.best_weight;
    }
};
    

struct clear_count
{
    void operator()(arc_counts &a) const
    {
        a.counts.setZero();
    }
};

struct clear_scratch
{
    void operator()(arc_counts &a) const
    {
        a.scratch.setZero();
    }
};

struct add_weighted_scratch
{
    Weight w;
    add_weighted_scratch(Weight w):w(w){}
    void operator()(arc_counts &a) const
    {
        if (!a.scratch.isZero())
            a.counts += w*a.scratch;
#ifdef DEBUG
        NANCHECK(a.counts);
        NANCHECK(a.scratch);
#endif
    }
};

    

    
}//ns

Weight WFST::train(
                   training_corpus & corpus,NormalizeMethod const& method,bool weight_is_prior_count,
                   Weight smoothFloor,Weight converge_arc_delta, Weight converge_perplexity_ratio,
                   int maxTrainIter,FLOAT_TYPE learning_rate_growth_factor,
                   int ran_restarts,unsigned cache_derivations_level
                   )
{
    normalize(method);
    
    forward_backward fb(*this,weight_is_prior_count,smoothFloor,cache_derivations_level,true);
    fb.prepare(corpus,true);

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
            
#ifdef DEBUG
#define DWSTAT(a) print_stats(arcs,a)
            arcs_table const&arcs=fb.arcs;
#else
#define DWSTAT
#endif
            
            DWSTAT("Before estimate");
            Weight corpus_p;
            Weight newPerplexity = fb.estimate(corpus_p); //lastPerplexity.isInfinity() // only delete no-path training the first time, in case we screw up with our learning rate
            DWSTAT("After estimate");
            Config::log() << "i=" << train_iter << " (rate=" << learning_rate << "): ";
            Config::log() << " per-output-symbol-perplexity=";
            corpus_p.root(corpus.n_output).inverse().print_base(Config::log(),2);
            Config::log() << " per-example-perplexity=";
            newPerplexity.print_base(Config::log(),2);
            if ( newPerplexity < bestPerplexity ) {
                Config::log() << " (new best)";
                bestPerplexity=newPerplexity;
                fb.arcs.visit(for_arcs::save_best());
            }

            Weight pp_ratio_scaled;
            if ( first_time ) {
                Config::log() << std::endl;
                first_time = false;
                pp_ratio_scaled.setZero();
            } else {
                Weight pp_ratio=newPerplexity/lastPerplexity;
                pp_ratio_scaled = pp_ratio.root(std::fabs(newPerplexity.getLogImp())); // EM delta=(L'-L)/abs(L')
                Config::log() << " (relative-perplexity-ratio=" << pp_ratio_scaled << "), max{d(weight)}=" << lastChange;
#ifdef DEBUG_ADAPTIVE_EM
                Config::log()  << " last-perplexity="<<lastPerplexity<<' ';
                if ( learning_rate > 1) {
                    fb.arcs.visit(for_arcs::swap_em_scaled());

                    Weight d;
                    Weight em_pp=fb.estimate(d);

                    Config::log() << "unscaled-EM-perplexity=" << em_pp;
                    
                    fb.arcs.visit(for_arcs::swap_em_scaled());

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
                        fb.arcs.visit(for_arcs::keep_em_weight());

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
            lastChange = fb.maximize(method,learning_rate);
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
    Config::log() << "Setting weights to model with lowest per-example-perplexity ( = prod[modelprob(example)]^(-1/num_examples) = 2^(-log_2(p_model(corpus))/N) = ";
    bestPerplexity.print_base(Config::log(),2);

    Config::log() << std::endl;
    fb.arcs.visit(for_arcs::use_best_weight());

    //        for ( List<IOSymSeq>::val_iterator seq=trn->examples.val_begin(),end = trn->examples.val_end() ; seq !=end ; ++seq ) seq->kill();
    
    return bestPerplexity;
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

void forward_backward::matrix_compute(int nIn,int *inLet,int nOut,int *outLet,int start,Weight ***w,matrix_io_index::states_t &io,List<int> const& eTopo)
{
    
    int i, o, s;
    for ( i = 0 ; i <= nIn ; ++i )
        for ( o = 0 ; o <= nOut ; ++o )
            for ( s = 0 ; s < n_st ; ++s )
                w[i][o][s].setZero();

    w[0][0][start] = 1;

    IOPair IO;

    typedef matrix_io_index::for_io for_io;
    for_io *pLDW;

    for ( i = 0 ; i <= nIn ; ++i ) {
        for ( o = 0 ; o <= nOut ; ++o ) {
#ifdef DEBUGFB
            Config::debug() <<"("<<i<<","<<o<<")\n";
#endif
            IO.in = 0;
            IO.out = 0;
            for ( List<int>::const_iterator topI=eTopo.const_begin(),end=eTopo.const_end() ; topI != end; ++topI ) {
                s = *topI;
                matrix_io_index::for_state const& fs = io[s];
                matrix_forward_prop(w,find_second(fs,IO),s,i,o,0,0);
            }
            for ( s = 0 ; s < n_st; ++s ) {
                if ( w[i][o][s].isZero() )
                    continue;
                matrix_io_index::for_state const& fs = io[s];
                if ( o < nOut ) {
                    IO.in = 0;
                    IO.out = outLet[o];
                    matrix_forward_prop(w,find_second(fs,IO),s,i,o,0,1);
                    if ( i < nIn ) {
                        IO.in = inLet[i];
                        IO.out = outLet[o];
                        matrix_forward_prop(w,find_second(fs,IO),s,i,o,1,1);
                    }
                }
                if ( i < nIn ) {
                    IO.in = inLet[i];
                    IO.out = 0;
                    matrix_forward_prop(w,find_second(fs,IO),s,i,o,1,0);               
                }
            }
        }
    }
}


void forward_backward::matrix_fb(IOSymSeq &s)
{
#ifdef DEBUGFB
    Config::debug() << "training example: \n"<<s << "\nForward\n" ;
#endif
    matrix_compute(s,false);
#ifdef DEBUGFB
    Config::debug() << "\nBackward\n";
#endif
    matrix_compute(s,true);
    
#ifdef DEBUGTRAINDETAIL // Yaser 7-20-2000
    matrix_dump(s.i.n,s.o.n);
#endif
}

void training_progress(unsigned train_example_no) 
{
    const unsigned EXAMPLES_PER_DOT=10;
    const unsigned EXAMPLES_PER_NUMBER=(70*EXAMPLES_PER_DOT);
    if (train_example_no % EXAMPLES_PER_DOT == 0)
        Config::log() << '.' ;
    if (train_example_no % EXAMPLES_PER_NUMBER == 0)
        Config::debug() << train_example_no << '\n' ;    
}


void warn_no_derivations(WFST const& x,IOSymSeq const& s,unsigned n) 
{
                
    Config::warn() << "No derivations in transducer for input/output #"<<n<<":\n";
    s.print(Config::warn(),x,"\n");
}

Weight forward_backward::estimate(Weight &unweighted_corpus_prob)
{
    arcs.visit(for_arcs::clear_count());
    unweighted_corpus_prob=1;
    Weight p;
    if (use_matrix)
        p=estimate_matrix(unweighted_corpus_prob);
    else
        p=estimate_cached(unweighted_corpus_prob);
    return p.root(trn->totalEmpiricalWeight).inverse();
}

Weight forward_backward::estimate_cached(Weight &unweighted_corpus_prob_accum)
{
    assert(!use_matrix);
    Weight weighted_corpus_prob=1;
    unsigned n=0;
    for (cached_derivs_t::val_iterator i=cached_derivs.val_begin(),e=cached_derivs.val_end();i!=e;++i) {
        ++n;
        training_progress(n);
        Weight prob=estimate_cached(*i);
        unweighted_corpus_prob_accum *= prob;
        weighted_corpus_prob *= prob.pow(i->weight);
    }
    return weighted_corpus_prob;
}

Weight forward_backward::estimate_cached(derivations & derivs)
{
    return derivs.collect_counts(arcs);
}



Weight forward_backward::estimate_matrix(Weight &unweighted_corpus_prob)
{
    assert(use_matrix && b);
    int i, o, s, nIn, nOut, *letIn, *letOut;

    // for perplexity
    Weight ret = 1;

    
    typedef matrix_io_index::for_io for_io;
    for_io *pLDW;
    IOPair io;
    

    List<IOSymSeq>::erase_iterator seq=corpus().examples.erase_begin(),lastExample=corpus().examples.erase_end();
    //#ifdef DEBUGTRAIN
    int train_example_no = 0 ; // Yaser 7-13-2000
    //#endif

#ifdef DEBUG_ESTIMATE_PP
    Config::debug() << " Exampleprobs:";
#endif

    while (seq != lastExample) { // loop over all training examples

        //#ifdef DEBUGTRAIN // Yaser 13-7-2000 - Debugging messages ..
        ++train_example_no ;
        training_progress(train_example_no);
        //#endif
        nIn = seq->i.n;
        nOut = seq->o.n;
        matrix_fb(*seq);
        Weight fin = f[nIn][nOut][x.final];
#ifdef DEBUG_ESTIMATE_PP
        Config::debug() << ',' << fin;
#endif

        ret *= fin.pow(seq->weight); // since perplexity = 2^(- avg log likelihood)=2^((-1/n)*sum(log2 prob)) = (2^sum(log2 prob))^(-1/n) , we can take prod(prob)^(1/n) instead; prod(prob) = ret, of course.  raising ^N does the multiplication N times for an example that is weighted N
        unweighted_corpus_prob *= fin;


        if (remove_bad_training)
            if ( !(fin.isPositive()) ) {
                warn_no_derivations(x,*seq,train_example_no);
                seq=trn->examples.erase(seq);
                continue;
            }
        check_fb_agree(fin,b[0][0][0]);

        letIn = seq->i.let;
        letOut = seq->o.let;

    arcs.visit(for_arcs::clear_scratch());

        // accumulate counts for each arc's contribution throughout all uses it has in explaining the training
        for ( i = 0 ; i <= nIn ; ++i ) // go over all symbols in input in the training pair
            for ( o = 0 ; o <= nOut ; ++o ) // go over all symbols in the output pair
                for ( s = 0 ; s < n_st ; ++s ) {
                    matrix_io_index::for_state const& fs = mio.forward[s];
                    if ( i < nIn ) { // input is not epsilon
                        io.in = letIn[i];
                        if ( o < nOut ) { // output is also not epsilon
                            io.out = letOut[o];
                            matrix_count(find_second(fs,io),s,i,o,1,1);
                        }
                        io.out = 0; // output is epsilon, input is not
                        matrix_count(find_second(fs,io),s,i,o,1,0);
                    }
                    io.in = 0; // input is epsilon
                    if ( o < nOut ) { // input is epsilon, output is not
                        io.out = letOut[o];
                        matrix_count(find_second(fs,io),s,i,o,0,1);
                    }
                    io.out = 0; // input and output are both epsilon
                    matrix_count(find_second(fs,io),s,i,o,0,0);
                }

        arcs.visit(for_arcs::add_weighted_scratch(seq->weight/fin));
        //        Weight mult=seq->weight;
        //        EACHDW(if (!dw->scratch.isZero()) dw->counts += mult*(dw->scratch / fin););

        ++seq;
    } // end of while(training examples)

    return ret; // ,trn->totalEmpiricalWeight); // return per-example perplexity = 2^entropy=p(corpus)^(-1/N)
}

void WFST::train_prune() {
    /*
      int n_states=numStates();
      bool *dead_states=NEW bool[n_states]; // blah: won't really work unless we also delete stuff from trn, so postponing
      for (int i=0;i<n_states;++i) {
      Weight sum=0;
      for ( HashTable<IOPair, List<DWPair> >::iterator ha(trn->forArcs[s]) ; ha ; ++ha ){ \
      List<DWPair>::val_iterator end = ha.val().val_end() ; \
      for ( List<DWPair>::val_iterator dw=ha.val().val_begin() ; dw !=end ; ++dw ) {\
      dead_states[i]=false;
      dead_states[i]=true;

      }
      delete[] dead_states;
    */

}
std::ostream& operator << (std::ostream &o,arc_counts const& ac)
{
    int pGroup;
    if ( WFST::isTiedOrLocked(pGroup = ac.groupId()) )    
        o << pGroup << ' ' ;                                                                                                                                    \
    o<< ac.src << "->" << *ac.arc <<  " weight " << ac.weight() << " scratch: "<< ac.scratch  <<" counts " <<ac.counts  << '\n';
    return o;
}


Weight forward_backward::maximize(WFST::NormalizeMethod const& method,FLOAT_TYPE delta_scale)
{
    
#ifdef DEBUGTRAINDETAIL
#define DUMPDW(h) arcs.dump(Config::debug(),h)
#else
#define DUMPDW(h)
#endif
    
    DUMPDW("Weights before prior smoothing");

    //    arcs.pre_norm_counts(corpus.totalEmpiricalWeight);
    arcs.visit(for_arcs::prep_new_weights(corpus().totalEmpiricalWeight));
    
    DUMPDW("Weights before normalization");

    DWSTAT("Before normalize");
    x.normalize(method);
    DWSTAT("After normalize");

    DUMPDW("Weights after normalization");

    arcs.visit(for_arcs::overrelax(delta_scale));
    
    //    arcs.overrelax();

    // find maximum change for convergence

    if (delta_scale > 1.)
        x.normalize(method);
    
    //    return arcs.max_change();
    for_arcs::max_change c;
    arcs.visit(c);
    return c.get();
}

Weight WFST::sumOfAllPaths(List<int> &inSeq, List<int> &outSeq)
{
    Assert(valid());
    training_corpus corpus;
    corpus.add(inSeq,outSeq);
    IOSymSeq const& s=corpus.examples.front();
    forward_backward fb(*this,false,false,false,false);
    fb.prepare(corpus,false);
    fb.matrix_compute(s,false);
    return fb.f[s.i.n][s.o.n][final];
}

ostream& operator << (ostream &out, struct State &s){ // Yaser 7-20-2000
    out << s.arcs << '\n';
    return(out);
}

ostream & operator << (ostream &o, IOPair p)
{
    return o << '(' << p.in << ' ' << p.out << ')';
}

ostream & operator << (ostream &o, DWPair p)
{
    return o << "#"<<p.id<<"->"<<p.dest;
}


ostream & hashPrint(const HashTable<IOPair, List<DWPair> > &h, ostream &o) {
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

void WFST::read_training_corpus(std::istream &in,training_corpus &corpus) 
{
    string buf;
    unsigned input_lineno=0;
    for ( ; ; ) {
        FLOAT_TYPE weight = 1;
        getline(in,buf);
        if ( !in )
            break;
        ++input_lineno;
                        
        if ( isdigit(buf[0]) || buf[0] == '-' || buf[0] == '.' ) {
            istringstream w(buf.c_str());
            w >> weight;
            if ( w.fail() ) {
                Config::warn() << "Bad training example weight: " << buf << std::endl;
                continue;
            }
            getline(in,buf);
            if ( !in )
                break;
            ++input_lineno;                            
        }
        WFST::symbol_ids ins(*this,buf.c_str(),0,input_lineno);
        getline(in,buf);
        if ( !in )
            break;
        ++input_lineno;
                        
        WFST::symbol_ids outs(*this,buf.c_str(),1,input_lineno);
        corpus.add(ins, outs, weight);
    }
    
}

}
