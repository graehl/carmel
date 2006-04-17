#ifndef GRAEHL__SHARED__LAZY_FOREST_KBEST_HPP
#define GRAEHL__SHARED__LAZY_FOREST_KBEST_HPP

#ifdef TEST
# define LAZY_FOREST_EXAMPLES
#endif
#ifdef LAZY_FOREST_EXAMPLES
#include <graehl/shared/info_debug.hpp>
#endif
#include <graehl/shared/assertlvl.hpp>

#ifndef INFOT
# define KBESTINFOT(x)
# define KBESTNESTT
#else
# define KBESTINFOT(x) INFOT(x)
# define KBESTNESTT NESTT
#endif 
#ifndef ERRORQ
# include <iostream>
# define KBESTERRORQ(x,y) do{ std::cerr << "\n" << x << ": "<<y<<std::endl; }while(0)
#else
# define KBESTERRORQ(x,y) ERRORQ(x,y)
#endif 

#include <vector>
#include <stdexcept>
#ifdef GRAEHL_HEAP
#include <graehl/shared/2heap.h>
#else
#include <algorithm>
#include <ext/algorithm> // is_sorted
#endif

inline std::ostream & operator <<(std::ostream &o,unsigned bp[2])
{
    return o << '[' << bp[0] << ','<<bp[1]<<']';
}

template <class Deriv>
bool derivation_better_than(const Deriv &me,const Deriv &than)
{
    return me > than;
}

struct lazy_derivation_cycle : public std::runtime_error 
{
    lazy_derivation_cycle() : std::runtime_error("lazy_forest::get_best tried to compute a derivation that eventually depended on itself - probable cause: negative cost cycle") {}
};
        

/// build a copy of your (at most binary) derivation forest, then query its root for the 1st, 2nd, ... best
/*
    struct DerivationFactory 
    {
        /// can override derivation_better_than(derivation_type,derivation_type).
        /// derivation_type should be a lightweight (value) object
        /// if INFOT debug prints are enabled, must also have o << deriv
        typedef Result *derivation_type;
        
        /// special derivation values (not used for normal derivations) (may be of different type but convertible to derivation_type)
        static derivation_type PENDING();
        static derivation_type NONE();
/// derivation_type must support: initialization by (copy), assignment, ==, !=
        
        /// take an originally better (or best) derivation, substituting for the changed_child_index-th child new_child for old_child (cost difference should usually be computed as (cost(new_child) - cost (old_child)))
        derivation_type make_worse(derivation_type prototype, derivation_type old_child, derivation_type new_child, unsigned changed_child_index) 
        {
            return new Result(prototype,old_child,new_child,changed_child_index);
        }
    };
*/
// TODO: implement unique visitor of all the lazykbest subresults (hash by pointer to derivation?)
template <class DerivationFactory>
class lazy_forest {
 public:
    typedef DerivationFactory derivation_factory_type;
    typedef typename derivation_factory_type::derivation_type derivation_type;
    typedef derivation_factory_type D;
    derivation_type NONE() 
    {
        return (derivation_type)derivation_factory.NONE();
    }
    derivation_type PENDING() 
    {
        return (derivation_type)derivation_factory.PENDING();
    }
    
    /// bool Visitor(derivation,ith) - if returns false, then stop early.
    /// otherwise stop after generating up to k (up to as many as exist in
    /// forest)
    template <class Visitor>
    void enumerate_kbest(unsigned k,Visitor visit=Visitor()) {
        KBESTINFOT("COMPUTING BEST " << k << " for node " << *this);
        KBESTNESTT;
        for (unsigned i=0;i<k;++i) {
            derivation_type ith_best=get_best(i);
            if (ith_best == NONE()) break;
            if (!visit(ith_best,i)) break;
        }
    }

    typedef lazy_forest<derivation_factory_type> forest;
    typedef forest Self;

    //FIXME: faster heap operations if we put handles to hyperedges on heap instead of copying?
    struct hyperedge {
        typedef hyperedge Self;
        /// antecedent subderivation forest (OR-nodes).  if unary,
        /// child[1]==NULL.  leaves have child[0]==NULL also.
        forest *child[2];
        /// index into kth best for that child's OR-node: which of the possible
        /// children we chose (0=best-cost, 1 next-best ...)
        unsigned childbp[2];
        /// the derivation that resulted from choosing the childbp[0]th out of
        /// child[0] and childbp[1]th out of child[1]
        derivation_type derivation;

