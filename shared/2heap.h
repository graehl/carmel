#ifndef TWO_HEAP_H
#define TWO_HEAP_H

#include <cstdlib>
using std::size_t;

// binary maximum-heap with elements packed in [heapStart, heapEnd) - heap-sorted on > (*heapStart is the maximum element)

// heapEnd - heapStart = number of elements
template <typename T> inline size_t heapSize ( T *s, T *e )
{
  return e - s;
}

template <typename T> inline void heapAdd ( T *heapStart, T *heapEnd, const T& elt )
     // caller is responsbile for ensuring that *heapEnd is allocated and
     // safe to store the element in (and keeping track of increased size)
{
  T *heap = heapStart - 1;
  size_t i = heapEnd - heap;
  size_t last = i;
  while ( (i /= 2) && heap[i] < elt ) {
    heap[last] = heap[i];
    last = i;
  }
  heap[last] = elt;
}

// internal routine: repair sub-heap condition given a violation at root element i (heap[i=1]==root)
template <typename T> static inline void heapify ( T *heap, size_t heapSize, size_t i)
{
  T temp = heap[i];
  size_t parent = i, child = 2*i;
  while ( child < heapSize ) {
    if ( heap[child] < heap[child+1] )
      ++child;
    if ( !(temp < heap[child] ) )
      break;
    heap[parent] = heap[child];
    parent = child;
    child *= 2;
  }
  if ( child == heapSize && temp < heap[child]) {
    heap[parent] = heap[child];
    parent = child;
  }
  heap[parent] = temp;
}

template <typename T> void heapPop (T *heapStart, T *heapEnd)
{
  T *heap = heapStart - 1;  // to start numbering of array at 1
  size_t heapSize = heapSize(heapStart,heapEnd);
  heap[1] = heap[heapSize--];
  heapAdjustRootDown
  heapify(heap, heapSize, 1);
}

template <typename T> inline T & heapTop (T *heapStart)
{
  return *heapStart;
}


template <typename T> void heapBuild ( T *heapStart, T *heapEnd )
{
  T *heap = heapStart - 1;
  size_t size = heapEnd - heapStart;
  for ( size_t i = size/2 ; i ; --i )
    heapify(heap, size, i);
}

// *element may need to be moved up toward the root of the heap.  fix.
template <typename T> inline void heapAdjustUp ( T *heapStart, T *element)
{
  T *heap = heapStart - 1;
  size_t parent, current = element - heap;
  T temp = heap[current];
  while ( current > 1 ) {
    parent = current / 2;
    if ( !(heap[parent] < temp) )
      break;
    heap[current] = heap[parent];
    current = parent;
  }
  heap[current] = temp;
}

// *heapStart may need to be moved up toward the bottom of the heap.  fix.
template <typename T> inline void heapAdjustRootDown ( T *heapStart, T *heapEnd)
{    
  T *heap = heapStart - 1;
  heapify(heap,heapSize(heapStart,heapEnd),1);
}

// *heapStart may need to be moved up toward the bottom of the heap.  fix.
template <typename T> inline void heapAdjustDown ( T *heapStart, T *heapEnd, T *element)
{    
  T *heap = heapStart - 1;
  heapify(heap,heapSize(heapStart,heapEnd),element-heap);
}

template <typename T> void heapSort (T *heapStart, T *heapEnd)
{
  heapBuild(heapStart, heapEnd);
  T *heap = heapStart - 1;      // to start numbering of array at 1
  T temp;
  int heapSize = heapEnd - heapStart;
  for ( int i = heapSize ; i != 1 ; --i ) {
    temp = heap[1];
    heap[1] = heap[i];
    heap[i] = temp;
    heapify(heap, i-1, 1);
  }
}


template <typename T> void treeHeapAdd(T *&heapRoot, T *node)
{
  T *oldRoot = heapRoot;
  if ( !oldRoot ) {
    heapRoot = node;
    node->left = node->right = NULL;
    node->nDescend = 0;
    return;
  }
  ++oldRoot->nDescend;
  int goLeft = !oldRoot->left || (oldRoot->right && oldRoot->right->nDescend > oldRoot->left->nDescend);
  if ( *oldRoot < *node ) {
    node->left = oldRoot->left;
    node->right = oldRoot->right;
    node->nDescend = oldRoot->nDescend;
    heapRoot = node;
    if ( goLeft )
      treeHeapAdd(node->left, oldRoot);
    else
      treeHeapAdd(node->right, oldRoot);
  } else {
    if (goLeft)
      treeHeapAdd(oldRoot->left, node);
    else
      treeHeapAdd(oldRoot->right, node);
  }
}


template <typename T> T *newTreeHeapAdd(T *heapRoot, T *node)
{
  if ( !heapRoot ) {
    node->left = node->right = NULL;
    node->nDescend = 0;
    return node;
  }
  T *newRoot = new T(*heapRoot);
  ++newRoot->nDescend;
  bool goLeft = !newRoot->left || (newRoot->right && newRoot->right->nDescend > newRoot->left->nDescend);
  if ( *newRoot < *node ) {
    node->left = newRoot->left;
    node->right = newRoot->right;
    node->nDescend = newRoot->nDescend;
    if ( goLeft )
      node->left = newTreeHeapAdd(node->left, newRoot);
    else
      node->right = newTreeHeapAdd(node->right, newRoot);
    return node;
  } else {
    if (goLeft)
      newRoot->left = newTreeHeapAdd(newRoot->left, node);
    else
      newRoot->right = newTreeHeapAdd(newRoot->right, node);
    return newRoot;
  }
}

// (vector) container versions (require that begin and end be C::value_type *)
template <typename C>
inline C & heapTop (const C & heap) {
  return heap.front();
}

template <typename C>
inline void heapPop (C & heap) {
  heapPop(heap.begin(),heap.end());
  heap.pop_back();
}

template <typename C>
inline void heapAdd ( C &heap, const typename C::value_type& elt ) {
  heap.push_back();
  heapAdd(heap.begin(),heap.end()-1,elt);
}

template <typename C>
inline void heapBuild ( C &heap ) {
  heapBuild(heap.begin(),heap.end());
}

template <typename C>
inline void heapAdjustUp ( C &heap, typename C::iterator element) {
  heapAdjustUp(heap.begin(),element);
}

template <typename C>
inline void heapSort ( C &heap ) {
  heapSort(heap.begin(),heap.end());
}

#endif
