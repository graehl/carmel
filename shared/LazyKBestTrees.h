#ifndef LAZYKBESTTREES_H
#define LAZYKBESTTREES_H


#include <vector>
#include "Debug.h"


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

    typedef typename F::result_T Result;
    F result;
    typedef typename F::label_T Label;
    struct OR;
    struct AND;
    typedef std::pair<Cost,Result *> Tree;

    struct BestMemo {
        std::vector<Tree> kbest;        
    };
    struct OrNode {
        std::vector<BestMemo> memo; // index by kth best
        struct AndNode {
            Label label;
            OrNode *first,*second;
            AndNode(const Label &l) : label(l),first(NULL),second(NULL) {}
            AndNode(const Label &l,OrNode *unary) : label(l),first(unary),second(NULL) {}
            AndNode(const Label &l,OrNode *left,OrNode *right) : label(l),first(left),second(right) {}            
        };
        std::vector<AndNode> tails;
        
        typedef std::pair<cost_T
    };
    
    
    // both OR/AND: the kbest std::vector always contains cached 1...n best, or
    // result.is_null(kbest[kbest.size()-1]) if no more can be generated
    struct OR {
        typedef pair<cost_T,unsigned> Instance; // index of OR-child to take next-best from
        std::vector<Instance> queue; // cheapest first
        std::vector<AND *> children;
        std::vector<Result> kbest;        
    };
    struct AND { // unary doesn't use queue.  leaf doesn't use anything except label
        Label label;
        typedef pair<cost_T,pair<unsigned,unsigned> > Instance; // index of (left,right)-child to take next-best from
        std::vector<Instance> queue; // cheapest first
        pair<OR *,OR *> children;
        std::vector<Result> kbest;
        bool isUnary() const {
            return children.second==NULL;
        }
    };    

*/


template <class T,bool destruct=true>
struct DefaultPoolAlloc {
    std::vector <T *> allocated;
    typedef T allocated_type;
    DefaultPoolAlloc() {}
    // move semantics:
    DefaultPoolAlloc(DefaultPoolAlloc<T> &other) : allocated(other.allocated) {
        other.allocated.clear();
    }
    DefaultPoolAlloc(const DefaultPoolAlloc<T> &other) : allocated() {
        assert(other.allocated.empty());
    }
    ~DefaultPoolAlloc() {
        for (typename std::vector<T*>::const_iterator i=allocated.begin(),e=allocated.end();i!=e;++i)
            if (destruct)
                delete *i;
            else
                ::operator delete((void *)*i);
    }
    T *allocate() {
        allocated.push_back((T *)::operator new(sizeof(T)));
        return allocated.back();
    }
};


/*
  struct result_t {
   result(result *prototype, result *old_child, result *new_child,unsigned which_child);
   bool operator < (const result &other) const; //  worse < better!
  };

  struct ResultAlloc {
      result_t *allocate(); // won't ever be deallocated unless you keep track of allocations yourself
  };
 */
#ifdef GRAEHL_HEAP
#include "2heap.h"
#else
#include <algorithm>
#endif

