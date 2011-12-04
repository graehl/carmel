#ifndef GRAEHL_SHARED__NUMERIC_HPP
#define GRAEHL_SHARED__NUMERIC_HPP

#include <boost/range.hpp>
#if 0
#include <boost/range/concepts.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/empty.hpp>
#include <boost/range/distance.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>
//#include <boost/range/as_literal.hpp>
#endif

namespace graehl {

using boost::begin;
using boost::end;
using boost::empty;
using boost::size;
using boost::distance;

using boost::const_rbegin;
using boost::const_rend;
using boost::const_begin;
using boost::const_end;
using boost::rbegin;
using boost::rend;

// for string, char*:
//using boost::as_literal; // UTF8
//using boost::as_array; // char[]

using boost::range_value;
using boost::range_iterator;
//using boost::range_size;
using boost::range_difference;
using boost::range_const_iterator;
//partial_sum

}//ns

#endif
