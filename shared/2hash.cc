#include "2hash.h"

template <typename K, typename V>
std::ostream & operator << (std::ostream & o, const Entry<K,V> & e)
{
  o << "Next: " << (e.next ? "Yes" : "No") << "\tKey: " << e.key << "\tVal: " << e.val;
  return o;
}