template <class result_t,class ResultAlloc=DefaultPoolAlloc<result_t> >
struct LazyKBest {
//    LazyKBest(const ResultAlloc &_result_alloc=ResultAlloc()) : result_alloc(_result_alloc) {}
    typedef result_t Result;
//    const Result *DONE=NULL;
    enum {
        PENDING=0x1
    };
    //    const result_t *PENDING=(result_t *)0x1;
    struct Node;
    struct Entry {
        typedef std::vector<result_t *> pq_t;
        unsigned childbp[2];
        Node *child[2];
        result_t *result;
        void set(result_t *r,Node *c0,Node *c1) {
            childbp[0]=childbp[1]=0;
            child[0]=c0;
            child[1]=c1;
            result=r;
        }
        // note: 2heap.h provides a max heap, so we want the same < as result_t
        inline bool operator <(const Entry &o) const {
            assertlvl(19,result && o.result);
            return *result < *o.result;
        }
    };
    // to use: initialize pending with viterbi.
    struct Node {
        static ResultAlloc result_alloc;
        typedef Entry QEntry; // fixme: make this indirect for faster heap ops
        typedef std::vector<QEntry> pq_t;
        pq_t pq;
        typedef std::vector<result_t *> memo_t;
        memo_t memo;
        QEntry pending; // store top here so after pop, successors can be deferred until next top is needed
        Node() {
            make_done();
        }
        result_t *get_best(unsigned n) {
            if (n < memo.size()) {
                assertlvl(11,memo[n] != (result_t *)PENDING);
                return memo[n]; // may be DONE
            } else {
                assertlvl(19,n==memo.size());
                if (done())
                    return NULL;
                memo.push_back((result_t *)PENDING); //FIXME: use dynarray.h?
//                IF_ASSERT(11) memo[n].result=PENDING;
                return (memo[n]=next());                
            }
        }
        // must be added from best to worst order
        void add_sorted(Result *r,Node *left=NULL,Node *right=NULL) {            
            if (done()) {
                pending.set(r,left,right);
            } else {
                Entry e;
                e.set(r,left,right);
                pq.push_back(e);
            }            
        }
        void make_done() {
            pending.result = NULL;
        }
        bool done() const {
            return pending.result == NULL;
        }
        void push(const QEntry &e) {
            pq.push_back(e);
#ifdef GRAEHL_HEAP
             //FIXME: use dynarray.h? so you don't have to push on a copy of e first
            heapAdd(pq.begin(),pq.end(),e);
#else
            push_heap(pq.begin(),pq.end());
#endif 
        }
        void pop() {
#ifdef GRAEHL_HEAP
            heapPop(pq.begin(),pq.end());
#else
            pop_heap(pq.begin(),pq.end());
#endif 
            pq.pop_back();
        }
        const QEntry &top() const {
            return pq.front();
        }
        void push_pending_succ() {
            assertlvl(99,!done());
            result_t *old_parent=pending.result;
            result_t *new_child;
#define BUILDSUCC(i) do { \
                  if (new_child=pending.child[i]->get_best(++pending.childbp[i])) { \
                    pending.result=result_alloc.allocate(); \
                    result_t *old_child=pending.child[i]->memo[pending.childbp[i]]; \
                    new(pending.result) result_t(old_parent,old_child,new_child,i);  \
                    push(pending); }  \
                  }while(0)
            if (pending.child[0]) {                
                BUILDSUCC(0);                
                if (pending.child[1] && --pending.childbp[0]==0) { // only increment second if first is initial - one path to every double (a,b) // -- to undo ++ in BUILDSUCC
                    BUILDSUCC(1);
                }
            }
#undef BUILDSUCC
        }
        result_t *next() {
            assertlvl(11,!done());
            result_t *ret=pending.result;
            push_pending_succ();
            if (pq.empty())
                make_done();
            else {                
                pending=top();
                pop();
            }
            return ret;            
        }
    };
    
    // visit(result_t &result,unsigned rank) // rank 0...k-1 (or earlier if forest has fewer trees)
    template <class Visitor>
    void enumerate_kbest(unsigned k,Node *goal,Visitor visit=Visitor()) {
        for (unsigned i=0;i<k;++i) {
            result_t *ith=goal->get_best(i);
            if (!ith) break;
            visit(*ith,i);
        }
    }
};

#ifdef MAIN
template <class result_t,class ResultAlloc >
ResultAlloc LazyKBest<result_t,ResultAlloc>::Node::result_alloc;
#endif

#ifdef TEST
# include "test.hpp"
# include <iostream>
using namespace std;


struct Result {
    unsigned val;
    Result(unsigned v) : val(v) {}
    Result(Result *prototype, Result *old_child, Result *new_child,unsigned which_child) {
        cerr << "proto=" << prototype->val << " old_child=" << old_child->val << " new_child=" << new_child->val << " childid=" << which_child << endl;
    }
    bool operator < (const Result &other) const {
        return true;
    } //  worse < better!

};

struct ResultPrinter {
    void operator()(const Result &r,unsigned i) const {
        cerr << "Visiting result #" << i << " val=" << r.val << endl;
    }
};

BOOST_AUTO_UNIT_TEST(TEST_LazyKBest) {
    typedef LazyKBest<Result> k;
    //a(b[OR b f ()],c(b,f))
    k::Node a,b,c,f;
    Result ra(1),rb(2),rc(3),rf(6),rb2(0),rb3(10);
    a.add_sorted(&ra,&b,&c);
    b.add_sorted(&rb); // terminal
    b.add_sorted(&rb2,&f);
    b.add_sorted(&rb3,&b);
    c.add_sorted(&rc,&b,&f);
    f.add_sorted(&rf); // terminal
    k().enumerate_kbest(5,&a,ResultPrinter());
    
    BOOST_CHECK(0);
}
#endif

#endif
