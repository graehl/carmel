#ifndef GRAEHL__SHARED__THE_NULL_OSTREAM_HPP
#define GRAEHL__SHARED__THE_NULL_OSTREAM_HPP

#include <graehl/shared/null_ostream.hpp>

#ifdef GRAEHL__SINGLE_MAIN
# define GRAEHL__NULL_OSTREAM_MAIN
#endif 

# ifdef GRAEHL__NULL_OSTREAM_MAIN
null_ostream the_null_ostream;
# else
/// singleton/constant (only need one)
extern null_ostream the_null_ostream;
# endif

#endif
