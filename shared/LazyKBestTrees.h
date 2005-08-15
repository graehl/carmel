#ifndef LAZYKBESTTREES_H
#define LAZYKBESTTREES_H

#include <new> // placement ::operator new(address) T()
#include <vector>
#include "Debug.h"

// TODO: implement deletion of all the lazykbest results (currently relies on pool/batch deletion)
//TODO:  MIGHT IT BE MORE FLEXIBLE TO PASS RESULT BY VALUE?

//USAGE:

template <class R,class A >
struct lazy_kbest;

/*
  struct R {
   result(result *prototype, result *old_child, result *new_child,unsigned which_child);
   bool operator < (const result &other) const; //  worse < better!
  };

  struct ResultAlloc {
      R *allocate(); // won't ever be deallocated unless you keep track of allocations yourself
  };
 */



// your operator new must be able to log and recover allocations or this will leak;  also double-initializes :(
template <class T>
struct DefaultNewAlloc {
    typedef T allocated_type;
    T *allocate() const {
        return new T();
    }
    void deallocate(T *p) const {
        delete p;
    }
};


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
        deallocate_all();
    }
    void deallocate_all() {
        for (typename std::vector<T*>::const_iterator i=allocated.begin(),e=allocated.end();i!=e;++i)
            if (destruct)
                delete *i;
            else
                ::operator delete((void *)*i);
    }
    void deallocate(T *p) const {
        // can only deallocate_all()
    }
    T *allocate() {
        allocated.push_back((T *)::operator new(sizeof(T)));
        return allocated.back();
    }
};


#ifdef GRAEHL_HEAP
#include "2heap.h"
#else
#include <algorithm>
#endif


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

        o << "{Entry(";
        if ( child[0]) {
            o << child[0] << '[' << childbp[0] << ']';
            if (child[1])
                o << "," << child[1] << '[' << childbp[1] << ']';
        }

        o << ")=" << *result;
        o << '}';

    }
    //        template <class r,class a> friend std::ostream & operator <<(std::ostream &,const typename lazy_kbest<r,a>::Entry &);
};

template <class R,class A=DefaultPoolAlloc<R> >
struct Node {
    typedef R Result;
    static R *PENDING() {
        return (R *)0x1;
    }
    typedef Node<R,A> Self;
    typedef Self default_print_on;
    typedef default_print_on has_print_on;
    void print_on(std::ostream &o) const
    {
        o << "{NODE @" << this << '[' << memo.size() << ']';
        if (memo.size())
            o << ": " << " first=" << *first_best() << " last=" << *last_best() << " pq=" << pq; // "  << memo=" << memo
        o << '}';
    }
    static A result_alloc;
    typedef Entry<R,A> QEntry; // fixme: make this indirect for faster heap ops
    typedef std::vector<QEntry> pq_t;
    typedef std::vector<R *> memo_t;

    //MEMBERS:
    pq_t pq;     // INVARIANT: pq[0] contains the last entry added to memo
    memo_t memo;


    Node() {
        //            make_done();
    }

