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
     typedef float cost_T;
     static void set_end(cost_T &cost) { cost=-1; }
     static bool is_end(cost_T cost) { return cost==-1; }
     static void set_pending(cost_T &cost) { cost=-2; }
     static bool is_pending(cost_T cost) { return cost==-2; }
     
     cost_T getCost(label_T label) const { return label;} // leaf
     cost_T getCost(label_T label,cost_T unary) const { return unary+label;} // unary
     cost_T getCost(label_T label,cost_T left,cost_T right) const { return left+right+label;} // binary
     
// optional (you can follow backpointers yourself)
     result_T *buildResult(label_T label) const { return label;} // leaf
     result_T *buildResult(label_T label,result_T *unary) const { return unary+label;} // unary
     result_T *buildResult(label_T label,result_T *left,result_T *right) const { return left+right+label;} // binary
  };
*/

template <class cost_T,class result_F,class Alloc>
struct LazyKBest {
    typedef result_F F;
    typedef cost_T Cost;
    typedef typename F::result_T Result;
    typedef typename F::label_T Label;
    F result;
    struct OR;
    struct AND;
    typedef std::pair<Cost,Result *> Tree;

    struct BestMemo {
        vector<Tree> kbest;        
    };
    struct OrNode {
        vector<BestMemo> memo; // index by kth best
        struct AndNode {
            Label label;
            OrNode *first,*second;
            AndNode(const Label &l) : label(l),first(NULL),second(NULL) {}
            AndNode(const Label &l,OrNode *unary) : label(l),first(unary),second(NULL) {}
            AndNode(const Label &l,OrNode *left,OrNode *right) : label(l),first(left),second(right) {}            
        };
        vector<AndNode> tails;
        
        typedef std::pair<cost_T
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
