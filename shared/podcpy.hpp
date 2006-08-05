#ifndef GRAEHL_SHARED__PODCPY_HPP
#define GRAEHL_SHARED__PODCPY_HPP

#include <cstring>

namespace graehl {

template <class P> inline
void podset(P& dst,unsigned char c=0)
{
    std::memset((void*)&dst,c,sizeof(dst));
}

template <class P> inline
void podzero(P& dst)
{
    std::memset((void*)&dst,0,sizeof(dst));
}

template <class P> inline 
P &podcpy(P& dst,P const& src) 
{
    std::memcpy((void*)&dst,(void*)&src,sizeof(dst));
}

}


#endif
