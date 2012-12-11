// linked list wrapper (uses default STL list instead unless -DUSE_SLIST)
#ifndef GRAEHL_SHARED_LIST_H
#define GRAEHL_SHARED_LIST_H
#include <graehl/shared/config.h>
#include <graehl/shared/stream_util.hpp>
#include <graehl/shared/word_spacer.hpp>
#include <iterator>
#include <graehl/shared/simple_serialize.hpp>
#include <graehl/shared/container.hpp>

#ifdef USE_SLIST
#include <graehl/shared/slist.h>
#define STL_LIST slist
#else
#include <list>
#define STL_LIST std::list
#endif

#include <iostream>

namespace graehl {


template <class T,class A=std::allocator<T> >
class List : public STL_LIST<T,A> {
  //
    typedef STL_LIST<T,A> S;
public:
    bool has2() const
    {
        const_iterator b=begin(),e=end();
        return b!=e&&++b!=e;
    }

    typedef List<T,A> self_type;
    void swap(self_type &b)
    {
        S::swap(b);
    }

    inline void friend swap(self_type &a,self_type &b)
    {
        a.swap(b);
    }

    typedef typename S::const_iterator const_iterator;
#ifdef USE_SLIST
    typedef typename S::erase_iterator iterator;
    iterator begin()
    {
        return S::erase_begin();
    }
    iterator end()
    {
        return S::erase_end();
    }
    const_iterator begin() const
    {
        return S::const_begin();
    }
    const_iterator end() const
    {
        return S::const_end();
    }
    typedef typename S::back_insert_iterator back_insert_iterator;
#else
    template <class A>
    void serialize(A &a)
    {
        serialize_container(a,(S &)*this);
    }

    typedef typename List::iterator erase_iterator;
    typedef typename List::iterator val_iterator;
    typename List::const_iterator const_begin() const { return this->begin(); } //{ return const_cast<const List *>(this)->begin(); }
    typename List::const_iterator const_end() const { return this->end(); } //{ return const_cast<const List *>(this)->end(); }
    typename List::iterator val_begin() { return this->begin(); }
    typename List::iterator val_end() { return this->end(); }
    typename List::iterator erase_begin() { return this->begin(); }
    typename List::iterator erase_end() { return this->end(); }
    struct back_insert_iterator : public std::insert_iterator<List>
    {
        explicit back_insert_iterator(List &l) : std::insert_iterator<List>(l,l.end())
        {}
    };
#endif
    back_insert_iterator back_inserter()
    {
        return back_insert_iterator(*this);
    }

  //constructors
  List():STL_LIST<T,A>(){};
  List(const List &l):STL_LIST<T,A> (l){};
  ~List(){};
  List(const T &it):
#ifdef USE_SLIST
    STL_LIST<T,A>(it)
#else
    STL_LIST<T,A>(1,it)
#endif
    {  }
  List(size_t sz,const T &it):STL_LIST<T,A>(sz,it) {  }
  int notEmpty() const { return !isEmpty(); }
  int isEmpty() const { return this->empty(); }
  void push(const T &it) {
    this->push_front(it);
  }

  T &top() {
    return this->front();
  }
  void pop() {
    this->pop_front();
  }
#ifndef USE_SLIST

    template <class T0>
    inline void push_front(T0 const& t0)
    {
        S::push_front(t0);
    }

    template <class T0,class T1>
    inline void push_front(T0 const& t0,T1 const& t1)
    {
        push_front(T(t0,t1));
    }
    template <class T0,class T1,class T2>
    inline void push_front(T0 const& t0,T1 const& t1,T2 const& t2)
    {
        push_front(T(t0,t1,t2));
    }
    template <class T0,class T1,class T2,class T3>
    inline void push_front(T0 const& t0,T1 const& t1,T2 const& t2,T3 const& t3)
    {
        push_front(T(t0,t1,t2,t3));
    }
    template <class T0,class T1,class T2,class T3,class T4>
    inline void push_front(T0 const& t0,T1 const& t1,T2 const& t2,T3 const& t3,T4 const& t4)
    {
        push_front(T(t0,t1,t2,t3,t4));
    }
    template <class T0,class T1,class T2,class T3,class T4,class T5>
    inline void push_front(T0 const& t0,T1 const& t1,T2 const& t2,T3 const& t3,T4 const& t4,T5 const& t5)
    {
        push_front(T(t0,t1,t2,t3,t4,t5));
    }

    template <class O> void print(O&o) const
    {
        o << "(";
        word_spacer sp;
        for( const_iterator n=const_begin(),e=const_end();n!=e;++n) {
            o << sp << *n;
        }
        o << ")";
    }
    TO_OSTREAM_PRINT
#endif
};

struct GListS {
  template <class T> struct container {
    typedef List<T> type;
  };
};

/*template <class T,class A>
back_insert_iterator<List<T,A> > back_inserter(List<T,A> &l)
{
    return back_insert_iterator<List<T,A> >(l);
}
*/


/*
  template <typename T>
  class ListPostInserter {
  typedef List<T,A> L;
  typedef L::iterator Lit;
  L &l;
  Lit lit;
  public:
  explicit ListPostInserter(L &l_) : l(l_) {

  }
  ListPostInserter(L &l_,Lit &lit_) : l(l_), lit(lit_) {    }

  };
*/
#ifdef USE_SLIST
//#define LIST_BACK_INSERTER slist_back_insert_iterator
//#include <iterator>
/*
  template <class T>
  class slist_back_insert_iterator
  // : public std::iterator<output_iterator_tag,T>
  {

  //typedef std::slist<T,A> L;
  typedef typename T L;
  typedef typename T::iterator Lit;
  typedef T _Container;
  protected:
  L * container;
  Lit cursor;
  public:
  typedef L container_type;
  //  typedef output_iterator_tag iterator_category;

  explicit slist_back_insert_iterator(_Container& __x) : container(&__x), cursor(__x.previous(__x.end())) {}
  slist_back_insert_iterator(L& __x,Lit c) : container(&__x), cursor(c) {}
  slist_back_insert_iterator<T,A>&
  operator=(const typename _Container::value_type & __val) {
  cursor = container->insert_after(cursor,__val);
  return *this;
  }
  slist_back_insert_iterator<L>& operator*() { return *this; }
  slist_back_insert_iterator<L>& operator++() { return *this; }
  slist_back_insert_iterator<L>& operator++(int) { return *this; }
  };*/
#else
            //#define LIST_BACK_INSERTER back_insert_iterator
#endif

}

namespace std
{
#ifdef USE_SLIST
template <class T,class A >
    struct back_insert_iterator<graehl::List<T,A> > : public graehl::List<T,A>::back_insert_iterator
{
    typedef graehl::List<T,A> list_type;
    typedef back_insert_iterator<list_type > self_type;
    typedef typename list_type::back_insert_iterator parent_type;

    back_insert_iterator(list_type &l) : parent_type(l) {}
/* // i think both may be autogenerated:
  back_insert_iterator(self_type const& o) : parent_type(o) {}
    self_type &operator=(self_type const& o)
    {
        parent_type::operator=(o);
        return *this;
    }
*/
    };
#endif
}


#endif
