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
     typedef int R;
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
     R *buildResult(label_T label) const { return label;} // leaf
     R *buildResult(label_T label,R *unary) const { return unary+label;} // unary
     R *buildResult(label_T label,R *left,R *right) const { return left+right+label;} // binary
  };

    typedef typename F::R Result;
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
  struct R {
   result(result *prototype, result *old_child, result *new_child,unsigned which_child);
   bool operator < (const result &other) const; //  worse < better!
  };

  struct ResultAlloc {
      R *allocate(); // won't ever be deallocated unless you keep track of allocations yourself
  };
 */
#ifdef GRAEHL_HEAP
#include "2heap.h"
#else
#include <algorithm>
#endif

template <class R,class A >
struct lazy_kbest;

//template <class r,class a> std::ostream &operator <<(std::ostream &o,const typename lazy_kbest<r,a>::LazyKEntry &e);


namespace lazy_kbest_impl {

template <class R,class A>
struct Node;

template <class R,class A=DefaultPoolAlloc<R> >
    struct Entry {
        typedef std::vector<R *> pq_t;
        unsigned childbp[2];
        Node<R,A> *child[2]; 
        R *result;
        void set(R *_result,Node<R,A> *c0,Node<R,A> *c1) {
            childbp[0]=childbp[1]=0;
            child[0]=c0;
            child[1]=c1;
            result=_result;
        }
        // note: 2heap.h provides a max heap, so we want the same < as R
        inline bool operator <(const Entry &o) const {
            assertlvl(19,result && o.result);
            return *result < *o.result;
        }
        typedef Entry<R,A>Self;
        typedef Self default_print_on;
        typedef default_print_on has_print_on;

        void print_on(std::ostream &o) const
        {
            o << '(' << child[0] << '[' << childbp[0] << "]," << child[1] << '[' << childbp[1] << "])=" << *result;
        }
        //        template <class r,class a> friend std::ostream & operator <<(std::ostream &,const typename lazy_kbest<r,a>::Entry &);
        //FIXME: g++ barfs on nested class + externally defined operator/member ... unnest Entry?
    };

template <class R,class A=DefaultPoolAlloc<R> >
    struct Node {
        typedef R Result;        
        enum {
            PENDING=0x1
        };

        typedef Node<R,A> Self;
        typedef Self default_print_on;
        typedef default_print_on has_print_on;
        void print_on(std::ostream &o) const
        {
            o << '{' << this << ": " << " pending=" << pending << " memo=" << memo << '}';
        }
        
