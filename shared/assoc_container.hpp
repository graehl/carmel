#ifndef GRAEHL__SHARED__ASSOC_CONTAINER_HPP
#define GRAEHL__SHARED__ASSOC_CONTAINER_HPP

/// algorithms for map-like (or hash_map-like), and vector as map with unsigned keys (with default value for expansion).
/// the latter version are named *_at.  at_expand = rvalue.  at_default = lvalue (doesn't grow even if off range)

/// accumulate (default = addition).  see accumulate.hpp for others.  maintain min/max.  

#ifdef GRAEHL__DBG_ASSOC
#include <graehl/shared/debugprint.hpp>
#endif

namespace graehl {

// table requires Val::operator += as well as copy ctor.  TODO: version that takes InPlaceFold functor.  doesn't rely on default contructor: 0
template <class Map,class Key,class Val>
inline void accumulate(Map &table,const Key &key,const Val &val) {
    std::pair<typename Map::iterator,bool> was_inserted
        =table.insert(typename Map::value_type(key,val));
    if (!was_inserted.second)
        was_inserted.first->second += val;
}

template <class Map,class It>
inline void accumulate_pairs(Map &table,It begin,It end)
{
    for (;begin!=end;++begin)
        accumulate(table,begin->first,begin->second);
}

template <class Map,class Pairs>
inline void accumulate_pairs(Map &table,Pairs const &source)
{
    accumulate_pairs(table,source.begin(),source.end());
}

template <class Map,class Key,class Val,class AccumF>
inline void accumulate(Map &table,const Key &key,const Val &val,AccumF accum_f) {
#ifdef GRAEHL__DBG_ASSOC
    DBPC3("accumulate-pre",key,val);
#endif 
    std::pair<typename Map::iterator,bool> was_inserted
        =table.insert(typename Map::value_type(key,val));
    if (!was_inserted.second)
        accum_f(was_inserted.first->second,val);
#ifdef GRAEHL__DBG_ASSOC
    assert(table.find(key)!=table.end());
    DBPC3("accumulate-post",key,table[key]);
#endif 
}

template <class AssocDest,class It,class AccumF>
inline void accumulate_pairs(AssocDest &table,It begin,It end,AccumF accum_f)
{
    for (;begin!=end;++begin)
        accumulate(table,begin->first,begin->second,accum_f);
}

template <class AssocDest,class Pairs,class AccumF>
inline void accumulate_pairs(AssocDest &table,Pairs const &source,AccumF accum_f)
{
    accumulate_pairs(table,source.begin(),source.end(),accum_f);
}

template <class Map,class Key,class Val>
inline void maybe_decrease_min(Map &table,const Key &key,const Val &val) {
    std::pair<typename Map::iterator,bool> was_inserted=table.insert(typename Map::value_type(key,val));
    if (!was_inserted.second) {
        typename Map::value_type::second_type &old_val=was_inserted.first->second;
//        INFOL(29,"maybe_decrease_min",key << " val=" << val << " oldval=" << old_val);
        if (val < old_val)
            old_val=val;
    } else {
//                INFOL(30,"maybe_decrease_min",key << " val=" << val);
    }
    //    return was_inserted->first->second;
}

template <class Map,class Key,class Val>
inline void maybe_increase_max(Map &table,const Key &key,const Val &val) {
    std::pair<typename Map::iterator,bool> was_inserted=table.insert(typename Map::value_type(key,val));
    if (!was_inserted.second) {
        typename Map::value_type::second_type &old_val=was_inserted.first->second;
//        INFOL(29,"maybe_increase_max",key << " val=" << val << " oldval=" << old_val);
        //!FIXME: no idea how to get this << to use ns_Syntax::operator <<
        if (old_val < val)
            old_val=val;
    } else {
//                INFOL(30,"maybe_increase_max",key << " val=" << val);
    }
//    return was_inserted->first->second;    
}

template <class Vector> inline
typename Vector::value_type &at_expand(Vector &vec,std::size_t index,const typename Vector::value_type &default_value=typename Vector::value_type()) 
{
    std::size_t sz=vec.size();
    if (index>=sz)
        vec.resize(index+1,default_value); //     vec.insert(vec.end(),(index-sz)+1,default_value);
    return vec[index];
}

template <class Vector> inline
typename Vector::value_type const& at_default(Vector const& vec,std::size_t index,typename Vector::value_type const& default_value=typename Vector::value_type())
{
    std::size_t sz=vec.size();
    if (index>=sz)
        return default_value;
    return vec[index];    
}

template <class Vector> inline
void maybe_decrease_min_at(Vector &vec,unsigned index,const typename Vector::value_type &decrease_to,const typename Vector::value_type &infinity=HUGE_VAL) 
{
    typename Vector::value_type &f=at_expand(vec,index,infinity);
    if (f > decrease_to)
        f = decrease_to;
}

template <class Vector,class Key,class Val,class AccumF> inline
void accumulate_at(Vector &table,const Key &key,const Val &val,AccumF accum_f) {
#ifdef GRAEHL__DBG_ASSOC
    DBPC3("accumulate-pre",key,val);
#endif
    typename Vector::value_type default_value;
    accum_f(default_value);
    accum_f(at_expand(table,key,default_value),val);
#ifdef GRAEHL__DBG_ASSOC
    DBPC3("accumulate-post",key,table[key]);
#endif 
}

template <class Vector,class It,class AccumF> inline
void accumulate_at_pairs(Vector &table,It begin,It end,AccumF accum_f)
{
    for (;begin!=end;++begin)
        accumulate_at(table,begin->first,begin->second,accum_f);
}

template <class Vector,class Pairs,class AccumF> inline
void accumulate_at_pairs(Vector &table,Pairs const &source,AccumF accum_f)
{
    accumulate_at_pairs(table,source.begin(),source.end(),accum_f);
}
    

}//graehl

#endif
