#ifndef _TEST_HPP
#define _TEST_HPP

#include <string>
#include <sstream>

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4267 4535 )
#endif
//included/
//#include <boost/test/unit_test_framework.hpp>
#include <boost/test/auto_unit_test.hpp>
#ifdef _MSC_VER
#pragma warning( pop )
#endif
#ifdef BOOST_AUTO_TEST_MAIN 
#ifdef BOOST_NO_EXCEPTIONS
#include <cstdlib>
namespace boost
{
  void throw_exception(std::exception const & e) {
	std::exit(-1);
  }// user defined
#endif
#endif
#endif
