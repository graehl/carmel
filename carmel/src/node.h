#ifndef NODE_H
#define NODE_H
#include "config.h"
template <typename T> struct Node {
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
