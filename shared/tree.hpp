// the usual recursive label+list of children trees
#ifndef GRAEHL_SHARED__TREE_HPP
#define GRAEHL_SHARED__TREE_HPP

#define GRAEHL_TREE_UNARY_OPTIMIZATION
namespace graehl {
typedef short rank_type; // (rank=#children) -1 = any rank, -2 = any tree ... (can't be unsigned type)
}

#ifndef SATISFY_VALGRIND
//FIXME: make SATISFY_VALGRIND + GRAEHL_TREE_UNARY_OPTIMIZATION compile
// program is still correct w/o this defined, but you get bogus valgrind warnings if not
//# define SATISFY_VALGRIND
#endif

#include <iostream>
#include <graehl/shared/myassert.h>
#include <graehl/shared/genio.h>
#include <graehl/shared/word_spacer.hpp>
//#include <vector>
#include <graehl/shared/dynarray.h>
#include <algorithm>
#ifdef USE_LAMBDA
#include <boost/lambda/lambda.hpp>
namespace lambda=boost::lambda;
#endif
#include <functional>
//#include <graehl/shared/config.h>
#include <graehl/shared/graphviz.hpp>
//#include "symbol.hpp"
#include <graehl/shared/byref.hpp>
#include <graehl/shared/stream_util.hpp>

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#include <graehl/shared/string_to.hpp>

#endif


//template <class L, class A> struct Tree;

