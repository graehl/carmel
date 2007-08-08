#ifndef GRAEHL__SHARED__DOUBLING_PRIMES_HPP
#define GRAEHL__SHARED__DOUBLING_PRIMES_HPP

#include <algorithm>

namespace graehl {

namespace detail {

enum { doubling_num_primes = 28 };

static const unsigned doubling_prime_list[doubling_num_primes] = {
    53u,         97u,         193u,       389u,       769u,
    1543u,       3079u,       6151u,      12289u,     24593u,
    49157u,      98317u,      196613u,    393241u,    786433u,
    1572869u,    3145739u,    6291469u,   12582917u,  25165843u,
    50331653u,   100663319u,  201326611u, 402653189u, 805306457u,
    1610612741u, 3221225473u, 4294967291u
};

}

// return next doubled (roughly) prime larger than or equal to n
inline unsigned
next_doubled_prime(unsigned n)
{
    unsigned const* i = detail::doubling_prime_list; 
    unsigned const* last = i + (int)detail::doubling_num_primes-1;
    for (;;++i)
        if (i==last || *i >= n)
            return *i;
    }
}


#endif
