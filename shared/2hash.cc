/*******************************************************************************
* This software ("Carmel") is licensed for research use only, as described in  *
* the LICENSE file in this distribution.  The software can be downloaded from  *
* http://www.isi.edu/natural-language/licenses/carmel-license.html.  Please    *
* contact Yaser Al-Onaizan (yaser@isi.edu) or Kevin Knight (knight@isi.edu)    *
* with questions about the software or commercial licensing.  All software is  *
* copyrighted C 2000 by the University of Southern California.                 *
*******************************************************************************/

#include "2hash.h"

template <typename K, typename V>
std::ostream & operator << (std::ostream & o, const Entry<K,V> & e)
{
  o << "Next: " << (e.next ? "Yes" : "No") << "\tKey: " << e.key << "\tVal: " << e.val;
  return o;
}

