#ifndef GRAEHL_SHARED__DYNAMIC_HASH_CACHE_HPP
#define GRAEHL_SHARED__DYNAMIC_HASH_CACHE_HPP

#include <graehl/shared/dynamic_sized.hpp>
#include <graehl/shared/doubling_primes.hpp>
#include <graehl/shared/hash_functions.hpp>
#include <cassert>

namespace graehl {

// all this essentially so I can keep unsigned *p, unsigned len separate rather than bundled into a struct?  seems silly but it's done.

// void Backing::get(unsigned *p,unsigned len,result_type *r)
// note: key is pascal_words_string, hash/equal/assign is hard coded for pod of lenx32bits pod
template <class result_type,class Backing>
struct dynamic_hash_cache
{
    typedef unsigned const* quad_p;
    
    typedef dynamic_hash_cache<result_type,Backing> self_type;
    typedef graehl::pascal_words_string key_type;
    typedef graehl::dynamic_pair<result_type,key_type> cache_entry;
    unsigned cachesize;
    unsigned max_len;
    dynamic_sized_array<cache_entry> cache;
    Backing b;
    void flush()
    {
        n_miss=n_hit=0;
        assert(cachesize==cache.size);
        for (unsigned i=0;i<cachesize;++i) {
            pascal_words_string&p=cache[i].dynamic;
            p.set_null();
        }
    }

    unsigned capacity() const 
    {
        return cachesize;
    }
    
    std::size_t n_miss,n_hit;
    // returns [0...1] portion of hits
    double hit_rate() const
    {
        return n_hit / ((double)n_miss+n_hit);
    }
    dynamic_hash_cache(Backing const& b,unsigned max_len,unsigned cache_size=10000)
        : 
        cachesize(next_doubled_prime(cache_size))
        , max_len(max_len)
        , cache(cachesize,max_len+graehl::n_unsigned(sizeof(cache_entry)))
        , b(b)
    {
        flush();
    }

    // conceptually const: answer doesn't depend on whether cache was modified
    result_type &operator()(quad_p p,unsigned len) const 
    {
        return const_cast<self_type&>(*this)(p,len);
    }
    
    result_type &operator()(quad_p p,unsigned len)
    {
        assert(len <= max_len);
        assert(cachesize==cache.size);
        unsigned i=(unsigned)graehl::hash_quads_64(p,len) % cachesize;
        cache_entry &e=cache[i];
        result_type &cached_val=e.fixed;
        key_type &cached_key=e.dynamic;
        if (cached_key.equal(p,len)) {
            ++n_hit;
        } else {
            cached_key.set(p,len);
            ++n_miss;
            b.get(p,len,&cached_val);
        }
        return cached_val;
    }
    
};


}


#endif
