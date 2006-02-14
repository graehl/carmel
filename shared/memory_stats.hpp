#ifndef MEMORY_STATS_HPP
#define MEMORY_STATS_HPP

#include <cstdlib>
#include "io.hpp"
#include "malloc.h"


namespace memory_stats_detail {
template <class V>
struct difference_f 
{
    V operator()(const V&l,const V&r) const
    {
        return l-r;
    }    
};

template <class V,class S,class F>
inline S transform2_array_coerce(const S&l,const S&r,F f) 
{
    const unsigned N=sizeof(S)/sizeof(V);
    S ret;
    const V *pl=(V*)&l;
    const V *pr=(V*)&r;
    V *pret=(V*)&ret;
    for (unsigned i=0;i<N;++i)
        pret[i]=f(pl[i],pr[i]);
    return ret;
}
}

typedef struct mallinfo malloc_info;

struct memory_stats  {
    malloc_info info;

  //   struct mallinfo {
//   int arena;    /* total space allocated from system */
//   int ordblks;  /* number of non-inuse chunks */
//   int smblks;   /* unused -- always zero */
//   int hblks;    /* number of mmapped regions */
//   int hblkhd;   /* total space in mmapped regions */
//   int usmblks;  /* unused -- always zero */
//   int fsmblks;  /* unused -- always zero */
//   int uordblks; /* total allocated space */
//   int fordblks; /* total non-inuse space */
//   int keepcost; /* top-most, releasable (via malloc_trim) space */
// };
//
    memory_stats()  {
        refresh();
    }
    void refresh() {
        info=mallinfo();
    }
    operator const malloc_info & () const {
        return info;
    }
    typedef size_mega<false,std::size_t> size_type;
    size_type program_allocated() const 
    {
        return size_type(info.uordblks);
    }
    size_type system_allocated() const 
    {
        return size_type(info.arena);
    }
    size_type memory_mapped() const
    {
        return size_type(info.hblkhd);
    }
};

inline memory_stats operator - (memory_stats after,memory_stats before) 
{
    using namespace memory_stats_detail;
    return transform2_array_coerce<unsigned>(after,before,difference_f<int>());
}

inline std::ostream &operator << (std::ostream &o, const memory_stats &s) {
    typedef size_mega<false> sz;
    return o << "["<<s.program_allocated()<<" allocated, " << s.system_allocated() << " from system, "<<s.memory_mapped()<<" memory mapped]";
}


#endif
