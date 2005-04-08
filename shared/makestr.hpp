#ifndef MAKESTRING_HPP
#define MAKESTRING_HPP

#include <sstream>
#include <stdexcept>


// ## __FILE__ ## ":" ## __LINE__ # "): failed to write " ## #expr
// usage: MAKESTRS(str,os << 1;os << 2 => str="12"
#define MAKESTRS(string,expr) do {std::ostringstream os;expr;if (!os) throw std::runtime_error("MAKESTRINGOS (");string=os.str();}while(0)
// usage: MAKESTR(str,1 << 2) => str="12"
#define MAKESTR(string,expr) MAKESTRS(string,os << expr)

// (obviously, no comma/newlines allowed in expr)


#ifdef TEST
#include "test.hpp"
#endif

#ifdef TEST_MAIN


BOOST_AUTO_UNIT_TEST( makestring )
{
    std::string s,e;
    e="12";
    MAKESTRS(s,os << "12");BOOST_CHECK(s==e);
    MAKESTRS(s,os << 1 << 2);BOOST_CHECK(s==e);
    MAKESTRS(s,os << 1;os << 2);BOOST_CHECK(s==e);

    //copy of MAKESTRS
    MAKESTR(s,"12");BOOST_CHECK(s==e);
    MAKESTR(s,1 << 2);BOOST_CHECK(s==e);
    MAKESTR(s,1;os << 2);BOOST_CHECK(s==e);
}
#endif

#endif
