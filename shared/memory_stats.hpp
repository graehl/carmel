#ifndef GRAEHL_SHARED__MEMORY_STATS_HPP
#define GRAEHL_SHARED__MEMORY_STATS_HPP

#if defined(__APPLE__) && defined(__MACH__)
#define _MACOSX 1
#endif

#ifdef __linux__
# include <graehl/shared/string_to.hpp>
# include <graehl/shared/proc_linux.hpp>
#endif
#include <cstdlib>
#if defined(__unix__)
# include <unistd.h>
#endif
#if defined(_MACOSX)
# include <malloc/malloc.h>
#else
# include <malloc.h>
#endif
#include <graehl/shared/io.hpp>
#include <graehl/shared/size_mega.hpp>
#include <graehl/shared/stream_util.hpp>
#include <graehl/shared/auto_report.hpp>

namespace graehl {

#if defined(__unix__)
typedef struct mallinfo malloc_info;
#elif defined(_MACOSX)
typedef struct mstats malloc_info;
#endif

struct memory_stats  {
#if defined(__unix__) || defined(_MACOSX)
    malloc_info info;
#endif
#ifdef __linux__
  double vmused; //bytes
#endif
#if defined(__unix__)
    long pagesize() const
    {
        return  sysconf(_SC_PAGESIZE);
    }

    std::ptrdiff_t process_data_end;
    char *sbrk_begin() const
    {
        return reinterpret_cast<char *>((void *)0);
    }
    char *sbrk_end() const
    {
        return (char *)sbrk(0);
    }
    std::ptrdiff_t sbrk_size() const
    {
        return sbrk_end()-sbrk_begin();
    }

#endif
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
#if defined(__unix__)
        info=mallinfo();
        process_data_end=sbrk_size();
#elif defined(_MACOSX)
        info=mstats();
#endif
#ifdef __linux__
        vmused=graehl::proc_bytes(graehl::get_proc_field(graehl::VmSize));
#endif
    }
#if defined(__unix__) || defined(_MACOSX)
    operator const malloc_info & () const {
        return info;
    }
#endif
#if defined(_MACOSX)
    typedef size_bytes_integral size_type;
#else
    typedef size_bytes size_type;
#endif

    // includes memory mapped
    size_type total_allocated() const
    {
        return program_allocated()+memory_mapped();
    }

    size_type high_water() const
    {
        return system_allocated()+memory_mapped();
    }

    size_type program_allocated() const
	{
#if defined(__unix__)
        return size_type((unsigned)info.uordblks);
#elif _MACOSX
        return size_type(info.bytes_used);
#else
		return 0;
#endif
    }

    // may only grown monotonically (may not reflect free())
    size_type system_allocated() const
    {
#ifdef __linux__
      return size_type(vmused);
#else
#if defined(__unix__)
        return process_data_end;
//        return size_type((unsigned)info.arena);
#elif defined(_MACOSX)
        return size_type(info.bytes_total);
#else
        return 0;
#endif
#endif
    }

    size_type memory_mapped() const
    {
#if defined(__unix__)
        return size_type((unsigned)info.hblkhd);
#else
		return 0;
#endif
    }
};

#define GRAEHL__MEMSTAT_DIFF(field) ret.info.field=after.info.field-before.info.field
#if defined(__unix__)
inline memory_stats operator - (memory_stats after,memory_stats before)
{
    memory_stats ret;
    ret.process_data_end = after.process_data_end-before.process_data_end;
# ifdef __linux__
    ret.vmused = after.vmused-before.vmused;
# endif
GRAEHL__MEMSTAT_DIFF(arena);    /* total space allocated from system */
GRAEHL__MEMSTAT_DIFF(ordblks);  /* number of non-inuse chunks */
GRAEHL__MEMSTAT_DIFF(smblks);   /* unused -- always zero */
GRAEHL__MEMSTAT_DIFF(hblks);    /* number of mmapped regions */
GRAEHL__MEMSTAT_DIFF(hblkhd);   /* total space in mmapped regions */
GRAEHL__MEMSTAT_DIFF(usmblks);  /* unused -- always zero */
GRAEHL__MEMSTAT_DIFF(fsmblks);  /* unused -- always zero */
GRAEHL__MEMSTAT_DIFF(uordblks); /* total allocated space */
GRAEHL__MEMSTAT_DIFF(fordblks); /* total non-inuse space */
GRAEHL__MEMSTAT_DIFF(keepcost); /* top-most, releasable (via malloc_trim) space */
#undef GRAEHL__MEMSTAT_DIFF
return ret;
//    using namespace memory_stats_detail;
//    return transform2_array_coerce<unsigned>(after,before,difference_f<int>());
}
#elif defined(_MACOSX)
inline memory_stats operator - (memory_stats after, memory_stats before)
{
    memory_stats ret;
    GRAEHL__MEMSTAT_DIFF(bytes_total);
    GRAEHL__MEMSTAT_DIFF(chunks_used);
    GRAEHL__MEMSTAT_DIFF(bytes_used);
    GRAEHL__MEMSTAT_DIFF(chunks_free);
    GRAEHL__MEMSTAT_DIFF(bytes_free);
    return ret;
}
#endif

template <class C,class T> inline std::basic_ostream<C,T> &
operator << (std::basic_ostream<C,T> &o, const memory_stats &s) {
    return o << "["<<s.program_allocated()<<" allocated, " << s.system_allocated() << " from system, "<<s.memory_mapped()<<" memory mapped]";
}

struct memory_change
{
    memory_stats before;
    static char const* default_desc()
    { return "\nmemory used: "; }
    typedef memory_stats::size_type S;
    template <class O>
    void print(O &o) const
    {
        memory_stats after;
        print_change(o,before.total_allocated(),after.total_allocated());
        o << "; from OS: ";
        print_change(o,before.high_water(),after.high_water());

    }
    template <class O>
    void print_change(O &o, S pre, S post) const
    {
        if (post == pre)
            o << '0';
        else if (post > pre)
            o << "+" << S(post-pre);
        else
            o << "-" << S(pre-post);
        o << " (" << pre << " -> " << post << ")";
    }

    typedef memory_change self_type;
    TO_OSTREAM_PRINT
};

typedef auto_report<memory_change> memory_report;

}//graehl

#endif
