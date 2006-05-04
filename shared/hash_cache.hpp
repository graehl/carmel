#ifndef HASH_CACHE_HPP
#define HASH_CACHE_HPP

#include <graehl/shared/hashtable_fwd.hpp>
#include <vector>
#include <pair>
#include <cassert>

namespace graehl {

/* *Hash is a stateless functor:
       size_t Hash::operator()(const Key &key) const
       
    *Backing class is stored by value, and must provide:
       void Backing::get(const Key &key,Val *val)

     *Key must provide equality,assignment
*/
template <class Key,class Val,class Backing,class Hash=::hash<Key>, size_t default_cache_size=7919 >
struct hash_cache 
{
    Backing backing_store;
    static Hash stateless_hasher;
    const unsigned cachesize=default_cache_size; // nice to have this prime.  TODO: growing, variable size, etc?
    typedef hash_cache<Kev,Val,Backing> Self;
    static unsigned hash(const Key &key) const
    {
        return stateless_hasher(key);
    }
    hash_cache(const Key &unused=Key()) { init(unused); }
    hash_cache(const Key &unused=Key(),const Backing &back) : backing_store(back) { init(unused); }
    typedef std::pair<Key,Val> entry;
    typedef std::vector<kvpair> cache;
    void init(const Key &unused=Key()) 
    {
        cache.clear();
        cache.resize(cachesize,entry(unused,Val()));
        assert(cache.size()==cachesize);
        n_miss=n_hit=0;
    }    
    unsigned n_miss,n_hit;
    // returns [0...1] portion of hits
    double hit_rate() 
    {
        return n_hit / ((double)n_miss+n_hit);
    }
    Val &operator[](const Key &key) 
    {
        unsigned i=hash(key) % cachesize;
        Key &cached_key=cache[i].first;
        Val &cached_val=cache[i].second;
        if (cached_key != key) {
            cached_key = key;
            ++n_miss;
            backing_store.get(key,&cached_val);
        } else {
            ++n_hit;
        }        
        return cached_val;
    }
};

}

#endif
