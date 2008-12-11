#ifndef GRAEHL_SHARED__DYNAMIC_HASH_CACHE_HPP
#define GRAEHL_SHARED__DYNAMIC_HASH_CACHE_HPP

#include <graehl/shared/dynamic_sized.hpp>
#include <graehl/shared/doubling_primes.hpp>
#include <graehl/shared/hash_functions.hpp>
#include <graehl/shared/no_locking.hpp>
#include <graehl/shared/percent.hpp>
#include <cassert>
#include <iostream>

// coarse grain locking (saves memory, negligible when you can just use a smaller cache, but worse performance).  note: no space or time overhead with graehl::no_locking
//#define DYNAMIC_HASH_CACHE_SINGLE_LOCK

//#define DYNAMIC_HASH_CACHE_TRACK_COLLISIONS

namespace graehl {

// all this essentially so I can keep unsigned *p, unsigned len separate rather than bundled into a struct?  seems silly but it's done.
/*
  struct Backing {
 typedef unsigned result_type;
 result_type operator()(unsigned *p,unsigned len);
}
*/

// note: key is stored as pascal_words_string, hash/equal/assign is hard coded plain-old-binary-data
template <class Backing,class Locking=graehl::no_locking>
struct dynamic_hash_cache
#ifdef DYNAMIC_HASH_CACHE_SINGLE_LOCK
    : private Locking
#endif 
{
    typedef typename Backing::result_type result_type;
    typedef Locking locking_type;
    typedef typename locking_type::scoped_lock lock_type;
#ifdef DYNAMIC_HASH_CACHE_TRACK_COLLISIONS
    std::size_t n_collide;
#endif 
    typedef boost::uint32_t const* quad_p;
    struct locked_result
#ifndef DYNAMIC_HASH_CACHE_SINGLE_LOCK
    : public locking_type
#endif
    {
        result_type result;
    };

    locking_type &locking(locked_result &r)
    {
        return
#ifdef DYNAMIC_HASH_CACHE_SINGLE_LOCK
            *this
#else
            r
#endif
            ;
    }
    typedef dynamic_hash_cache<Backing,Locking> self_type;
    typedef graehl::pascal_words_string key_type;
    typedef graehl::dynamic_pair<locked_result,key_type> cache_entry;
    unsigned cachesize;
    unsigned max_len;
    dynamic_sized_array<cache_entry> cache;
    Backing b;
    void flush()
    {
#ifdef DYNAMIC_HASH_CACHE_SINGLE_LOCK
        lock_type l(*this);
#endif 
#ifdef DYNAMIC_HASH_CACHE_TRACK_COLLISIONS
        n_collide=
#endif 
            n_miss=n_hit=0;
        
        assert(cachesize==cache.size);
        for (unsigned i=0;i<cachesize;++i) {
            cache_entry &e=cache[i];
#ifndef DYNAMIC_HASH_CACHE_SINGLE_LOCK
            lock_type l(e.fixed);
#endif 
            e.dynamic.set_null();
        }
    }

    unsigned capacity() const 
    {
        return cachesize;
    }

    template <class O>
    void stats(O &o) const
    {
        
        o << " (cache capacity="<< capacity()<<" hit rate="<<percent<4>(hit_rate())<<"% hits="<<n_hit<<" misses="<<n_miss;
#ifdef DYNAMIC_HASH_CACHE_TRACK_COLLISIONS
        o << " hash collisions="<<n_collide<< " collision rate="<<std::setprecision(2)<<100.*n_collide/n_queries()<<"%";
#endif 
        o <<")";
    }
    
    std::size_t n_miss,n_hit;
    double n_queries() const 
    {
        return (double)n_miss+(double)n_hit;
    }
    
    // returns [0...1] portion of hits
    double hit_rate() const
    {
        return n_hit / n_queries();
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
    result_type operator()(quad_p p,unsigned len) const 
    {
        return const_cast<self_type&>(*this)(p,len);
    }
    
    result_type operator()(quad_p p,unsigned len)
    {
        assert(len <= max_len);
        assert(cachesize==cache.size);
        unsigned i=(unsigned)graehl::hash_quads_64(p,len) % cachesize;

        bool hit;        
        cache_entry &e=cache[i];
        key_type &cached_key=e.dynamic;
        locking_type &m=locking(e.fixed);
        result_type &cached_result=e.fixed.result;
        {
            lock_type l(m);
            hit=cached_key.equal(p,len);
            if (hit) {
                ++n_hit;
                return cached_result;
            }
#ifdef DYNAMIC_HASH_CACHE_TRACK_COLLISIONS
            if (!cached_key.is_null())
                ++n_collide;
#endif 
        }
        ++n_miss;
        result_type r=b(p,len);
        {    
            lock_type l(m);
            cached_key.set(p,len);
            cached_result=r;
        }        
        return r;
    }
    
};


}


#endif
