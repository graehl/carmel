#ifndef GRAEHL_SHARED__SHOW_HPP
#define GRAEHL_SHARED__SHOW_HPP

/// for debugging prints

/// you can define SHOWS to something else first. needs to support SHOWS<<x for output
#ifndef SHOWS
#include <iostream>
#define SHOWS std::cerr
#endif

#undef SHOWALWAYS
#define SHOWALWAYS(x) x

/* usage:
#if DEBUG
# define IFD(x) x
#else
# define IFD(x)
#endif

SHOWS is the stream (std::cerr or whatever - you can define)
SHOWP is just print.
SHOWC(x) prints x=x
SHOW.. is SHOWC.. + newline
SHOWNL is just newline

e.g. SHOWC(IFD,x) SHOWC(IFD,y) SHOW(IFD,nl_after)
or, shorter, SHOW3(IFD,x,y,nl_after)

will both print x=X y=Y nl_after=NL_AFTER\n if DEBUG.

careful: none of this is wrapped in a block.  so you can't use one of these macros as a single-line block.

e.g.

#include <graehl/shared/ifdbg.hpp>
DECLARE_DBG_LEVEL_IF(IFD)
// then: //IFDBG(IFD,1) { SHOWM2(IFD,"descr"); } //({} not optional)
// or: //SHOWIF1(IFD,"descr",value)
*/

#define SHOWE(x) " "#x<<"="<<(x)<<" "
#define SHOWP(IF,x) IF(SHOWS<<x;)
#define SHOWNL(IF) SHOWP(IF,"\n")
#define SHOWC(IF,x,s) SHOWP(IF,#x<<"="<<(x)<<s)
#define SHOW(IF,x) SHOWC(IF,x,"\n")
#define SHOW1(IF,x) SHOWC(IF,x," ")
#define SHOW2(IF,x,y) SHOW1(IF,x) SHOW(IF,y)
#define SHOW3(IF,x,y0,y1) SHOW1(IF,x) SHOW2(IF,y0,y1)
#define SHOW4(IF,x,y0,y1,y2) SHOW1(IF,x) SHOW3(IF,y0,y1,y2)
#define SHOW5(IF,x,y0,y1,y2,y3) SHOW1(IF,x) SHOW4(IF,y0,y1,y2,y3)
#define SHOW6(IF,x,y0,y1,y2,y3,y4) SHOW1(IF,x) SHOW5(IF,y0,y1,y2,y3,y4)
#define SHOW7(IF,x,y0,y1,y2,y3,y4,y5) SHOW1(IF,x) SHOW6(IF,y0,y1,y2,y3,y4,y5)

#define SHOWM(IF,m,x) SHOWP(IF,m<<": ") SHOW(IF,x)
#define SHOWM0(IF,m,x) SHOWP(IF,m)
#define SHOWM1(IF,m,x) SHOWM(IF,m,x)
#define SHOWM2(IF,m,x0,x1) SHOWP(IF,m<<": ") SHOW2(IF,x0,x1)
#define SHOWM3(IF,m,x0,x1,x2) SHOWP(IF,m<<": ") SHOW3(IF,x0,x1,x2)
#define SHOWM4(IF,m,x0,x1,x2,x3) SHOWP(IF,m<<": ") SHOW4(IF,x0,x1,x2,x3)
#define SHOWM5(IF,m,x0,x1,x2,x3,x4) SHOWP(IF,m<<": ") SHOW5(IF,x0,x1,x2,x3,x4)
#define SHOWM6(IF,m,x0,x1,x2,x3,x4,x5) SHOWP(IF,m<<": ") SHOW6(IF,x0,x1,x2,x3,x4,x5)
#define SHOWM7(IF,m,x0,x1,x2,x3,x4,x5,x6) SHOWP(IF,m<<": ") SHOW7(IF,x0,x1,x2,x3,x4,x5,x6)

#endif
