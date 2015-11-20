// Copyright 2014 Jonathan Graehl-http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/** \file

    macros SHOW* for (conditional / debug build only) debugging prints,
    e.g. SHOWE(x) prints "x="<x<<" "


    SHOWS is the stream (std::cerr or whatever - you can define)
    SHOWP is just print.
    SHOWC(x) prints x=x
    SHOW.. is SHOWC.. + newline
    SHOWNL is just newline

    e.g. SHOWC(IFD, x) SHOWC(IFD, y) SHOW(IFD, nl_after)
    or, shorter, SHOW3(IFD, x, y, nl_after)

    will both print x=X y=Y nl_after=NL_AFTER\n if DEBUG.

    careful: none of this is wrapped in a block.  so you can't use one of these macros as a single-line block.

    e.g.

    #include <graehl/shared/ifdbg.hpp>
    DECLARE_DBG_LEVEL_IF(MODULE_IFDBG)
      then:  IFDBG(MODULE_IFDBG,1) { SHOWM2(MODULE_IFDBG, "descr"); }  (braces are mandatory for SHOW* macros
   to form a single statement)

      or:  SHOWIF1(MODULE_IFDBG, "descr", value)

    (see ifdbg.hpp for IF macro arguments e.g.


    #ifdef NDEBUG
    # define MODULE_IFDBG(x)
    #else
    # define MODULE_IFDBG(x) x
    #endif

    )

*/

#ifndef GRAEHL_SHARED__SHOW_HPP
#define GRAEHL_SHARED__SHOW_HPP
#pragma once

/// you can define SHOWS to something else first. needs to support SHOWS << x for output
#ifndef SHOWS
#include <iostream>
#define SHOWS std::cerr
#endif

#undef SHOWALWAYS
#define SHOWALWAYS(x) x

#define SHOWE(x) " " #x << "=" << (x) << " "
#define SHOWP(IF, x) IF(SHOWS << x;)
#define SHOWNL(IF) SHOWP(IF, "\n")
#define SHOWC(IF, x, s) SHOWP(IF, #x << "=" << (x) << s)
#define SHOW(IF, x) SHOWC(IF, x, "\n")
#define SHOW1(IF, x) SHOWC(IF, x, " ")
#define SHOW2(IF, x, y) SHOW1(IF, x) SHOW(IF, y)
#define SHOW3(IF, x, y0, y1) SHOW1(IF, x) SHOW2(IF, y0, y1)
#define SHOW4(IF, x, y0, y1, y2) SHOW1(IF, x) SHOW3(IF, y0, y1, y2)
#define SHOW5(IF, x, y0, y1, y2, y3) SHOW1(IF, x) SHOW4(IF, y0, y1, y2, y3)
#define SHOW6(IF, x, y0, y1, y2, y3, y4) SHOW1(IF, x) SHOW5(IF, y0, y1, y2, y3, y4)
#define SHOW7(IF, x, y0, y1, y2, y3, y4, y5) SHOW1(IF, x) SHOW6(IF, y0, y1, y2, y3, y4, y5)

#define SHOWM(IF, m, x) SHOWP(IF, m << ": ") SHOW(IF, x)
#define SHOWM0(IF, m, x) SHOWP(IF, m)
#define SHOWM1(IF, m, x) SHOWM(IF, m, x)
#define SHOWM2(IF, m, x0, x1) SHOWP(IF, m << ": ") SHOW2(IF, x0, x1)
#define SHOWM3(IF, m, x0, x1, x2) SHOWP(IF, m << ": ") SHOW3(IF, x0, x1, x2)
#define SHOWM4(IF, m, x0, x1, x2, x3) SHOWP(IF, m << ": ") SHOW4(IF, x0, x1, x2, x3)
#define SHOWM5(IF, m, x0, x1, x2, x3, x4) SHOWP(IF, m << ": ") SHOW5(IF, x0, x1, x2, x3, x4)
#define SHOWM6(IF, m, x0, x1, x2, x3, x4, x5) SHOWP(IF, m << ": ") SHOW6(IF, x0, x1, x2, x3, x4, x5)
#define SHOWM7(IF, m, x0, x1, x2, x3, x4, x5, x6) SHOWP(IF, m << ": ") SHOW7(IF, x0, x1, x2, x3, x4, x5, x6)

#endif
