#ifndef GRAEHL__SHARED__ASSOC_CONTAINER_HPP
#define GRAEHL__SHARED__ASSOC_CONTAINER_HPP

namespace graehl {

// table requires Val::operator += as well as copy ctor.  TODO: version that takes InPlaceFold functor.  doesn't rely on default contructor: 0
template <class AssocContainer,class Key,class Val>
inline void accumulate(AssocContainer &table,const Key &key,const Val &val) {
    std::pair<typename AssocContainer::iterator,bool> was_inserted
        =table.insert(typename AssocContainer::value_type(key,val));
    if (!was_inserted.second)
        was_inserted.first->second += val;
}

template <class AssocDest,class It>
inline void accumulate_pairs(AssocDest &table,It begin,It end)
{
    for (;begin!=end;++begin)
        accumulate(table,begin->first,begin->second);
}

template <class AssocDest,class AssocSource>
inline void accumulate_pairs(AssocDest &table,AssocSource const &source)
{
    accumulate_pairs(table,source.begin(),source.end());
}

template <class AssocContainer,class Key,class Val>
inline void maybe_decrease_min(AssocContainer &table,const Key &key,const Val &val) {
    std::pair<typename AssocContainer::iterator,bool> was_inserted=table.insert(typename AssocContainer::value_type(key,val));
    if (!was_inserted.second) {
        typename AssocContainer::value_type::second_type &old_val=was_inserted.first->second;
//        INFOL(29,"maybe_decrease_min",key << " val=" << val << " oldval=" << old_val);
        if (val < old_val)
            old_val=val;
    } else {
//                INFOL(30,"maybe_decrease_min",key << " val=" << val);
    }
    //    return was_inserted->first->second;
}

template <class AssocContainer,class Key,class Val>
inline void maybe_increase_max(AssocContainer &table,const Key &key,const Val &val) {
    std::pair<typename AssocContainer::iterator,bool> was_inserted=table.insert(typename AssocContainer::value_type(key,val));
    if (!was_inserted.second) {
        typename AssocContainer::value_type::second_type &old_val=was_inserted.first->second;
//        INFOL(29,"maybe_increase_max",key << " val=" << val << " oldval=" << old_val);
        //!FIXME: no idea how to get this << to use ns_Syntax::operator <<
        if (old_val < val)
            old_val=val;
    } else {
//                INFOL(30,"maybe_increase_max",key << " val=" << val);
    }
//    return was_inserted->first->second;    
}

template <class To,class From>
inline void maybe_increase_max(To &to,const From &from) {
    if (to < from)
        to=from;
}

template <class To,class From>
inline void maybe_decrease_min(To &to,const From &from) {
    if (from < to)
        to=from;
}


}

#endif
