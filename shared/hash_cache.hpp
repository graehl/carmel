#ifndef GRAEHL__SHARED__HASH_CACHE_HPP
#define GRAEHL__SHARED__HASH_CACHE_HPP

#include <boost/functional/hash.hpp>
//#include <graehl/shared/hashtable_fwd.hpp>
#include <vector>
#include <cassert>

#include <graehl/shared/doubling_primes.hpp>

namespace graehl {

/* *Hash is a stateless functor:
       size_t Hash::operator()(const Key &key) const
       
    *Backing class is stored by value, and must provide:
       void Backing::get(const Key &key,Val *val)

     *Key must provide equality,assignment
*/
template <class Key,class Val,class Backing,class Hash=boost::hash<Key>, size_t default_cache_size=7919 >
struct hash_cache 
{
    Backing backing_store;
    static Hash stateless_hasher;
    unsigned cachesize;
    typedef hash_cache<Key,Val,Backing> Self;
    static unsigned hash(const Key &key)
    {
        return stateless_hasher(key);
    }
    hash_cache(const Key &null_value=Key(),std::size_t cache_size=10000) { init(null_value); }
    hash_cache(const Key &null_value=Key(),const Backing &back,std::size_t cache_size=10000) : backing_store(back) { init(null_value); }
    typedef std::pair<Key,Val> entry;
    typedef std::vector<entry> cache;
    void init(const Key &null_value=Key(),std::size_t cache_size=10000) 
    {
        cachesize=next_doubled_prime(cache_size);
        cache.clear();
        cache.resize(cachesize,entry(null_value,Val()));
        assert(cache.size()==cachesize);
        n_miss=n_hit=0;
    }    
    std::size_t n_miss,n_hit;
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
