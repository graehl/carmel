/*******************************************************************************
* This software ("Carmel") is licensed for research use only, as described in  *
* the LICENSE file in this distribution.  The software can be downloaded from  *
* http://www.isi.edu/natural-language/licenses/carmel-license.html.  Please    *
* contact Yaser Al-Onaizan (yaser@isi.edu) or Kevin Knight (knight@isi.edu)    *
* with questions about the software or commercial licensing.  All software is  *
* copyrighted C 2000 by the University of Southern California.                 *
*******************************************************************************/
#ifndef LIST_H 
#define LIST_H 1

#ifdef _MSC_VER
 #include <list>
 #define STL_LIST std::list
#else
 #include <slist>
 #define STL_LIST std::slist
#endif

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
  List<T>::const_iterator end = list.end();  
  for( List<T>::const_iterator n = list.begin() ; n != end ;++n)
    out << *n << ' ';
  out << ")";
  return out;
}

#endif
