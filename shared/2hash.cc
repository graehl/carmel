#include "2hash.h"

#if 0
template <typename K, typename V>
std::ostream & operator << (std::ostream & o, const Entry<K,V> & e)
{
  o << "Next: " << (e.next ? "Yes" : "No") << "\tKey: " << e.key << "\tVal: " << e.val;
  return o;
}
#endif
