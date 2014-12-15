/** \file

    forall and exists ('all') and ('any').
*/


#ifndef GRAEHL__ANY_ALL___HPP
#define GRAEHL__ANY_ALL___HPP

namespace graehl {

/// exists some x in s
template <class Seq, class Pred>
bool any(Seq const& s, Pred const& p) {
  for (typename Seq::const_iterator i = s.begin(), e = s.end(); i != e; ++i)
    if (p(*i)) return true;
  return false;
}

/// for every x in s
template <class Seq, class Pred>
bool all(Seq const& s, Pred const& p) {
  for (typename Seq::const_iterator i = s.begin(), e = s.end(); i != e; ++i)
    if (!p(*i)) return false;
  return true;
}

/// does not exist some x in s
template <class Seq, class Pred>
bool none(Seq const& s, Pred const& p) {
  for (typename Seq::const_iterator i = s.begin(), e = s.end(); i!=e; ++i)
    if (p(*i)) return false;
  return true;
}

}

#endif
