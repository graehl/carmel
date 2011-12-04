#ifndef GRAEHL__SHARED__STREAM_UTIL_HPP
#define GRAEHL__SHARED__STREAM_UTIL_HPP

#include <iomanip>
#include <iostream>
#include <cmath>

namespace graehl {

// faster c++ stream -> OS read/write
inline void unsync_stdio()
{
    std::ios_base::sync_with_stdio(false);
}

// also don't coordinate input/output (for stream processing vs. interactive prompt/response)
inline void unsync_cout()
{
    unsync_stdio();
    std::cin.tie(0);
}



template <class I,class O>
void copy_stream_to(O &o,I&i)
{
    o << i.rdbuf();
}

template <class I>
void rewind_read(I &i)
{
    i.seekg(0,std::ios::beg);
}

#if 0
std::ostream &trunc_(std::ostream &st, const char *s)
{
    if (st.opfx()) {
        int l = strlen(s), w = st.width(0);
        if (l > w)
            if (st.flags() & ios::right)
                st.write(s + l - w, w);
            else
                st.write(s, w);
        else
            st << s;
        st.osfx();
    }
    return st;
}


/* //omanip = old gcc extension?
omanip<const char *> trunc(const char *s)
{
    return omanip<const char *>(trunc_, s);
}
*/
#endif

}//graehl

#endif
