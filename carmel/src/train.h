#ifndef CARMEL_TRAIN_H
#define CARMEL_TRAIN_H

#include <graehl/carmel/src/state.h>
#include <graehl/shared/word_spacer.hpp>
#include <graehl/shared/array.hpp>

namespace graehl {

void check_fb_agree(Weight f,Weight b);
void training_progress(unsigned train_example_no);

struct arc_counts 
{
    unsigned src;
    unsigned dest() const
    {
        return arc->dest;
    }
    int in() const
    {
        return arc->in;
    }
    int out() const
    {
        return arc->out;
    }
    int groupId() const 
    {
        return arc->groupId;
    }
    
    FSTArc *arc;
    Weight scratch;
    Weight em_weight;
    Weight best_weight;
    Weight counts;
    Weight prior_counts;
    Weight &weight() const 
    {
        return arc->weight;
    }
};

std::ostream& operator << (std::ostream &o,arc_counts const& ac);

    

struct DWPair {    
    unsigned dest; // to allow reverse forward = backward version
    unsigned id; // to arc_counts table
    DWPair(DWPair const& o) : dest(o.dest),id(o.id) {}
    DWPair() {}
    DWPair(unsigned dest,unsigned id) : dest(dest),id(id) {}
};

std::ostream & operator << (std::ostream &o, DWPair p);


std::ostream & hashPrint(HashTable<IOPair, List<DWPair> > &h, std::ostream &o);

struct symSeq {
    int n;
    int *let;
    int *rLet;
    typedef int* iterator;
    typedef int*const_iterator;
    iterator begin() const
    {
        return let;
    }
    iterator end() const
    {
        return let+n;
    }
    iterator rbegin() const
    {
        return rLet;
    }
    iterator rend() const
    {
        return rLet+n;
    }
    unsigned size() const
    {
        return n;
    }
    template <class O,class Alphabet>
    void print(O &o,Alphabet const& a) const
    {
        word_spacer sp;
        for (int i=0;i<n;++i)
            o << sp << a[let[i]];
    }
};

std::ostream & operator << (std::ostream & out , const symSeq & s);

struct IOSymSeq {
    symSeq i;
    symSeq o;
    FLOAT_TYPE weight;
    IOSymSeq(List<int> const&inSeq, List<int> const&outSeq, FLOAT_TYPE w) 
    {
        init(inSeq,outSeq,w);
    }
    template <class S>
    void init(S const&inSeq, S const&outSeq, FLOAT_TYPE w) {
        i.n = inSeq.size();
        o.n = outSeq.size();
        i.let = NEW int[i.n];
        o.let = NEW int[o.n];
        i.rLet = NEW int[i.n];
        o.rLet = NEW int[o.n];
        int *pi, *rpi;
        pi = i.let;
        rpi = i.rLet + i.n;
        for ( typename S::const_iterator inL=inSeq.begin(),endI=inSeq.end() ; inL != endI ; ++inL )
            *pi++ = *--rpi = *inL;
        pi = o.let;
        rpi = o.rLet + o.n;
        for ( typename S::const_iterator outL=outSeq.begin(),endO=outSeq.end() ; outL != endO ; ++outL )
            *pi++ = *--rpi = *outL;
        weight = w;
    }
    void kill() {
        if (o.let) {
            delete[] o.let;
            delete[] o.rLet;
            delete[] i.let;
            delete[] i.rLet;
            o.let=NULL;
        }
    }
    IOSymSeq(IOSymSeq const& o) 
    {
        init(o.i,o.o,o.weight);
    }
    
    ~IOSymSeq() 
    {
        kill();
    }
    
    template <class O,class Alphabet>
    void print(O &os,Alphabet const& in,Alphabet const& out,char const* term="\n") const
    {
        i.print(os,in);
        os << term;
        o.print(os,out);
        os << term;
    }
    template <class O,class FST>
    void print(O &o,FST const& x,char const* term="\n") const
    {
        print(o,x.in_alph(),x.out_alph(),term);
    }
    
};

std::ostream & operator << (std::ostream & out , const IOSymSeq & s);   // Yaser 7-21-2000

class training_corpus : boost::noncopyable
{
 public:
    training_corpus() { clear(); }
    void clear()
    {
        maxIn=maxOut=0;
        n_input=n_output=0;
        w_input=w_output=totalEmpiricalWeight=0;
        examples.clear();
    }
    
    void add(List<int> &inSeq, List<int> &outSeq, FLOAT_TYPE weight=1.) 
    {
        examples.push_front(inSeq,outSeq,weight);
        IOSymSeq const& n=examples.front();
        unsigned i=n.i.n,o=n.o.n;
        n_input += i;
        n_output += o;
        w_input += weight*i;
        w_output += weight*o;
        if (maxIn<i) maxIn=i;
        if (maxOut<o) maxOut=o;
        totalEmpiricalWeight += weight;
    }
    void finish_adding() 
    {
        examples.reverse();
    }
    void set_null()
    {
        clear();
        List<int> empty_list;
        add(empty_list, empty_list, 1.0);
    }    

//    bool cache_derivations;
    unsigned maxIn, maxOut; // highest index (N-1) of input,output symbols respectively.
    List <IOSymSeq> examples;
    //Weight smoothFloor;
    FLOAT_TYPE totalEmpiricalWeight; // # of examples, if each is weighted equally
    FLOAT_TYPE n_input,n_output,w_input,w_output; // for per-symbol ppx.  w_ is multiplied by example weight.  n_ is unweighted
};

}//ns



#endif
