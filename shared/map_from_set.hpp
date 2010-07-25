#ifndef GRAEHL_HASH_MAP_FROM_SET_HPP
#define GRAEHL_HASH_MAP_FROM_SET_HPP

#include <utility> // std::pair

namespace graehl {

template <class KeyT>
struct first_
{
    typedef KeyT const& result_type;
    template <class Pair>
    result_type operator()(Pair const& pair) const
    { return pair.first; }
};

// H is a hash_set<pair<K,V> > with hash/equal based only on K
template <class HashSet>
struct map_from_set
{
    typedef HashSet HT;
    typedef typename HT::value_type pair_type;
    typedef typename HT::iterator iterator;
    typedef typename HT::const_iterator const_iterator;

    typedef typename pair_type::first_type key_type;
    typedef typename pair_type::second_type data_type;

    typedef std::pair<iterator,bool> insert_ret;

//    typedef key_type K;
    // key type is templated to avoid implicit type conversions for multi_index?  or does the conversion happen anyway?
    typedef data_type V;

    template <class K>
    inline static
    data_type & get(HT &ht,K const& k)
    {
        return const_cast<V &>(ht.insert(pair_type(k,V())).first->second);
    }

    template <class K>
    inline static
    data_type & get(HT &ht,K const& k, bool &is_new)
    {
        insert_ret i=ht.insert(pair_type(k,V()));
        is_new=i.second;
        return const_cast<V &>(i.first->second);
    }

    template <class K>
    inline static
    V & get_default(HT &ht,K const& k,V const& v)
    {
        return const_cast<V &>(ht.insert(pair_type(k,v)).first->second);
    }

    template <class K>
    inline static
    V & get_default(HT &ht,K const& k,V const& v, bool &is_new)
    {
        insert_ret i=ht.insert(pair_type(k,v));
        is_new=i.second;
        return const_cast<V &>(i.first->second);
    }

    template <class K>
    inline static
    V &get_existing(HT const&ht,K const& key)
    {
        typename HT::const_iterator i=ht.find(key);
        if (i==ht.end())
            throw std::runtime_error("Key not found in table (should always be there!)");
        return const_cast<V&>(i->second);
    }

    template <class K>
    inline static
    insert_ret insert(HT &ht,K const& key,V const &v=V())
    {
        return ht.insert(value_type(key,v));
    }

    template <class K>
    inline static
    void set(HT &ht,K const& key,V const &v=V())
    {
        std::pair<typename HT::iterator,bool> r=ht.insert(value_type(key,v));
        if (!r.second)
            const_cast<V&>(r.first->second)=v;
    }

    template <class K>
    inline static
    V * get_ptr(HT &ht,K const& key)
    {
        typename HT::iterator i=ht.find(key);
        return i==ht.end() ? 0 : &const_cast<V&>(i->second);
    }

    template <class K>
    inline static
    V const* get_ptr(HT const& ht,K const& key)
    {
        return get_ptr(const_cast<HT&>(ht),key);
    }

};


template <class H>
typename H::value_type::second_type &
map_get(H &h
        ,typename H::value_type::first_type const& k)
{
    typedef typename H::value_type::second_type V;
    return const_cast<V&>(h.insert(value_type(k,V())).first->second);
}

template <class H>
typename H::value_type::second_type &
map_get_default(H &h
                ,typename H::value_type::first_type const& k
                ,typename H::value_type::second_type const& v)
{
    typedef typename H::value_type::second_type V;
    return const_cast<V&>(h.insert(value_type(k,v)).first->second);
}

/// throws if k not in h
template <class H>
typename H::value_type::second_type &
map_get_existing(H const& h
                 ,typename H::value_type::first_type const& key)
{
    typedef typename H::value_type::second_type V;
    typename H::iterator i=h.find(key);
    if (i==h.end())
        throw std::runtime_error("Expected to get a key from map that isn't there");
    return const_cast<V&>(i->second);
}

template <class H>
void
map_set(H &h
        ,typename H::value_type::first_type const& k
        ,typename H::value_type::second_type const& v)
{
    typedef typename H::value_type::second_type V;
    std::pair<typename H::iterator,bool> r=h.insert(typename H::value_type(k,v));
    if (!r.second)
        const_cast<V&>(r.first->second)=v;
}

template <class H>
typename H::value_type::second_type *
map_get_ptr(H const& h
            ,typename H::value_type::first_type const& k)
{
    typedef typename H::value_type::second_type V;
    typename H::iterator i=h.find(k);
    if (i==h.end())
        return 0;
    return &const_cast<V &>(i->second);
}


template <class H>
std::pair<typename H::iterator,bool>
map_insert(H &h
           ,typename H::value_type::first_type const& k
           ,typename H::value_type::second_type const& v=typename H::value_type::second_type())
{
    return h.insert(typename H::value_type(k,v));
}

}



#endif
