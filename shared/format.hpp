#ifndef GRAEHL__SHARED__FORMAT_HPP
#define GRAEHL__SHARED__FORMAT_HPP

#include <boost/format.hpp>
#include <iomanip>

namespace fm {
using std::string;
using std::endl;
using std::flush;
using boost::format;
using boost::io::group;
using boost::io::str;
using std::setfill;
using std::setw;
using std::hex;
using std::dec;
using std::showbase;
using std::left;
using std::right;
using std::internal;
}

#define FSTR(x,y) fm::str(fm::format(x) % y)

#endif
