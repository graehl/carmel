#ifndef LIST_H 
#define LIST_H 1
#include "config.h"

//#ifdef _MSC_VER
 #include <list>
 #define STL_LIST std::list
//#else
// #include <slist>
// #define STL_LIST std::slist
//#endif

#include <iostream>


template <typename T> 
class List: public STL_LIST<T> {
public:
  //constructors 
  List():STL_LIST<T>(){};
  List(const List &l):STL_LIST<T> (l){};
  ~List(){};
  List(const T &it):STL_LIST<T>(it) {  }
  List(size_t sz,const T &it):STL_LIST<T>(sz,it) {  }
  int notEmpty() const { return !isEmpty(); }
  int isEmpty() const { return empty(); }
  int length() const{ return static_cast<int>(size()); }
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
