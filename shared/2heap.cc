#include "2heap.h"
#include "kbest.h"

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