        hyperedge(derivation_type _derivation,forest *c0,forest *c1) 
        {
            set(_derivation,c0,c1);
        }
        void set(derivation_type _derivation,forest *c0,forest *c1) {
            childbp[0]=childbp[1]=0;
            child[0]=c0;
            child[1]=c1;
            derivation=_derivation;
        }
        // we intend to use a max-heap, so a < b iff b is better than a
        inline bool operator <(const hyperedge &o) const {
            return derivation_better_than(o.derivation,derivation);
        }
        void print(std::ostream &o) const
        {
            o << "{hyperedge(";
            if ( child[0]) {
                o << child[0] << '[' << childbp[0] << ']';
                if (child[1])
                    o << "," << child[1] << '[' << childbp[1] << ']';
            }
            o << ")=" << derivation;
            o << '}';
        }
        template <class O>
        friend O &operator<<(O &o,const Self &e) 
        {
            e.print(o);
            return o;
        }
        typedef Self default_print;
        typedef default_print has_print;        
    };
    
    void print(std::ostream &o) const
    {
        o << "{NODE @" << this << '[' << memo.size() << ']';        
        if (memo.size()) {            
            o << ": " << " first={{{";
            o<< *first_best();
            o<< "}}} last={{{";
            o<< *last_best();
            o<< "}}}";
//          o << " pq=" << pq;          
//          o<< pq; // "  << memo=" << memo
        }        
        o << '}';
    }
    template <class O>
    friend O &operator<<(O &o,const Self &e) 
    {
        e.print(o);
        return o;
    }

    static void set_derivation_factory(derivation_factory_type const &df) 
    {
        derivation_factory=df;
    }
    
    /// if you have any state in your factory, assign to it here
    static derivation_factory_type derivation_factory;

    /// set this to true if you want negative cost cycles to throw rather than silently stop producing successors
    static bool throw_on_cycle;

    static bool is_null(derivation_type d) 
    {
        return d==NONE();
    }
    
    /// return the nth best (starting from 0) or NONE() (test with is_null(d)
    /// if the finite # of derivations in the forest is exhausted.
    /// IDEA: LAZY!!
    /// - only do the work of computing succesors to nth best when somebody ASKS
    /// for n+1thbest INVARIANT: pq[0] contains the last entry added to memo
    /// (this is true after sort() or first add_sorted()) IF: a new n is asked
    /// for: must be 1 off the end of memo; push it as PENDING and go to work:
    /// {get succesors to pq[0] and heapify, storing memo[n]=top().  if no more
    /// left, memo[n]=NONE()} You're DONE when: pq is empty, or memo[n] = NONE()
    derivation_type get_best(unsigned n) {
        KBESTINFOT("GET_BEST n=" << n << " node=" << *this); // //
        KBESTNESTT;
        if (n < memo.size()) {
            if (memo[n] == PENDING()) {
                if (throw_on_cycle)
                    throw lazy_derivation_cycle();
                KBESTERRORQ("LazyKBest::get_best","memo entry " << n << " is pending - there must be a negative cost cycle - returning DONE instead (this means that the caller's nbest will be filled without using me)... n-1th best"); //=" << memo[n-1]
                memo[n] = NONE();
            }
            return memo[n]; // may be DONE
        } else {
            assertlvl(19,n==memo.size());
            if (done()) {
                memo.push_back(NONE());
                return NONE();
            }
            memo.push_back(PENDING());
            return (memo[n]=next_best());
        }
    }
    /// returns last non-DONE derivation (one must exist!)
    derivation_type last_best() const {
        assertlvl(11,memo.size() && memo.front() != NONE());
        if (memo.back() && memo.back() != PENDING())
            return memo.back();
        assertlvl(11,memo.size()>1);
        return *(memo.end()-2);
    }
    /// returns best non-DONE derivation (one must exist!)
    derivation_type first_best() const {
        assertlvl(11,memo.size() && memo.front() != NONE() && memo.front() != PENDING());
        return memo.front();
    }
    
