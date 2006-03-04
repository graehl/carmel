#ifndef MEMORY_STATS_HPP
#define MEMORY_STATS_HPP

#include <cstdlib>
#include "io.hpp"
#include "malloc.h"


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
    memory_stats ret;
#define MEMSTAT_DIFF(field) ret.info.field=after.info.field-before.info.field
MEMSTAT_DIFF(arena);    /* total space allocated from system */
MEMSTAT_DIFF(ordblks);  /* number of non-inuse chunks */
MEMSTAT_DIFF(smblks);   /* unused -- always zero */
MEMSTAT_DIFF(hblks);    /* number of mmapped regions */
MEMSTAT_DIFF(hblkhd);   /* total space in mmapped regions */
MEMSTAT_DIFF(usmblks);  /* unused -- always zero */
MEMSTAT_DIFF(fsmblks);  /* unused -- always zero */
MEMSTAT_DIFF(uordblks); /* total allocated space */
MEMSTAT_DIFF(fordblks); /* total non-inuse space */
MEMSTAT_DIFF(keepcost); /* top-most, releasable (via malloc_trim) space */
#undef MEMSTAT_DIFF
return ret;
//    using namespace memory_stats_detail;
//    return transform2_array_coerce<unsigned>(after,before,difference_f<int>());
}

inline std::ostream &operator << (std::ostream &o, const memory_stats &s) {
    typedef size_mega<false> sz;
    return o << "["<<s.program_allocated()<<" allocated, " << s.system_allocated() << " from system, "<<s.memory_mapped()<<" memory mapped]";
}


#endif