    // IDEA: LAZY!!!
    // only do the work of computing succesors to nth best when somebody ASKS for n+1thbest
    // INVARIANT: pq[0] contains the last entry added to memo
    // IF: a new n is asked for: must be 1 off the end of memo; push it as PENDING and go to work:
    //// get succesors to pq[0] and heapify, storing memo[n]=top().  if no more left, memo[n]=NULL
    // DONE when: pq is empty, or memo[n] = NULL
    R *get_best(unsigned n) {
        INFOT("GET_BEST n=" << n << " node=" << *this); // //
        NESTT;
        if (n < memo.size()) {
            if (memo[n] == PENDING()) {
                ERRORQ("LazyKBest::get_best","memo entry " << n << " is pending - there must be a negative cost cycle - returning DONE instead (this means that the caller's nbest will be filled without using me)... n-1th best=" << memo[n-1]);
                memo[n] = NULL;
            }
            assertlvl(99,memo[n] != PENDING());
            return memo[n]; // may be DONE
        } else {
            assertlvl(19,n==memo.size());
            if (this->done()) {
                memo.push_back(NULL);
                return NULL;
            }
            memo.push_back(PENDING()); //FIXME: use dynarray.h and push back without init?
            //                IF_ASSERT(11) memo[n].result=PENDING;
            return (memo[n]=next_best());
        }
    }
    // returns last non-DONE result (one must exist!)
    R *last_best() const {
        assertlvl(11,memo.size() && memo.front());
        if (memo.back() && memo.back() != PENDING())
            return memo.back();
        assertlvl(11,memo.size()>1);
        return *(memo.end()-2);
    }
    // returns best non-DONE result (one must exist!)
    R *first_best() const {
        assertlvl(11,memo.size() && memo.front() && memo.front() != PENDING());
        return memo.front();
    }
        // returns essentially top().result
    //// INVARIANT: top() contains the next best entry
    // PRECONDITION: don't call if pq is empty. (if !done()).  memo ends with [old top result,PENDING].
    R *next_best() {
        assertlvl(11,!done());
        //            if (pq.empty()) return NULL;
        QEntry pending=pq.front(); // creating a copy saves so many ugly complexities in trying to make pop_heap / push_heap efficient ...
        pop(); // since we made a copy already into pending...

        R *old_parent=pending.result;
        assertlvl(19,memo.size()>=2 && memo.back() == PENDING() && old_parent==memo[memo.size()-2]);
        if (pending.child[0]) { // increment first
            BUILDSUCC(pending,old_parent,0);
            if (pending.child[1] && --pending.childbp[0]==0) { // increment second only if first is initial - one path to any (a,b)
                BUILDSUCC(pending,old_parent,1); // increments childbp[1]
            }
        }

        if (pq.empty())
            return NULL;
        else
            return top().result;
    }

    // PRE: unincremented pending
    //POST: if pending succesor incrementing ith child exists, increment, create new result, and add to heap
    void BUILDSUCC(QEntry &pending,R *old_parent,unsigned i)  {
        Node *child_node=pending.child[i];
        R *old_child=child_node->memo[pending.childbp[i]];
        assertlvl(99,old_child=child_node->get_best(pending.childbp[i]));

        //            R *new_child;
        INFOT("BUILDSUCC #" << i << " @" << this << ": " << " old_child=" <<old_child << *this);
        NESTT;

        if (R *new_child=(child_node->get_best(++pending.childbp[i]))) {         // has child-succesor
            INFOT("HAVE CHILD SUCCESOR TO " << *this << ": @" << i << ' ' << pending.childbp[0] << ',' << pending.childbp[1]);
            pending.result=result_alloc.allocate();
            new(pending.result) R(old_parent,old_child,new_child,i); // FIXME: ensure placement new is used - how?  ::operator new is bad ...
            INFOT("new result="<<*pending.result);
            push(pending);
        }
    }

    // must be added from best to worst order
    void add_sorted(Result *r,Node<R,A> *left=NULL,Node<R,A> *right=NULL) {
        INFOT("add_sorted this=" << this << " result=" << *r << " left=" << left << " right=" << right);
        QEntry e;
        e.set(r,left,right);
        if (pq.empty()) { // first added
            assertlvl(29,memo.empty());
            memo.push_back(r);
        }
        pq.push_back(e);
        INFOT("done (heap) " << e);
    }


        void make_done() {
            //            top().result = NULL;
            assert("unsupported"==0);
        }
        bool done() const {
            //            return top().result == NULL;
            return pq.empty();
        }
        void push(const QEntry &e) {
            pq.push_back(e);
#ifdef GRAEHL_HEAP
             //FIXME: use dynarray.h? so you don't have to push on a copy of e first
            heapAdd(pq.begin(),pq.end(),e);
#else
            push_heap(pq.begin(),pq.end());
                        //This algorithm puts the element at position end()-1 into what must be a pre-existing heap consisting of all elements in the range [begin(), end()-1), with the result that all elements in the range [begin(), end()) will form the new heap. Hence, before applying this algorithm, you should make sure you have a heap in v, and then add the new element to the end of v via the push_back member function.
#endif
        }
        void pop() {
#ifdef GRAEHL_HEAP
            heapPop(pq.begin(),pq.end());
#else
            pop_heap(pq.begin(),pq.end());
            //This algorithm exchanges the elements at begin() and end()-1, and then rebuilds the heap over the range [begin(), end()-1). Note that the element at position end()-1, which is no longer part of the heap, will nevertheless still be in the vector v, unless it is explicitly removed.
#endif
            pq.pop_back();
        }
        const QEntry &top() const {
            return pq.front();
        }

};

template <class R,class A>
A Node<R,A>::result_alloc;

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

