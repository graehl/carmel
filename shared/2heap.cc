/*******************************************************************************
* This software ("Carmel") is licensed for research use only, as described in  *
* the LICENSE file in this distribution.  The software can be downloaded from  *
* http://www.isi.edu/natural-language/licenses/carmel-license.html.  Please    *
* contact Yaser Al-Onaizan (yaser@isi.edu) or Kevin Knight (knight@isi.edu)    *
* with questions about the software or commercial licensing.  All software is  *
* copyrighted C 2000 by the University of Southern California.                 *
*******************************************************************************/

#include "2heap.h"
#include "kbest.h"

template <typename T> void heapSort (T *heapStart, T *heapEnd)
{
  heapBuild(heapStart, heapEnd);
  T *heap = heapStart - 1;	// to start numbering of array at 1
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



