/*******************************************************************************
* This software ("Carmel") is licensed for research use only, as described in  *
* the LICENSE file in this distribution.  The software can be downloaded from  *
* http://www.isi.edu/natural-language/licenses/carmel-license.html.  Please    *
* contact Yaser Al-Onaizan (yaser@isi.edu) or Kevin Knight (knight@isi.edu)    *
* with questions about the software or commercial licensing.  All software is  *
* copyrighted C 2000 by the University of Southern California.                 *
*******************************************************************************/

#ifndef NODE_H
#define NODE_H

template <class T> struct Node {
  T data;
  Node<T> *next;
  Node(const T &it) : data(it), next(NULL) { }
  Node(const T &it, Node<T> *ne) : data(it), next(ne) { }
  static Node<T> *freeList;
  static const int newBlocksize;
#ifdef CUSTOMNEW
  void *operator new(size_t s)
  {
    size_t dummy = s;
    dummy = dummy;
    Node<T> *ret, *max;
    if (freeList) {
      ret = freeList;
      freeList = freeList->next;
      return ret;
    }
    freeList = (Node<T> *)::operator new(newBlocksize * sizeof(Node<T>));
    freeList->next = NULL;
    max = freeList + newBlocksize -1;
    for ( ret = freeList++; freeList < max ; ret = freeList++ )
      freeList->next = ret;
    return freeList--;
  }
  void operator delete(void *p) 
  {
    Node<T> *e = (Node<T> *)p;
    e->next = freeList;
    freeList = e;
  }
#endif
};

#endif