template <class R,class A>
std::ostream &operator <<(std::ostream &o,const Entry<R,A> &e)
{
    e.print_on(o);
    return o;
}

template <class R,class A>
std::ostream &operator <<(std::ostream &o,const Node<R,A> &e)
{
    e.print_on(o);
    return o;
}

} //NAMESPACE lazy_kbest_impl



/*
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
*/


// MAIN DRIVER CLASS:
template <class R,class A=DefaultPoolAlloc<R> >
struct lazy_kbest {
//    lazy_kbest(const A &_result_alloc=A()) : result_alloc(_result_alloc) {}
//    const Result *DONE=NULL;
    //    const R *PENDING=(R *)0x1;
    //    template <class r,class q> struct Node;
    //    template <class r,class q>
    // to use: initialize pending with viterbi.

    // bool visit(R &result,unsigned rank) // rank 0...k-1 (or earlier if forest has fewer trees) - stop early if returns false

    typedef lazy_kbest_impl::Node<R,A> Node;
    template <class Visitor>
    static void enumerate_kbest(unsigned k,Node *goal,Visitor visit=Visitor()) {
    typedef R Result;
        INFOT("COMPUTING BEST " << k << " for node " << *goal);
        NESTT;
        for (unsigned i=0;i<k;++i) {
            R *ith=goal->get_best(i);
            if (!ith) break;
            if (!visit(*ith,i)) break;
        }
    }

    // for best effect, do this before enumerate_kbest
    static void print_forest_rec(Node *top) {

    }
    /*
    void deallocate_all() {
        Node::result_alloc.deallocate_all();
    }
    */
};




#ifdef TEST
//# include "default_print_on.hpp"
//FIXME: doesn't work

# include "test.hpp"
# include <iostream>
# include <string>
using namespace std;
namespace ns_TEST_lazy_kbest {

struct Result {
    char val;
    string history;
    float cost;
    string tree;

    Result(char v,float cost_=1) : val(v), history(1,v), cost(cost_) {}
    friend ostream & operator <<(ostream &o,const Result &v);
    Result(Result *prototype, Result *old_child, Result *new_child,unsigned which_child) {
        NESTT;
        INFOT("NEW RESULT proto=" << prototype->val << " old_child=" << *old_child << " new_child=" << *new_child << " childid=" << which_child);
        val = prototype->val;
        std::ostringstream newhistory,newtree;
        newhistory << prototype->history << ',' << (which_child ? "L" : "R") << '-' << old_child->cost << "+" << new_child->cost;
        //<< '(' << new_child->history << ')';
        history = newhistory.str();
        newtree << val << '(' << new_child->tree << ')';
        tree = newtree.str();

        cost = prototype->cost + - old_child->cost + new_child->cost;
    }
    bool operator < (const Result &other) const {
        return cost > other.cost;
    } //  worse < better!

};

ostream & operator <<(ostream &o,const Result &v)
{
    return o << "{{{val=" << v.val << " cost=" << v.cost << " tree=" << v.tree  << " history=" << v.history << "}}}";
}


struct ResultPrinter {
    void operator()(const Result &r,unsigned i) const {
        NESTT;
        INFOT("Visiting result #" << i << r);
        INFOT("");
        cout << "RESULT #" << i << "=" << r << "\n";
        INFOT("");
    }
};

typedef lazy_kbest<Result> LK;
}

BOOST_AUTO_UNIT_TEST(TEST_lazy_kbest) {
    using namespace ns_TEST_lazy_kbest;
    //a:6(b:5[OR b:10 f a ()],c:1(b:5,f:1))
    LK::Node a,b,c,f;
    Result ra('a',6),rb('b',5),rc('c',1),rf('d',2),rb2('B',5),rb3('D',10),ra2('A',12);
    a.add_sorted(&ra,&b,&c);
    a.add_sorted(&ra2,&a);
    b.add_sorted(&rb); // terminal
    b.add_sorted(&rb2,&f);
    b.add_sorted(&rb3,&b);
    c.add_sorted(&rc,&b,&f);
    f.add_sorted(&rf); // terminal
    NESTT;
    LK::enumerate_kbest(1,&f,ResultPrinter());
    LK::enumerate_kbest(1,&b,ResultPrinter());
    LK::enumerate_kbest(15,&a,ResultPrinter());

}
#endif


#endif