    /// Get next best derivation, or NONE if no more.
    //// INVARIANT: top() contains the next best entry
    /// PRECONDITION: don't call if pq is empty. (if !done()).  memo ends with [old top derivation,PENDING].
    derivation_type next_best() {
        assertlvl(11,!done());
        hyperedge pending=top(); // creating a copy saves so many ugly complexities in trying to make pop_heap / push_heap efficient ...
        KBESTINFOT("GENERATE SUCCESSORS FOR "<<pending);
        pop(); // since we made a copy already into pending...

        derivation_type old_parent=pending.derivation; // remember this because we'll be destructively updating pending.derivation below
        assertlvl(19,memo.size()>=2 && memo.back() == PENDING() && old_parent==memo[memo.size()-2]);
        if (pending.child[0]) { // increment first
            generate_successor_hyperedge(pending,old_parent,0);
            if (pending.child[1] && pending.childbp[0]==0) { // increment second only if first is initial - one path to any (a,b)
                generate_successor_hyperedge(pending,old_parent,1);
            }
        }
        if (pq.empty())
            return NONE();
        else
            return top().derivation;
    }

    /// may be followed by add()
    void add_first_sorted(derivation_type r,forest *left=NULL,forest *right=NULL)
    {
        assertlvl(9,pq.empty() && memo.empty());
        memo.push_back(r);
        add(r,left,right);
    }
    
    /// must be added from best to worst order ( r1 > r2 > r3 > ... )
    void add_sorted(derivation_type r,forest *left=NULL,forest *right=NULL) {
        if (pq.empty()) { // first added
            assertlvl(29,memo.empty());
            memo.push_back(r);
        }
        add(r,left,right);
    }

    /// may add in any order, but must call sort() before any get_best()
    void add(derivation_type r,forest *left=NULL,forest *right=NULL)
    {        
        KBESTINFOT("add this=" << this << " derivation=" << r << " left=" << left << " right=" << right);
        pq.push_back(hyperedge(r,left,right));
        KBESTINFOT("done (heap) " << r);
    }
    
    void sort()
    {
#ifdef GRAEHL_HEAP
        heapBuild(pq.begin(),pq.end());
#else
        make_heap(pq.begin(),pq.end());
#endif
        finish_adding();
    }

    /// optional: if you call only add() on sorted, you must finish_adding().  otherwise don't call, or call sort() instead
    void finish_adding() 
    {
        memo.clear();
        memo.push_back(top().derivation);
    }
    
    bool is_sorted()
    {
#ifdef GRAEHL_HEAP
        return heapVerify(pq.begin(),pq.end());
#else
        return __gnu_cxx::is_heap(pq.begin(),pq.end());
#endif 
    }
    
    bool done() const {
        return pq.empty();
    }
    
 private:
    typedef std::vector<hyperedge> pq_t;
    typedef std::vector<derivation_type > memo_t;

    //MEMBERS:
    pq_t pq;     // INVARIANT: pq[0] contains the last entry added to memo
    memo_t memo;

    //try worsening ith (0=left, 1=right) child and adding to queue
    inline void generate_successor_hyperedge(hyperedge &pending,derivation_type old_parent,unsigned i)  {
        lazy_forest &child_node=*pending.child[i];
        unsigned &child_i=pending.childbp[i];
        derivation_type old_child=child_node.memo[child_i];
        KBESTINFOT("generate_successor_hyperedge #" << i << " @" << this << ": " << " old_parent="<<old_parent<<" old_child=" <<old_child << " NODE="<<*this);
        KBESTNESTT;

        derivation_type new_child=(child_node.get_best(++child_i));
        if (new_child!=NONE()) {         // has child-succesor
            KBESTINFOT("HAVE CHILD SUCCESSOR for i=" << i << ": [" << pending.childbp[0] << ',' << pending.childbp[1] << "]");
            pending.derivation=derivation_factory.make_worse(old_parent,old_child,new_child,i);
            KBESTINFOT("new derivation: "<<pending.derivation);
            push(pending);
        }
        --child_i;
        KBESTINFOT("restored original i=" << i << ": [" << pending.childbp[0] << ',' << pending.childbp[1] << "]");
    }

