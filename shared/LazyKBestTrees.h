#ifndef LAZYKBESTTREES_H
#define LAZYKBESTTREES_H


#include <vector>

using namespace std;


// supports binary weighted AND-labeled forests with alternating AND/OR
// bipartite graph structure (essentially, a hypergraph)
// cost_T should act like nonneg reals with addition
// result_F should be a functor for bottom-up combination of binary/unary ANDs as a function of AND labels:
// e.g.:
/*
  struct result_F {
     typedef int result_T;
     typedef int label_T;
     static void set_end(result_T &result) { result=-1; }
     static bool is_end(result_T result) { return result==-1; }
     static void set_pending(result_T &result) { result=-2; }
     static bool is_pending(result_T result) { return result==-2; }
     result_T operator ()(label_T label) const { return label;} // leaf
     result_T operator ()(label_T label,result_T unary) const { return unary+label;} // unary
     result_T operator ()(label_T label,result_T left,result_T right) const { return left+right+label;} // binary
     bool operator <(result_T a, result_T b) const { // < means cheaper/better
        return a < b;
     }
  };
*/

template <class cost_T,class result_F>
struct LazyKBest {
    typedef result_F F;
    typedef cost_T Cost;
    typedef typename F::result_T Result;
    typedef typename F::label_T Label;
    F result;
    struct OR;
    struct AND;

    struct BestMemo {
        vector<Result> kbest;        
    };
    
    
    // both OR/AND: the kbest vector always contains cached 1...n best, or
    // result.is_null(kbest[kbest.size()-1]) if no more can be generated
    struct OR {
        typedef pair<cost_T,unsigned> Instance; // index of OR-child to take next-best from
        vector<Instance> queue; // cheapest first
        vector<AND *> children;
        vector<Result> kbest;        
    };
    struct AND { // unary doesn't use queue.  leaf doesn't use anything except label
        Label label;
        typedef pair<cost_T,pair<unsigned,unsigned> > Instance; // index of (left,right)-child to take next-best from
        vector<Instance> queue; // cheapest first
        pair<OR *,OR *> children;
        vector<Result> kbest;
        bool isUnary() const {
            return children.second==NULL;
        }
    };    
    
};

#endif