        static A result_alloc;
        typedef Entry<R,A> QEntry; // fixme: make this indirect for faster heap ops
        typedef std::vector<QEntry> pq_t;
        pq_t pq;
        typedef std::vector<R *> memo_t;
        memo_t memo;
        QEntry pending; // store top here so after pop, successors can be deferred until next top is needed
        Node() {
            make_done();
        }
        R *get_best(unsigned n) {
            INFOTEST("node=" << *this << " n=" << n); // // 
            if (n < memo.size()) {
                assertlvl(11,memo[n] != (R *)PENDING);
                return memo[n]; // may be DONE
            } else {
                assertlvl(19,n==memo.size());
                if (done()) {
                    memo.push_back(NULL);                    
                    return NULL;
                }
                memo.push_back((R *)PENDING); //FIXME: use dynarray.h?
//                IF_ASSERT(11) memo[n].result=PENDING;
                return (memo[n]=next());                
            }
        }
        // must be added from best to worst order
        void add_sorted(Result *r,Node<R,A> *left=NULL,Node<R,A> *right=NULL) {            
            INFOTEST("add_sorted this=" << this << " result=" << *r << " left=" << left << " right=" << right);
            if (done()) {
                pending.set(r,left,right);
                INFOTEST("done (pending) " << pending);
            } else {
                QEntry e;
                e.set(r,left,right);
                pq.push_back(e);
                INFOTEST("done (heap) " << e);
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
        // modifies pending heavily and pushes it (after you incremented childbp[i]
        void BUILDSUCC(R *old_parent,unsigned i)  {
            R *old_child=pending.child[i]->memo[pending.childbp[i]];     
            //            R *new_child;
            INFOTEST("BUILDSUCC " << this << ": " << i << ' ' << pending.childbp[0] << ',' << pending.childbp[1] << " old_child=" <<old_child);
            if (R *new_child=(pending.child[i])->get_best(++pending.childbp[i])) {        
                INFOTEST(this << ": " << i << ' ' << pending.childbp[0] << ',' << pending.childbp[1]);  
                pending.result=result_alloc.allocate(); 
                new(pending.result) R(old_parent,old_child,new_child,i);  
                push(pending);
            }              
        }
        void push_pending_succ() {
            assertlvl(99,!done());
            R *old_parent=pending.result;
            
            /*#define BUILDSUCC(old_parent,i) do {                               \
                    R *old_child=pending.child[i]->memo[pending.childbp[i]];     \
                    INFOTEST("BUILDSUCC " << this << ": " << i << ' ' << pending.childbp[0] << ',' << pending.childbp[1] << " old_child=" <<old_child);        \
                if (new_child=(pending.child[i])->get_best(++pending.childbp[i])) {        \
                    INFOTEST(this << ": " << i << ' ' << pending.childbp[0] << ',' << pending.childbp[1]);  \
                    pending.result=result_alloc.allocate(); \
                    new(pending.result) R(old_parent,old_child,new_child,i);  \
                    push(pending); }  \
                    }while(0)*/
            if (pending.child[0]) { // always increment first childbp
                BUILDSUCC(old_parent,0);                
                if (pending.child[1] && --pending.childbp[0]==0) { // only increment second childbp if first is initial - so one path to every pair (a,b) // -- to undo ++ in BUILDSUCC
                    BUILDSUCC(old_parent,1);
                }
            }
#undef BUILDSUCC
        }
        R *next() {
            assertlvl(11,!done());
            R *ret=pending.result;
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
#ifdef MAIN
template <class R,class A>
A Node<R,A>::result_alloc;
#endif
template <class V>
std::ostream & operator << (std::ostream &o,const std::vector<V> &v) 
{
    o << '[';
    bool first=true;
    for (typename std::vector<V>::const_iterator i=v.begin(),e=v.end();i!=e;++i) {
        if (first)
            first=false;
        else
            o << ',';
        o << *i;
    }
    o << ']';
    return o;
}


} //NAMESPACE lazy_kbest_impl



template <class R,class A>
std::ostream &operator <<(std::ostream &o,const lazy_kbest_impl::Entry<R,A> &e)
{
    e.print_on(o);
    return o;
}

template <class R,class A>
std::ostream &operator <<(std::ostream &o,const lazy_kbest_impl::Node<R,A> &e)
{
    e.print_on(o);
    return o;
}



template <class R,class A=DefaultPoolAlloc<R> >
struct lazy_kbest {
    typedef lazy_kbest_impl::Entry<R,A> Entry;
    typedef lazy_kbest_impl::Node<R,A> Node;
//    lazy_kbest(const A &_result_alloc=A()) : result_alloc(_result_alloc) {}
    typedef R Result;
//    const Result *DONE=NULL;
    //    const R *PENDING=(R *)0x1;
    //    template <class r,class q> struct Node;
    //    template <class r,class q>
    // to use: initialize pending with viterbi.
    
    // visit(R &result,unsigned rank) // rank 0...k-1 (or earlier if forest has fewer trees)
    template <class Visitor>
    void enumerate_kbest(unsigned k,Node *goal,Visitor visit=Visitor()) {
        for (unsigned i=0;i<k;++i) {
            R *ith=goal->get_best(i);
            if (!ith) break;
            visit(*ith,i);
        }
    }
};




#ifdef TEST
//# include "default_print_on.hpp"
//FIXME: doesn't work

# include "test.hpp"
# include <iostream>
using namespace std;

struct Result {
    char val;
    Result(char v) : val(v) {}
    Result(Result *prototype, Result *old_child, Result *new_child,unsigned which_child) {
        cerr << "proto=" << prototype->val << " old_child=" << old_child->val << " new_child=" << new_child->val << " childid=" << which_child << endl;
    }
    bool operator < (const Result &other) const {
        return val < other.val;
    } //  worse < better!

};

ostream & operator <<(ostream &o,const Result &v) 
{
    return o << "(val=" << v.val << ')';
}


struct ResultPrinter {
    void operator()(const Result &r,unsigned i) const {
        cerr << "Visiting result #" << i << " val=" << r.val << endl;
    }
};

typedef lazy_kbest<Result> LK;

BOOST_AUTO_UNIT_TEST(TEST_lazy_kbest) {
    //a(b[OR b f ()],c(b,f))
    LK::Node a,b,c,f;
    Result ra('a'),rb('b'),rc('c'),rf('d'),rb2('B'),rb3('D');
    a.add_sorted(&ra,&b,&c);
    b.add_sorted(&rb); // terminal
    b.add_sorted(&rb2,&f);
    b.add_sorted(&rb3,&b);
    c.add_sorted(&rc,&b,&f);
    f.add_sorted(&rf); // terminal
    LK().enumerate_kbest(5,&a,ResultPrinter());
    
    BOOST_CHECK(0);
}
#endif


#endif
