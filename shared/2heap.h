/*******************************************************************************
* This software ("Carmel") is licensed for research use only, as described in  *
* the LICENSE file in this distribution.  The software can be downloaded from  *
* http://www.isi.edu/natural-language/licenses/carmel-license.html.  Please    *
* contact Yaser Al-Onaizan (yaser@isi.edu) or Kevin Knight (knight@isi.edu)    *
* with questions about the software or commercial licensing.  All software is  *
* copyrighted C 2000 by the University of Southern California.                 *
*******************************************************************************/
#ifndef TO_HEAP_H
#define TO_HEAP_H 1

// binary max heap with elements packed in [heapStart, heapEnd)
// heapEnd - heapStart = number of elements

template <class T> int heapSize ( T *s, T *e )
{
  return e - s;
}

template <class T> void heapAdd ( T *heapStart, T *heapEnd, const T& elt ) 
     // caller is responsbile for ensuring that *heapEnd is allocated and 
     // safe to store the element in (and keeping track of increased size)
{
  T *heap = heapStart - 1;
  int i = heapEnd - heap;
  int last = i;
  while ( (i /= 2) && heap[i] < elt ) {
    heap[last] = heap[i];
    last = i;
  }
  heap[last] = elt;
}

template <class T> static void heapify ( T *heap, int heapSize, int i) // internal routine
{
  T temp = heap[i];
  int parent = i, child = 2*i;
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

template <class T> void heapPop (T *heapStart, T *heapEnd)
{
  T *heap = heapStart - 1;	// to start numbering of array at 1
  int heapSize = heapEnd - heapStart;
  heap[1] = heap[heapSize--];
  heapify(heap, heapSize, 1);
}

template <class T> void heapBuild ( T *heapStart, T *heapEnd )
{
  T *heap = heapStart - 1;
  int size = heapEnd - heapStart;
  for ( int i = size/2 ; i ; --i )
    heapify(heap, size, i);
}


template <class T> void heapAdjustUp ( T *heapStart, T *element)
{
  T *heap = heapStart - 1;
  int parent, current = element - heap;
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

template <class T> void treeHeapAdd(T *&heapRoot, T *node);

template <class T> void heapSort (T *heapStart, T *heapEnd);

template <class T> T *newTreeHeapAdd(T *heapRoot, T *node)
{
  if ( !heapRoot ) {
    node->left = node->right = NULL;
    node->nDescend = 0;
    return node;
  }
  T *newRoot = new T(*heapRoot);
  ++newRoot->nDescend;
  int goLeft = !newRoot->left || (newRoot->right && newRoot->right->nDescend > newRoot->left->nDescend);
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

#endif
