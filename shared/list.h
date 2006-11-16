// linked list wrapper (uses default STL list instead unless -DUSE_SLIST)
#ifndef LIST_H
#define LIST_H
#include <graehl/shared/config.h>
#include <graehl/shared/stream_util.hpp>

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
    typedef List<T,A> self_type;
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
    
#else 
    typedef typename List::iterator erase_iterator;
    typedef typename List::iterator val_iterator;
    typename List::const_iterator const_begin() const { return this->begin(); } //{ return const_cast<const List *>(this)->begin(); }
    typename List::const_iterator const_end() const { return this->end(); } //{ return const_cast<const List *>(this)->end(); }
    typename List::iterator val_begin() { return this->begin(); }
    typename List::iterator val_end() { return this->end(); }
    typename List::iterator erase_begin() { return this->begin(); }
    typename List::iterator erase_end() { return this->end(); }
#endif

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
    push_front(it);
  }
  T &top() {
    return this->front();
  }
  void pop() {
    this->pop_front();
  }
#ifndef USE_SLIST
    template <class O> void print(O&o) const
    {
        o << "(";
        bool first=true;
        for( typename List<T,A>::const_iterator n=begin(),e=list.end();n!=e;++n) {
            o << *n
                if (first)
                    first=false;
                else
                    o << ' ';
        }
        o << ")";
    }
    TO_OSTREAM_PRINT    
#endif 
};
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

#endif