    void push(const hyperedge &e) {
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
    const hyperedge &top() const {
        return pq.front();
    }
    typedef void default_print;
    typedef void has_print;    
};

template <class F>
typename lazy_forest<F>::derivation_factory_type lazy_forest<F>::derivation_factory;
template <class F>
bool lazy_forest<F>::throw_on_cycle=false;

/// THE REST OF THIS FILE IS JUST EXAMPLES

#ifdef LAZY_FOREST_EXAMPLES

//# include "default_print.hpp"
//FIXME: doesn't work

//# include "test.hpp"
# include <iostream>
# include <string>
# include <sstream>
# include <cmath>

namespace lazy_forest_kbest_example {

using namespace std;
struct Result {
    struct Factory 
    {
        typedef Result *derivation_type;
//        static derivation_type NONE,PENDING;
//        enum { NONE=0,PENDING=1 };
        static derivation_type NONE() { return (derivation_type)0;}
        static derivation_type PENDING() { return (derivation_type)1;}
        derivation_type make_worse(derivation_type prototype, derivation_type old_child, derivation_type new_child, unsigned changed_child_index) 
        {
            return new Result(prototype,old_child,new_child,changed_child_index);
        }
    };
    
        
    Result *child[2];
    string rule;
    string history;
    float cost;
    Result(const string & rule_,float cost_,Result *left=NULL,Result *right=NULL) : rule(rule_),history(rule_,0,1),cost(cost_) {
        child[0]=left;child[1]=right;
        if (child[0]) {
            cost+=child[0]->cost;
            if (child[1])
                cost+=child[1]->cost;
        }
    }
    friend ostream & operator <<(ostream &o,const Result &v)
    {
        o << "cost=" << v.cost <<  " tree={{{";
        v.print_tree(o,false);
        o <<"}}} derivtree={{{";
        v.print_tree(o);
        return o<<"}}} history={{{" << v.history << "}}}";
    }
    Result(Result *prototype, Result *old_child, Result *new_child,unsigned which_child) {
        rule=prototype->rule;
        child[0]=prototype->child[0];
        child[1]=prototype->child[1];
        EXPECT(which_child,<,2);
        EXPECT(child[which_child],==,old_child);
        child[which_child]=new_child;
        cost = prototype->cost + - old_child->cost + new_child->cost;
        KBESTNESTT;
        KBESTINFOT("NEW RESULT proto=" << *prototype << " old_child=" << *old_child << " new_child=" << *new_child << " childid=" << which_child << " child[0]=" << child[0] << " child[1]=" << child[1]);
        
        std::ostringstream newhistory,newtree,newderivtree;
        newhistory << prototype->history << ',' << (which_child ? "R" : "L") << '-' << old_child->cost << "+" << new_child->cost;
        //<< '(' << new_child->history << ')';
        history = newhistory.str();
    }
    bool operator < (const Result &other) const {
        return cost > other.cost;
    } //  worse < better!
    void print_tree(ostream &o,bool deriv=true) const
    {
        if (deriv)
            o << "["<<rule<<"]";
        else {
            o << rule.substr(rule.find("->")+2,1);    
        }
        if (child[0]) {
            o << "(";
            child[0]->print_tree(o,deriv);
            if (child[1]) {
                o << " ";
                child[1]->print_tree(o,deriv);
            }
            o << ")";            
        }
    }
    
};

//Result::Factory::derivation_type Result::Factory::NONE=0,Result::Factory::PENDING=(Result*)0x1;

struct ResultPrinter {
    bool operator()(const Result *r,unsigned i) const {
        KBESTNESTT;
        KBESTINFOT("Visiting result #" << i << " = " << r);
        KBESTINFOT("");
        cout << "RESULT #" << i << "=" << *r << "\n";
        KBESTINFOT("done #:" << i);
        return true;
    }
};

typedef lazy_forest<Result::Factory> LK;

/*
  qe
qe -> A(qe qo) # .33 a
qe -> A(qo qe) # .33 b
qe -> B(qo) # .34 c
qo -> A(qo qo) # .25 d
qo -> A(qe qe) # .25 e
qo -> B(qe) # .25 f
qo -> C # .25 g

.25 -> 1.37
.34 -> 1.08
.33 -> 1.10
*/

inline void jonmay_cycle(int weightset=0) 
{
    using std::log;
    LK::lazy_forest qe, 
        qo;
//    float ca=1.1,cb=1.1,cc=1.08,cd=1.37,ce=1.1,cf=1.37,cg=1.37;
    /*
      ca=cb=1.1;
      cc=1.08;
      cd=ce=cf=cg=1.37;
    */
    float ca=.502,cb=.491,cc=0.152,cd=.603,ce=.502,cf=.174,cg=0.01;

    if (weightset==2) {
        ca=cb=cc=cd=ce=cf=cg=1;
    }
    if (weightset==1) {
        ca=cb=-log(.33);
        cc=-log(.34);
        cd=ce=cf=cg=-log(.25);
    }

    Result g("qo->C",cg);    
    Result c("qe->B(qo)",cc,&g);
    Result a("qe->A(qe qo)",ca,&c,&g);
    Result b("qe->A(qo qe)",cb,&g,&c);
    Result d("qo->A(qo qo)",cd,&g,&g);
    Result e("qo->A(qe qe)",ce,&c,&c);
    Result f("qo->B(qe)",cf,&c);
    

    qe.add_sorted(&c,&qo);
    qe.add_sorted(&a,&qe,&qo);
    qe.add_sorted(&b,&qo,&qe);
    MUST(qe.is_sorted());
    
    qo.add(&e,&qe);
    qo.add(&g);
    qo.add(&d,&qo,&qo);
    qo.add(&f,&qe);

    MUST(!qo.is_sorted());
    qo.sort();
    MUST(qo.is_sorted());
    
    NESTT;
    //LK::enumerate_kbest(10,&qo,ResultPrinter());
    qe.enumerate_kbest(25,ResultPrinter());
}

inline void simplest_cycle()
{
    float cc=0,cb=1,ca=.33;
    Result c("q->C",cc);
    Result b("q->B(q)",cb,&c);
    Result a("q->A(q q)",ca,&c,&c);
    LK::lazy_forest q,q2;
    q.add(&a,&q,&q);
    q.add(&b,&q);
    q.add(&c);
    MUST(!q.is_sorted());
    q.sort();
    MUST(q.is_sorted());
    NESTT;
    q.enumerate_kbest(30,ResultPrinter());
}

inline void simple_cycle()
{

    float cc=0;Result c("q->C",cc);
    float cd=.01;Result d("q2->D(q)",cd,&c);    
    float cb=.2;Result b("q->B(q2 q2)",cb,&d,&d);
    float ca=.33;Result a("q->A(q q)",ca,&c,&c);
    LK::lazy_forest q,q2;
    q.add(&a,&q,&q);
    q.add(&b,&q2,&q2);
    q.add(&c);
    MUST(!q.is_sorted());
    q.sort();
    MUST(q.is_sorted());
    q2.add_sorted(&d,&q);
    NESTT;
    q.enumerate_kbest(30,ResultPrinter());
}



inline void jongraehl_example()
{
    float eps=-1000;
    LK::lazy_forest a,b,c,f;
    float cf=-2;Result rf("f->F",cf);
    float cb=-5;Result rb("b->B",cb);
    float cfb=eps;Result rfb("f->I(b)",cfb,&rb);
    float cbf=-cf+eps;Result rbf("b->H(f)",cbf,&rf);
    float cbb=10;Result rbb("b->G(b)",cbb,&rb);
    float ccbf=8;Result rcbf("c->C(b f)",ccbf,&rb,&rf);
    float cabc=6;Result rabc("a->A(b c)",cabc,&rb,&rcbf);
    float caa=-cabc+eps;Result raa("a->Z(a)",caa,&rabc);
//    Result ra("a",6),rb("b",5),rc("c",1),rf("d",2),rb2("B",5),rb3("D",10),ra2("A",12);
    f.add_sorted(&rf); // terminal
    f.add_sorted(&rfb);
    b.add_sorted(&rb); // terminal
    b.add_sorted(&rbf,&f);
    b.add_sorted(&rbb,&b);
    c.add_sorted(&rcbf,&b,&f);
    a.add_sorted(&rabc,&b,&c);
    a.add_sorted(&raa,&a);
    MUST(a.is_sorted());
    MUST(b.is_sorted());
    MUST(c.is_sorted());
    MUST(f.is_sorted());
    NESTT;
    LK::throw_on_cycle=true;
    f.enumerate_kbest(1,ResultPrinter());
    b.enumerate_kbest(1,ResultPrinter());
    c.enumerate_kbest(1,ResultPrinter());
    a.enumerate_kbest(15,ResultPrinter());
}
}//ns
#endif

# ifdef TEST
BOOST_AUTO_UNIT_TEST(TEST_lazy_kbest) {
    using namespace lazy_forest_kbest_example;
    jongraehl_example();
    jonmay_cycle();
}
#endif


#endif