// Tree owns its own child pointers list but not trees it points to! - routines for creating trees through new and recurisvely deleting are provided outside the class.  Reading from a stream does create children using new.  finish with dealloc_recursive()
// alloc builds child vectors on heap.
// FIXME: need two allocators (or always rebind/copy from one) instead of just new/deleting self_type ... that is, shared_tree nodes are coming off usual heap, but we act interested in how we get the child pointer vectors.  really, if you care about one, you probably care about the other.
namespace graehl {

template <class L, class Alloc=std::allocator<void *> > struct shared_tree : private Alloc {
    typedef shared_tree self_type;
    typedef L Label;
    Label label;
    rank_type rank;
  friend inline void swap(self_type &a,self_type &b) {
    using namespace std;
    swap(a.label,b.label);
    rank_type r=b.rank;b.rank=a.rank;a.rank=r;
    self_type **children=b.c.children;b.c.children=a.c.children;a.c.children=children;
//    swap(a.rank,b.rank);
//    swap(a.c.children,b.c.children); //FIXME: diff pointer types? then detect based on rank
  }

 protected:
    //shared_tree(const self_type &t) : {}
    union children_or_child
    {
        self_type **children;
        self_type *child;
    };


    children_or_child c;

 public:
    bool is_leaf() const
    {
        return rank==0;
    }

    template <class T>
    struct related_child
    {
        self_type dummy;
        T data;
        related_child() {}
    };


    template<class T>
    T &leaf_data() {
        Assert(rank==0);
        Assert(sizeof(T) <= sizeof(c.children));
//        return *(reinterpret_cast<T*>(&children));
        return *(T *)(related_child<T>*)&c.child;
        // C99 aliasing is ok with this because we cast to a struct that relates T* and self_type*, or so I'm told
    }
    template<class T>
    const T &leaf_data() const {
        return const_cast<self_type *>(this)->leaf_data<T>();
    }
    /*
      int &leaf_data_int() {
      Assert(rank==0);
      return *(reinterpret_cast<int *>(&children));
      }
      void * leaf_data() const {
      return const_cast<self_type *>(this)->leaf_data();
      }
      int leaf_data_int() const {
      return const_cast<self_type *>(this)->leaf_data();
      }
    */
    rank_type size() const {
        return rank;
    }
    shared_tree() : rank(0)
#ifdef SATISFY_VALGRIND
                  , children(0)
#endif
    {}
    explicit shared_tree (const Label &l) : label(l),rank(0)
#ifdef SATISFY_VALGRIND
                  , children(0)
#endif

    {  }
    shared_tree (const Label &l,rank_type n) : label(l) { alloc(n); }
    explicit shared_tree (std::string const& s) : rank(0)
#ifdef SATISFY_VALGRIND
                  , children(0)
#endif

    {
        std::istringstream ic(s);ic >> *this;
    }
    void alloc(rank_type _rank) {
        rank=_rank;
#ifdef GRAEHL_TREE_UNARY_OPTIMIZATION
        if (rank>1)
#else
            if (rank)
#endif
                c.children = (self_type **)this->allocate(rank);
#ifdef SATISFY_VALGRIND
            else
                children=0;
#endif
    }
    template <class to_type>
    void copy_deep(to_type &to) const
    {
        to.clear();
        to.label=label;
        to.alloc(rank);
        iterator t=to.begin();
        for (const_iterator i=begin(),e=end();i!=e;++i,++t) {
            (*i)->copy_deep(*(*t=new to_type));
        }
    }

    shared_tree(rank_type _rank) {
        alloc(_rank);
    }
    void dump_children() {
        dealloc();
    }
    void create_children(unsigned n) {
        alloc(n);
    }
    void set_rank(unsigned n) {
        dealloc();
        alloc(n);
    }
    void clear()
    {
        dealloc();
    }
    void dealloc() {
#ifdef GRAEHL_TREE_UNARY_OPTIMIZATION
        if (rank>1)
#else
            if (rank)
#endif
                this->deallocate((void **)c.children,rank);
        rank=0;
    }
    void dealloc_recursive();

    ~shared_tree() {
        dealloc();
    }
    // STL container stuff
    typedef self_type *value_type;
    typedef value_type *iterator;

    typedef const self_type *const_value_type;
    typedef const const_value_type *const_iterator;

    value_type & child(rank_type i) {
#ifdef GRAEHL_TREE_UNARY_OPTIMIZATION
        if (rank == 1) {
            Assert(i==0);
            return c.child;
        }
#endif
        return c.children[i];
    }

    value_type & child(rank_type i) const {
        return const_cast<self_type*>(this)->child(i);
    }

    value_type & operator [](rank_type i) { return child(i); }
    value_type & operator [](rank_type i) const { return child(i); }

    iterator begin() {
#ifdef GRAEHL_TREE_UNARY_OPTIMIZATION
        if (rank == 1)
            return &c.child;
        else
#endif
            return c.children;
    }
    iterator end() {
#ifdef GRAEHL_TREE_UNARY_OPTIMIZATION
        if (rank == 1)
            return &c.child+1;
        else
#endif
            return c.children+rank;
    }
    const_iterator begin() const {
        return const_cast<self_type *>(this)->begin();
    }
    const_iterator end() const {
        return const_cast<self_type *>(this)->end();
    }
    template <class T>
    friend size_t tree_count(const T *t);
    template <class T>
    friend size_t tree_height(const T *t);

    //height = maximum length path from root
    size_t height() const
    {
        return tree_height(this);
    }

    size_t count_nodes() const
    {
        return tree_count(this);
    }
    template <class charT, class Traits>
    std::ios_base::iostate print(std::basic_ostream<charT,Traits>& o,bool lisp_style=true) const
    {
        if (!lisp_style || !rank)
            o << label;
        if (rank) {
            o << '(';
            word_spacer sp;
            if (lisp_style)
                o << sp << label;
            for (const_iterator i=begin(),e=end();i!=e;++i) {
                o << sp;
                (*i)->print(o,lisp_style);
            }
            o << ')';
        }
        return GENIOGOOD;
    }

    template <class O,class Writer>
    void print_writer(O& o,Writer const& w,bool lisp_style=true) const
    {
        if (!lisp_style || !rank)
            w(o,label);
        if (rank) {
            o << '(';
            word_spacer sp;
            if (lisp_style) {
                o << sp;
                w(o,label);
            }
            for (const_iterator i=begin(),e=end();i!=e;++i) {
                o << sp;
                (*i)->print_writer(o,w,lisp_style);
            }
            o << ')';
        }
    }

    /*
      template <class charT, class Traits, class Writer>
      std::ios_base::iostate print_graphviz(std::basic_ostream<charT,Traits>& o,Writer writer, const char *treename="T") const {
      o << "digraph ";
      out_quote(o,treename);
      o << " {\n";
      print_graphviz_rec(o,writer,0);
      o << "}\n";
      return GENIOGOOD;
      }
      template <class charT, class Traits, class Writer>
      void print_graphviz_rec(std::basic_ostream<charT,Traits>& o,Writer writer,unsigned node_no) const {
      for (const_iterator i=begin(),e=end();i!=e;++i) {
      i->print_graphviz_rec(o,writer,++node_no);
      }
      }
    */

    typedef int style_type;
    BOOST_STATIC_CONSTANT(style_type,UNKNOWN=0);
    BOOST_STATIC_CONSTANT(style_type,PAREN_FIRST=1);
    BOOST_STATIC_CONSTANT(style_type,HEAD_FIRST=2);

    //template <class T, class charT, class Traits>  friend
    template <class charT, class Traits, class Reader>
    static self_type *read_tree(std::basic_istream<charT,Traits>& in,Reader r,style_type style=UNKNOWN) {
        self_type *ret = new self_type;
        std::ios_base::iostate err = std::ios_base::goodbit;
        if (ret->read(in,r,style))
            in.setstate(err);
        if (!in.good()) {
            ret->dealloc_recursive();
            return NULL;
        }
        return ret;
    }

    template <class charT, class Traits>
    static self_type *read_tree(std::basic_istream<charT,Traits>& in) {
        return read_tree(in,DefaultReader<Label>());
    }

//    template <class T> friend void delete_tree(T *);

#ifdef DEBUG_TREEIO
#define DBTREEIO(a) DBP(a)
#else
#define DBTREEIO(a)
#endif

    // this method is supposed to recognize two types of trees: one where the initial is (head ....) and the other which is head(...) ... (1 (2 3)) vs. 1(2 3) - note that this is shorthand for (1 ((2) (3))) vs. 1(2() 3()) (leaves have 0 children).  also, comma is allowed a separator as well as space.  this means that labels can't contain '(),' ... also, we don't like 1 (2 3) ... this would parse as the tree 1, not 1(2 3) (i.e. no space between label and '(')
    // Reader passed by value, so can't be stateful (unless itself is a pointer to shared state)
    template <class charT, class Traits, class Reader>
    std::ios_base::iostate read(std::basic_istream<charT,Traits>& in,Reader read,style_type style=UNKNOWN)
    {
        char c;
        rank=0;
        dynamic_array<self_type *> in_children;
        try {
        EXPECTI_COMMENT_FIRST(in>>c);
        if (c == '(') {
            if (style==HEAD_FIRST)
                return GENIOBAD;
            style=PAREN_FIRST;
            EXPECTI_COMMENT_FIRST(deref(read)(in,label));
            DBTREEIO(label);
        } else {
            in.unget();
            EXPECTI_COMMENT_FIRST(deref(read)(in,label));
            DBTREEIO(label);
            if (style==PAREN_FIRST) { // then must be leaf
                goto good;
            } else
                style=HEAD_FIRST;
            I_COMMENT(in.get(c));
            // to disambiguate (1 (2) 3 (4)) vs. 1( 2 3(4)) ... should be (1 2 3 4) and not (1 2 (3 4)) ... in other words open paren must follow root label.  but now we allow space by fixing style at root
            if (in.eof()) // since c!='(' before in>>c, can almost not test for this - but don't want to unget() if nothing was read.
                goto good;
            if (!in)
                goto fail;
            if (c!='(') {
                in.unget();
                goto good;
            }
        } //POST: read a '(' and a label (in either order)
        DBTREEIO('(');
        for(;;) {
            EXPECTI_COMMENT(in>>c);
            if (c == ',')
                EXPECTI_COMMENT(in>>c);
            if (c==')') {
                DBTREEIO(')');
                break;
            }
            in.unget();
            self_type *in_child = read_tree(in,read,style);
            if (in_child) {
                DBTREEIO('!');
                in_children.push_back(in_child);
            } else
                goto fail;
        }
        dealloc();
        alloc((rank_type)in_children.size());
        //copy(in_children.begin(),in_children.end(),begin());
        in_children.moveto(begin());
        goto good;
        } catch(...) {
            goto fail;
        }

    fail:
        for (typename dynamic_array<self_type *>::iterator i=in_children.begin(),end=in_children.end();i!=end;++i)
            (*i)->dealloc_recursive();
        return GENIOBAD;
    good:
        DBTREEIO(*this);
        return GENIOGOOD;
    }
    TO_OSTREAM_PRINT

    template <class charT, class Traits>
    std::ios_base::iostate read(std::basic_istream<charT,Traits>& in)
    {
        return read(in,DefaultReader<L>());
    }
    void set_no_children()
    {
        for (iterator i=begin(),e=end();i!=e;++i)
            *i=NULL;
    }

    FROM_ISTREAM_READ
};

template <class L, class Alloc=std::allocator<void *> > struct tree : public shared_tree<L,Alloc> {
    typedef tree self_type;
    typedef shared_tree<L,Alloc> shared;

    tree()  {}
    explicit tree(L const& l) : shared(l) {}
    explicit tree(std::string const& c) : shared(c) {}
    tree(shared const& o)
    {
        o.copy_deep(*this);
    }
  tree(shared const& o,bool /*move construct*/) {
    swap(*(shared *)this,o);
  }

    tree(self_type const& o)
    {
        o.copy_deep(*this);
    }
    tree(rank_type rank) : shared(rank) { this->set_no_children(); }
    tree(L const&l,rank_type r) : shared(l,r) { this->set_no_children(); }
    ~tree() { clear(); }
    typedef self_type *value_type;

    typedef value_type *iterator;
    typedef const self_type *const_value_type;
    typedef const const_value_type *const_iterator;

    void operator=(self_type &o)
    {
        o.clear();
        this->copy_deep(o);
    }

    value_type &child(rank_type i)
    {
        return (value_type &)shared::child(i);
    }
    value_type const&child(rank_type i) const
    {
        return (value_type const&)shared::child(i);
    }
    iterator begin()
    {
        return (iterator)shared::begin();
    }
    iterator end()
    {
        return (iterator)shared::end();
    }
    const_iterator begin() const
    {
        return (const_iterator)shared::begin();
    }
    const_iterator end() const
    {
        return (const_iterator)shared::end();
    }

    void clear()
    {
        for (iterator i=begin(),e=end();i!=e;++i)
            if (*i)
                delete *i;
        this->dealloc();
    }

    TO_OSTREAM_PRINT
    template <class charT, class Traits, class Reader>
    std::ios_base::iostate read(std::basic_istream<charT,Traits>& in,Reader read)
    {
        clear();
        return shared::read(in,read);
    }
    template <class charT, class Traits>
    std::ios_base::iostate read(std::basic_istream<charT,Traits>& in)
    {
        return read(in,DefaultReader<L>());
    }

    FROM_ISTREAM_READ

};




template <class C>
void delete_arg(C
#ifndef DEBUG
                const
#endif
                &c) {
  delete c;
#ifdef DEBUG
  c=NULL;
#endif
}

template <class T,class F>
bool tree_visit(T *tree,F func)
{
    if (!deref(func).discover(tree))
        return false;
    for (typename T::iterator i=tree->begin(), end=tree->end(); i!=end; ++i)
        if (!tree_visit(*i,func))
            break;
    return deref(func).finish(tree);
}

template <class T,class F>
void tree_leaf_visit(T *tree,F func)
{
    if (tree->size()) {
        for (T *child=tree->begin(), *end=tree->end();child!=end;++child) {
            tree_leaf_visit(child,func);
        }
    } else {
        deref(func)(tree);
    }
}


template <class Label,class Labeler=DefaultNodeLabeler<Label> >
struct TreeVizPrinter : public GraphvizPrinter {
    Labeler labeler;
    typedef shared_tree<Label> T;
    unsigned samerank;
    enum make_not_anon_24 {ANY_ORDER=0,CHILD_ORDER=1,CHILD_SAMERANK=2};


