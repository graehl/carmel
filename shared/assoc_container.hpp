#ifndef GRAEHL__SHARED__ASSOC_CONTAINER_HPP
#define GRAEHL__SHARED__ASSOC_CONTAINER_HPP

#ifdef GRAEHL__DBG_ASSOC
#include <graehl/shared/debugprint.hpp>
#endif

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

struct accumulate_multiply 
{
    template <class A,class X>
    void operator()(A &sum,X const& x) const 
    {
        sum *= x;
    }
};

struct accumulate_sum
{
    template <class A,class X>
    void operator()(A &sum,X const& x) const 
    {
        sum += x;
    }
};

struct accumulate_max
{
    template <class A,class X>
    void operator()(A &sum,X const& x) const 
    {
        if (x > sum)
            sum=x;
    }
};

struct accumulate_min
{
    template <class A,class X>
    void operator()(A &sum,X const& x) const 
    {
        if (x < sum)
            sum=x;
    }
};


template <class AssocContainer,class Key,class Val,class AccumF>
inline void accumulate(AssocContainer &table,const Key &key,const Val &val,AccumF accum_f) {
#ifdef GRAEHL__DBG_ASSOC
    DBPC3("accumulate-pre",key,val);
#endif 
    std::pair<typename AssocContainer::iterator,bool> was_inserted
        =table.insert(typename AssocContainer::value_type(key,val));
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

template <class AssocDest,class AssocSource,class AccumF>
inline void accumulate_pairs(AssocDest &table,AssocSource const &source,AccumF accum_f)
{
    accumulate_pairs(table,source.begin(),source.end(),accum_f);
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

template <class Container> inline
typename Container::value_type &at_expand(Container &vec,std::size_t index,const typename Container::value_type &default_value=typename Container::value_type()) 
{
    std::size_t sz=vec.size();
    if (index>=sz)
        vec.resize(index+1,default_value); //     vec.insert(vec.end(),(index-sz)+1,default_value);
    return vec[index];
}

template <class Container> inline
void maybe_decrease_min_at(Container &vec,unsigned index,const typename Container::value_type &decrease_to,const typename Container::value_type &infinity=HUGE_VAL) 
{
    typename Container::value_type &f=at_expand(vec,index,infinity);
    if (f > decrease_to)
        f = decrease_to;
}


}

#endif
