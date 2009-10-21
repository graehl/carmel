#ifndef GRAEHL__SHARED__MAYBE_UPDATE_BOUND_HPP
#define GRAEHL__SHARED__MAYBE_UPDATE_BOUND_HPP

namespace graehl {

// see also associative container versions in assoc_container.hpp

template <class To,class From>
inline void maybe_increase_max(To &to,const From &from) {
    if (to<from)
        to=from;
}

template <class To,class From>
inline void maybe_decrease_min(To &to,const From &from) {
    if (from<to)
        to=from;
}

} //graehl


#endif