    TreeVizPrinter(std::ostream &o_,unsigned samerank_=CHILD_SAMERANK,const std::string &prelude="",const Labeler &labeler_=Labeler(),const char *graphname="tree") : GraphvizPrinter(o_,prelude,graphname), labeler(labeler_),samerank(samerank_) {}
    void print(const T &t) {
        print(t,next_node++);
        o << std::endl;
    }
    void print(const T &t,unsigned parent) {
        o << " " << parent << " [";
        labeler.print(o,t.label);
        o << "]\n";
        if (t.rank) {
            unsigned child_start=next_node;
            next_node+=t.rank;
            unsigned child_end=next_node;
            unsigned child=child_start+1;
            if (samerank!=ANY_ORDER)
                if (t.rank > 1) { // ensure left->right order
                    o << " {";
                    if (samerank==CHILD_SAMERANK)
                        o << "rank=same ";
                    o << child_start;
                    for (;child != child_end;++child)
                        o << " -> " << child;
                    o << " [style=invis,weight=0.01]}\n";
                }
            child=child_start;
            for (typename T::const_iterator i=t.begin(),e=t.end();i!=e;++i,++child) {
                o << " " << parent << " -> " << child;
                if (samerank==ANY_ORDER) {
                    //                    o << "[label=" << i-t.begin() << "]";
                }
                o << "\n";
            }
            child=child_start;
            for (typename T::const_iterator i=t.begin(),e=t.end();i!=e;++i,++child) {
                print(**i,child);
            }
        }
    }
};

struct TreePrinter {
  std::ostream &o;
  bool first;
  TreePrinter(std::ostream &o_):o(o_),first(true) {}
template <class T>
  bool discover(T *t) {
   if (!first) o<<' ';
   o<<t->label;
   if (t->size()) { o << '('; first=true; }
   return true;
  }
template <class T>
  bool finish(T *t) {
   if (t->size())
    o << ')';
   first=false;
   return true;
  }
};

template <class T,class F>
void postorder(T *tree,F func)
{
  Assert(tree);
  for (typename T::iterator i=tree->begin(), end=tree->end(); i!=end; ++i)
        postorder(*i,func);
  deref(func)(tree);
}

template <class T,class F>
void postorder(const T *tree,F func)
{
  Assert(tree);
  for (typename T::const_iterator i=tree->begin(), end=tree->end(); i!=end; ++i)
        postorder(*i,func);
  deref(func)(tree);
}


template <class L,class A>
void delete_tree(shared_tree<L,A> *tree)
{
  Assert(tree);
    tree->dealloc_recursive();
//  postorder(tree,delete_arg<shared_tree<L,A> *>);
}

template <class L,class A>
void shared_tree<L,A>::dealloc_recursive() {
        for(iterator i=begin(),e=end();i!=e;++i)
            (*i)->dealloc_recursive();
            //delete_tree(*i);
        //foreach(begin(),end(),delete_tree<self_type>);
        dealloc();
  }

template <class T1,class T2>
bool tree_equal(const T1& a,const T2& b)
{
  if (a.size() != b.size() ||
        !(a.label == b.label))
        return false;
  typename T2::const_iterator b_i=b.begin();
  for (typename T1::const_iterator a_i=a.begin(), a_end=a.end(); a_i!=a_end; ++a_i,++b_i)
        if (!(tree_equal(**a_i,**b_i)))
          return false;
  return true;
}

template <class T1,class T2,class P>
bool tree_equal(const T1& a,const T2& b,P equal)
{
  if ( (a.size()!=b.size()) || !equal(a,b) )
        return false;
  typename T2::const_iterator b_i=b.begin();
  for (typename T1::const_iterator a_i=a.begin(), a_end=a.end(); a_i!=a_end; ++a_i,++b_i)
        if (!(tree_equal(**a_i,**b_i,equal)))
          return false;
  return true;
}

template <class T1,class T2>
struct label_equal_to {
  bool operator()(const T1&a,const T2&b) const {
        return a.label == b.label;
  }
};

template <class T1,class T2>
bool tree_contains(const T1& a,const T2& b)
{
  return tree_contains(a,b,label_equal_to<T1,T2>());
}

template <class T1,class T2,class P>
bool tree_contains(const T1& a,const T2& b,P equal)
{
  if ( !equal(a,b) )
        return false;
  // leaves of b can match interior nodes of a
  if (!b.size())
        return true;
  if( a.size()!=b.size())
        return false;
  typename T1::const_iterator a_i=a.begin();
  for (typename T2::const_iterator b_end=b.end(), b_i=b.begin(); b_i!=b_end; ++a_i,++b_i)
        if (!(tree_contains(**a_i,**b_i,equal)))
          return false;
  return true;
}


template <class L,class A>
inline bool operator !=(const shared_tree<L,A> &a,const shared_tree<L,A> &b)
{
  return !(a==b);
}

template <class L,class A>
inline bool operator ==(const shared_tree<L,A> &a,const shared_tree<L,A> &b)
{
  return tree_equal(a,b);
}



template <class T>
struct TreeCount {
  size_t count;
  void operator()(const T * tree) {
        ++count;
  }
  TreeCount() : count(0) {}
};

#include <boost/ref.hpp>
template <class T>
size_t tree_count(const T *t)
{
  TreeCount<T> n;
  postorder(t,boost::ref(n));
  return n.count;
}

//height = maximum length path from root
template <class T>
size_t tree_height(const T *tree)
{
    Assert(tree);
  if (!tree->size())
        return 0;
  size_t max_h=0;
  for (typename T::const_iterator i=tree->begin(), end=tree->end(); i!=end; ++i) {
        size_t h=tree_height(*i);
        if (h>=max_h)
          max_h=h;
  }
  return max_h+1;
}

template <class T,class O>
struct Emitter {
  O &out;
  Emitter(O &o) : out(o) {}
  void operator()(T * tree) {
        out << tree;
  }
};



template <class T,class O>
void emit_postorder(const T *t,O out)
{
#ifndef USE_LAMBDA
  Emitter<T,O &> e(out);
  postorder(t,e);
#else
  postorder(t,o << boost::lambda::_1);
#endif
}


template <class L>
tree<L> *new_tree(const L &l) {
  return new tree<L> (l,0);
}

template <class L>
tree<L> *new_tree(const L &l, tree<L> *c1) {
  tree<L> *ret = new tree<L>(l,1);
  (*ret)[0]=c1;
  return ret;
}

template <class L>
tree<L> *new_tree(const L &l, tree<L> *c1, tree<L> *c2) {
  tree<L> *ret = new tree<L>(l,2);
  (*ret)[0]=c1;
  (*ret)[1]=c2;
  return ret;
}

template <class L>
tree<L> *new_tree(const L &l, tree<L> *c1, tree<L> *c2, tree<L> *c3) {
  tree<L> *ret = new tree<L>(l,3);
  (*ret)[0]=c1;
  (*ret)[1]=c2;
  (*ret)[2]=c3;
  return ret;
}

template <class L,class charT, class Traits>
tree<L> *read_tree(std::basic_istream<charT,Traits>& in)
{
    tree<L> *ret = new tree<L>;
    in >> *ret;
    return ret;
  //return tree<L>::read_tree(in,DefaultReader<L>());
  //return tree<L>::read_tree(in);
}

#ifdef GRAEHL_TEST

template<class T> bool always_equal(const T& a,const T& b) { return true; }

BOOST_AUTO_TEST_CASE( test_tree )
{
    using namespace std;
  tree<int> a,b,*c,*d,*g=new_tree(1),*h;
  //string sa="%asdf\n1(%asdf\n 2 %asdf\n,%asdf\n3 (4\n ()\n,\t 5,6))";
  string sa="%asdf\n1%asdf\n(%asdf\n 2 %asdf\n,%asdf\n3 (4\n ()\n,\t 5,6))";
  //string sa="1(2 3(4 5 6))";
  string streeprint="1(2 3(4 5 6))";
  string sb="(1 2 (3 4 5 6))";
  stringstream o;
  istringstream isa(sa);
  isa >> a;
  o << a;
  BOOST_CHECK_EQUAL(o.str(),sb);
  o >> b;
  ostringstream o2;
  TreePrinter tp(o2);
  tree_visit(&a,boost::ref(tp));
  BOOST_CHECK_EQUAL(streeprint,o2.str());
  c=new_tree(1,
                new_tree(2),
                new_tree(3,
                  new_tree(4),new_tree(5),new_tree(6)));
  d=new_tree(1,new_tree(2),new_tree(3));
  istringstream isb(sb);
  h=read_tree<int>(isb);
  //h=Tree<int>::read_tree(istringstream(sb),DefaultReader<int>());
  BOOST_CHECK(a == a);
  BOOST_CHECK_EQUAL(a,b);
  BOOST_CHECK_EQUAL(a,*c);
  BOOST_CHECK_EQUAL(a,*h);
  BOOST_CHECK_EQUAL(a.count_nodes(),6);
  BOOST_CHECK(tree_equal(a,*c,always_equal<tree<int> >));
  BOOST_CHECK(tree_contains(a,*c,always_equal<tree<int> >));
  BOOST_CHECK(tree_contains(a,*d,always_equal<tree<int> >));
  BOOST_CHECK(tree_contains(a,*d));
  BOOST_CHECK(tree_contains(a,b));
  BOOST_CHECK(!tree_contains(*d,a));
  tree<int> e("1(1(1) 1(1,1,1))"),
                f("1(1(1()),1)");
  BOOST_CHECK(!tree_contains(a,e,always_equal<tree<int> >));
  BOOST_CHECK(tree_contains(e,a,always_equal<tree<int> >));
  BOOST_CHECK(!tree_contains(f,e));
  BOOST_CHECK(tree_contains(e,f));
  BOOST_CHECK(tree_contains(a,*g));
  BOOST_CHECK(e.height()==2);
  BOOST_CHECK(f.height()==2);
  BOOST_CHECK_EQUAL(a.height(),2);
  BOOST_CHECK(g->height()==0);
  delete c;
  delete d;
  delete h;
  delete g;

//  delete_tree(c);
//  delete_tree(d);
//  delete_tree(g);
//  delete_tree(h);
  tree<int> k("1");
  tree<int> l("1()");
  BOOST_CHECK(tree_equal(k,l));
  BOOST_CHECK(k.rank==0);
  BOOST_CHECK(l.rank==0);
  BOOST_CHECK(l.label==1);
  string s1="(1 2 (3))";
  string s2="(1 2 3)";
  tree<int> t1,t2;
  string_to(s1,t1);
  string_to(s2,t2);
  BOOST_CHECK_EQUAL(s2,to_string(t2));
  BOOST_CHECK_EQUAL(t1,t2);
  BOOST_CHECK(tree_equal(t1,t2));
}


#endif
}

#endif
