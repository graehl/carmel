#ifndef LIST_H 
#define LIST_H 1
#include "config.h"

#ifdef USE_SLIST
#include "slist.h"
#define STL_LIST slist
#else
#include <list>
#define STL_LIST std::list
#endif

#include <iostream>


template <typename T> 
class List: public STL_LIST<T> {
public:
#ifndef USE_SLIST
	typedef typename List::iterator erase_iterator;
	typedef typename List::iterator val_iterator;
	typename List::const_iterator const_begin() const { return begin(); } //{ return const_cast<const List *>(this)->begin(); }
	typename List::const_iterator const_end() const { return end(); } //{ return const_cast<const List *>(this)->end(); }
	typename List::iterator val_begin() { return begin(); }
	typename List::iterator val_end() { return end(); }
#endif
	typename List::iterator erase_begin() { return begin(); }
	typename List::iterator erase_end() { return end(); }

  //constructors 
  List():STL_LIST<T>(){};
  List(const List &l):STL_LIST<T> (l){};
  ~List(){};
  List(const T &it): 
#ifdef USE_SLIST
    STL_LIST<T>(it)
#else
    STL_LIST<T>(1,it)
#endif
    {  }
  List(size_t sz,const T &it):STL_LIST<T>(sz,it) {  }
  int notEmpty() const { return !isEmpty(); }
  int isEmpty() const { return empty(); }
  int count_length() const{ 
#ifdef USE_SLIST
    int count=0;
	for (typename List::const_iterator i=const_begin(),end=const_end();i!=end;++i)
      ++count;
    return count;
#else
    return static_cast<int>(size()); 
#endif
  }
  void push(const T &it) { 
    push_front(it);
  }
  T &top() {
    return front();
  }
  void pop() {
    pop_front();
  }
};  
/*
  template <typename T> 
  class ListPostInserter {
  typedef List<T> L;
  typedef L::iterator Lit;
  L &l;
  Lit lit;
  public:
  explicit ListPostInserter(L &l_) : l(l_) {
		
  }
  ListPostInserter(L &l_,Lit &lit_) : l(l_), lit(lit_) {	}

  };
*/
#ifdef USE_SLIST
#define LIST_BACK_INSERTER slist_back_insert_iterator
//#include <iterator>
/*
  template <class T>
  class slist_back_insert_iterator
  // : public std::iterator<output_iterator_tag,T>
  {
  
  //typedef std::slist<T> L;
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
  slist_back_insert_iterator<T>&
  operator=(const typename _Container::value_type & __val) { 
  cursor = container->insert_after(cursor,__val);
  return *this;
  }
  slist_back_insert_iterator<L>& operator*() { return *this; }
  slist_back_insert_iterator<L>& operator++() { return *this; }
  slist_back_insert_iterator<L>& operator++(int) { return *this; }
  };*/
#else
#define LIST_BACK_INSERTER back_insert_iterator
#endif

template <typename T> std::ostream & operator << (std::ostream &out, const List<T> &list)
{
  out << "(";
  typename List<T>::const_iterator end = list.end();  
  for( typename List<T>::const_iterator n = list.begin() ; n != end ;++n)
    out << *n << ' ';
  out << ")";
  return out;
}

#endif
