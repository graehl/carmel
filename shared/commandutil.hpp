#ifndef _COMMANDUTIL_HPP
#define _COMMANDUTIL_HPP

#include <string>
#include <fstream>

template <class T>
void extract_from_filename(const char *filename,T &to) {
    ifstream in(filename);
    if (!in)
        throw std::string("Couldn't read file ") + filename;
    else {
        in >> to;
    }
}

#ifdef TEST
#include "test.hpp"
#endif

#ifdef TEST
BOOST_AUTO_UNIT_TEST( TEST_COMMANDUTIL )
{
}
#endif

#endif
